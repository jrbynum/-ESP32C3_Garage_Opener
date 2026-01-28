/*
 * ESP32-C3 Garage Door Controller (Secure Client - Low Power)
 * 
 * Logic:
 * 1. Deep Sleep by default.
 * 2. Wakes on Button Press (GPIO 2, LOW).
 * 3. Scans for Garage-Master.
 * 4. Connects, Sends Trigger, Disconnects.
 * 5. Returns to Deep Sleep.
 * 
 * Hardware: Button on GPIO 2 (D0) to GND.
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define BUTTON_PIN 2
#define SCAN_TIMEOUT_SEC 5
#define TOTAL_TIMEOUT_SEC 10 // Safety cutoff

static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

static BLEAdvertisedDevice* myDevice = nullptr;
bool deviceFound = false;

// ===================== SLEEP HELPER =====================
void goToSleep() {
    Serial.println("Entering Deep Sleep...");
    delay(100); // Flush serial
    
    // Configure Wakeup on Button (GPIO 2) LOW
    // Ensure button is wired between GPIO 2 and GND
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    esp_deep_sleep_enable_gpio_wakeup(1ULL << BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
    
    esp_deep_sleep_start();
}

// ===================== BLE CALLBACKS =====================
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      deviceFound = true;
    }
  }
};

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {}
  void onDisconnect(BLEClient* pclient) {}
};

// ===================== MAIN FLOW =====================
void setup() {
  Serial.begin(115200);
  
  // 1. Identify reason for wakeup
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  if (cause != ESP_SLEEP_WAKEUP_GPIO) {
      Serial.println("Power On Reset / Not Button Wake.");
      // If it's fresh power-on, we might want to print ID then sleep
      BLEDevice::init("Garage-Controller");
      String myMac = BLEDevice::getAddress().toString().c_str();
      Serial.print("CONTROLLER ID: "); Serial.println(myMac);
      Serial.println("Sleeping in 5s...");
      delay(5000);
      goToSleep();
  }

  Serial.println("Wakeup! Scanning...");
  
  BLEDevice::init("Garage-Controller");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(100); // Fast scan
  pBLEScan->setWindow(99);
  pBLEScan->setActiveScan(true);
  
  // 2. Scan
  pBLEScan->start(SCAN_TIMEOUT_SEC, false);
  
  // 3. Connect & Send
  if (deviceFound && myDevice != nullptr) {
      Serial.println("Found Master. Connecting...");
      BLEClient* pClient = BLEDevice::createClient();
      pClient->setClientCallbacks(new MyClientCallback());

      if (pClient->connect(myDevice)) {
          BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
          if (pRemoteService != nullptr) {
                                BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
                            if (pRemoteCharacteristic != nullptr) {
                                Serial.println("Sending Auth Payload...");
                                
                                // Construct Payload: [0x01] + "AA:BB:CC:DD:EE:FF"
                                String myMac = BLEDevice::getAddress().toString().c_str();
                                String payload = "";
                                payload += (char)0x01; // Command Byte
                                payload += myMac;      // MAC String
                                
                                pRemoteCharacteristic->writeValue(payload.c_str(), payload.length());
                                delay(200); // Ensure transmission
                            }          }
          pClient->disconnect();
      } else {
          Serial.println("Failed to connect.");
      }
  } else {
      Serial.println("Master not found.");
  }

  // 4. Sleep
  goToSleep();
}

void loop() {
  // Empty - Logic is linear in setup()
}
