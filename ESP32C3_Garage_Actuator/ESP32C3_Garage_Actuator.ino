/*
 * ESP32-C3 Garage Door Actuator (Master)
 * 
 * Features:
 * - Controls 2 Garage Doors (Relays).
 * - HS96L03 (SSD1306) OLED Display.
 * - BLE Server & WiFi AP (Garage_Master).
 * - Web UI for MAC-to-Door mapping.
 * 
 * Hardware:
 * - Relay 1: GPIO 3 (D1)
 * - Relay 2: GPIO 4 (D2)
 * - OLED:   GPIO 6 (D4/SDA), GPIO 7 (D5/SCL)
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pins
#define RELAY1_PIN 3
#define RELAY2_PIN 4
#define SDA_PIN    6
#define SCL_PIN    7

// Relay Logic (Change these if your relay is Active-Low)
#define RELAY_ON   HIGH
#define RELAY_OFF  LOW

#define RELAY_TRIGGER_MS 500
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// BLE UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// WiFi
const char* AP_SSID = "Garage_Master";
String apPass = "admin123";

// Global Objects
Preferences preferences;
WebServer server(80);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

bool deviceConnected = false;
String currentPeerMAC = "";
String lastUnauthorizedMAC = "None";
unsigned long lastWebActionTime = 0;
const unsigned long WEB_LOCKOUT_MS = 4000;

// ... (Existing Storage Variables)

// ======================= CONFIG =======================
void loadWifiConfig() {
    preferences.begin("garage_cfg", true); // Read-only
    String savedPass = preferences.getString("ap_pass", "");
    if (savedPass.length() >= 8) {
        apPass = savedPass;
    }
    preferences.end();
}

void handleSetPass() {
    if (server.hasArg("pass")) {
        String newPass = server.arg("pass");
        if (newPass.length() >= 8) {
            preferences.begin("garage_cfg", false); // Read-write
            preferences.putString("ap_pass", newPass);
            preferences.end();
            
            String html = "<html><body><h2>Password Saved!</h2><p>Rebooting system... Connect with new password.</p></body></html>";
            server.send(200, "text/html", html);
            delay(1000);
            ESP.restart();
        } else {
            server.send(200, "text/html", "<html><body><h2>Error</h2><p>Password must be at least 8 characters.</p><a href='/'>Back</a></body></html>");
        }
    } else {
        server.sendHeader("Location", "/");
        server.send(303);
    }
}
// Simpler: 
// Key: "mac0", Value: "AA:BB..."
// Key: "perm0", Value: 1 (Door1), 2 (Door2), 3 (Both)
String allowedMACs[10];
int    allowedPerms[10];
int    allowedCount = 0;

// Helper: Format MAC
String macToString(esp_bd_addr_t bda) {
    char str[18];
    sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X",
            bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
    return String(str);
}

// ======================= DISPLAY =======================
void updateDisplay(String status, String subStatus = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Header
  display.setCursor(0,0);
  display.print("Garage Master");
  
  display.setCursor(0,10);
  display.print("IP: 192.168.4.1");
  
  display.setCursor(0,25);
  display.setTextSize(2);
  display.print(status);
  
  if(subStatus != "") {
    display.setTextSize(1);
    display.setCursor(0, 50);
    display.print(subStatus);
  }

  display.display();
}

// ======================= DATA MANAGEMENT =======================

void loadWhitelist() {
    preferences.begin("garage_auth", true);
    allowedCount = preferences.getInt("count", 0);
    for (int i = 0; i < allowedCount; i++) {
        String keyM = "mac" + String(i);
        String keyP = "perm" + String(i);
        allowedMACs[i] = preferences.getString(keyM.c_str(), "");
        allowedPerms[i] = preferences.getInt(keyP.c_str(), 0);
    }
    preferences.end();
}

void addMacToWhitelist(String mac, int permission) {
    if (allowedCount >= 10) return;
    
    // Check if exists, update permission if so
    for(int i=0; i<allowedCount; i++) {
      if(allowedMACs[i].equals(mac)) {
         preferences.begin("garage_auth", false); 
         String keyP = "perm" + String(i);
         preferences.putInt(keyP.c_str(), permission);
         allowedPerms[i] = permission;
         preferences.end();
         return;
      }
    }

    allowedMACs[allowedCount] = mac;
    allowedPerms[allowedCount] = permission;
    allowedCount++;

    preferences.begin("garage_auth", false);
    preferences.putInt("count", allowedCount);
    String keyM = "mac" + String(allowedCount - 1);
    String keyP = "perm" + String(allowedCount - 1);
    preferences.putString(keyM.c_str(), mac);
    preferences.putInt(keyP.c_str(), permission);
    preferences.end();
}

void clearWhitelist() {
    preferences.begin("garage_auth", false);
    preferences.clear();
    preferences.end();
    allowedCount = 0;
}

// Returns permission: 0=None, 1=Door1, 2=Door2, 3=Both
int getPermission(String mac) {
    for (int i = 0; i < allowedCount; i++) {
        if (allowedMACs[i].equals(mac)) return allowedPerms[i];
    }
    return 0;
}

// ======================= WEB SERVER =======================

void handleRoot() {
    String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family:sans-serif; padding:10px; background:#222; color:#eee;}";
    html += ".card{background:#333; padding:15px; border-radius:8px; margin-bottom:15px;}";
    html += "button{padding:8px 15px; margin:5px; border:none; border-radius:4px; cursor:pointer;}";
    html += ".btn-1{background:#3498db; color:white;} .btn-2{background:#e67e22; color:white;} .btn-3{background:#9b59b6; color:white;}";
    html += ".btn-big{padding:15px 30px; font-size:1.2em; width:100%; margin-bottom:10px;}";
    html += "ul{list-style:none; padding:0;} li{border-bottom:1px solid #444; padding:8px 0;}";
    html += "</style></head><body>";
    
    html += "<h2>Garage Master</h2>";
    
    // Direct Control
    html += "<div class='card'><h3>Direct Control</h3>";
    html += "<a href='/open?door=1'><button class='btn-1 btn-big'>Open Door 1</button></a>";
    html += "<a href='/open?door=2'><button class='btn-2 btn-big'>Open Door 2</button></a>";
    html += "</div>";

    // Recent Attempt
    html += "<div class='card'><h3>Recent Scan</h3>";
    html += "<p>MAC: <strong>" + lastUnauthorizedMAC + "</strong></p>";
    
    if (lastUnauthorizedMAC != "None") {
         html += "<div>Assign to:</div>";
         html += "<a href='/auth?mac=" + lastUnauthorizedMAC + "&door=1'><button class='btn-1'>Door 1</button></a>";
         html += "<a href='/auth?mac=" + lastUnauthorizedMAC + "&door=2'><button class='btn-2'>Door 2</button></a>";
         html += "<a href='/auth?mac=" + lastUnauthorizedMAC + "&door=3'><button class='btn-3'>Both</button></a>";
    }
    html += "</div>";

    // List
    html += "<div class='card'><h3>Paired Remotes (" + String(allowedCount) + "/10)</h3><ul>";
    for (int i = 0; i < allowedCount; i++) {
        String pStr = (allowedPerms[i] == 1) ? "Door 1" : (allowedPerms[i] == 2) ? "Door 2" : "BOTH";
        html += "<li>" + allowedMACs[i] + " <span style='float:right; color:#aaa'>" + pStr + "</span></li>";
    }
    html += "</ul><br><a href='/clear'><button style='background:#c0392b; color:white'>Clear All</button></a></div>";
    
    // WiFi Settings
    html += "<div class='card'><h3>WiFi Settings</h3>";
    html += "<form action='/setpass' method='POST'>";
    html += "<input type='text' name='pass' placeholder='New Password (min 8 chars)' style='padding:8px; width:70%; margin-right:5px;'>";
    html += "<button class='btn-1' style='background:#555;'>Save</button>";
    html += "</form></div>";
    
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleOpen() {
    // Lockout Check
    if (millis() - lastWebActionTime < WEB_LOCKOUT_MS) {
        Serial.println("Web ignored: Lockout active");
        server.sendHeader("Location", "/");
        server.send(303);
        return;
    }
    lastWebActionTime = millis();

    if (server.hasArg("door")) {
        int doorId = server.arg("door").toInt();
        triggerDoor(doorId);
    }
    server.sendHeader("Location", "/");
    server.send(303);
}

void handleAuth() {
    if (server.hasArg("mac") && server.hasArg("door")) {
        addMacToWhitelist(server.arg("mac"), server.arg("door").toInt());
        updateDisplay("Remote Added", server.arg("mac"));
        delay(1000);
        updateDisplay("Ready");
    }
    server.sendHeader("Location", "/");
    server.send(303);
}

void handleClear() {
    clearWhitelist();
    updateDisplay("Memory Cleared");
    delay(1000);
    updateDisplay("Ready");
    server.sendHeader("Location", "/");
    server.send(303);
}

// ======================= CONTROL =======================

void triggerDoor(int doorId) {
    int pin = (doorId == 1) ? RELAY1_PIN : RELAY2_PIN;
    Serial.printf("Triggering Door %d (Pin %d)\n", doorId, pin);
    
    updateDisplay("OPENING...", "Door " + String(doorId));
    
    digitalWrite(pin, RELAY_ON); 
    delay(RELAY_TRIGGER_MS);
    digitalWrite(pin, RELAY_OFF);
    
    updateDisplay("Ready");
}

// ======================= BLE CALLBACKS =======================

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device Connected (Waiting for Auth Payload...)");
      updateDisplay("Connected", "Verifying...");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device Disconnected");
      updateDisplay("Ready", "Waiting...");
      pServer->getAdvertising()->start();
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String value = pCharacteristic->getValue();
        
        // Expected Protocol: 1 Byte CMD (0x01) + MAC Address String
        // Example: 0x01 + "AA:BB:CC:DD:EE:FF"
        // Min length: 1 + 17 = 18 bytes
        
        if (value.length() >= 18 && value[0] == 0x01) {
            String clientMac = value.substring(1, 18); // Extract "AA:BB..."
            clientMac.toUpperCase(); 
            
            Serial.print("Auth Request from: "); Serial.println(clientMac);
            currentPeerMAC = clientMac; // Store for context

            int perm = getPermission(clientMac);
            
            if (perm > 0) {
                if (perm == 1) triggerDoor(1);
                else if (perm == 2) triggerDoor(2);
                else if (perm == 3) { triggerDoor(1); delay(200); triggerDoor(2); }
            } else {
                Serial.println("Unauthorized");
                lastUnauthorizedMAC = clientMac;
                updateDisplay("Denied!", clientMac);
            }
        } else {
            Serial.println("Invalid Payload Format");
        }
    }
};

// ======================= SETUP & LOOP =======================

void setup() {
    Serial.begin(115200);
    
    // Relays
    pinMode(RELAY1_PIN, OUTPUT);
    pinMode(RELAY2_PIN, OUTPUT);
    digitalWrite(RELAY1_PIN, RELAY_OFF);
    digitalWrite(RELAY2_PIN, RELAY_OFF);

    // OLED
    Wire.begin(SDA_PIN, SCL_PIN);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        Serial.println(F("SSD1306 allocation failed"));
    }
    display.clearDisplay();
    updateDisplay("Booting...");

    loadWhitelist();
    loadWifiConfig();

    // WiFi AP
    WiFi.softAP(AP_SSID, apPass.c_str());
    
    server.on("/", handleRoot);
    server.on("/open", handleOpen);
    server.on("/auth", handleAuth);
    server.on("/clear", handleClear);
    server.on("/setpass", HTTP_POST, handleSetPass);
    server.begin();

    // BLE
    BLEDevice::init("Garage-Master");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
    pCharacteristic->setCallbacks(new MyCallbacks());
    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();
    
    updateDisplay("Ready", "AP: " + String(AP_SSID));
}

void loop() {
    server.handleClient();
    delay(10); 
}
