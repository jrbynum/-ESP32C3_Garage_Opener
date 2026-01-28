# Secure ESP32-C3 Garage Door System

A dual-channel, secure, and web-managed garage door opener system built on the Seeed Studio XIAO ESP32C3. It features a master actuator unit with an OLED display and a low-power, deep-sleeping remote controller.

## 📂 Project Structure

*   **`ESP32C3_Garage_Actuator/`**: Source code for the main unit installed in the garage.
*   **`ESP32C3_Garage_Controller/`**: Source code for the battery-powered remote, plus STL files for the 3D-printed case.
*   **`Garage_Door_Instructable/`**: Additional documentation and diagrams.

---

## 🛠️ Hardware Architecture

### 1. Actuator (Master Unit)
*   **Role:** Controls relays, hosts the Web UI (`192.168.4.1`), and manages the authorized user list (whitelist).
*   **Key Components:**
    *   ESP32-C3 (XIAO)
    *   SSD1306 OLED (I2C)
    *   2x Relays (Active High/Low configurable)

**Wiring Diagram:**
```text
                         +----------------------+
                         |  SEEED XIAO ESP32C3  |
    [5V PSU]------------>| 5V                   |
      |     +----------->| GND                  |
      |     |            |                      |
      |     |      +---->| D1 (GPIO 3) ---------------------> RELAY 1 (Door 1)
      |     |      |     |                      |
      |     |      |   +>| D2 (GPIO 4) ---------------------> RELAY 2 (Door 2)
      |     |      |   | |                      |
      |     |      |   | | D4 (GPIO 6) ---------------------> OLED SDA
      |     |      |   | |                      |
      |     |      |   | | D5 (GPIO 7) ---------------------> OLED SCL
      |     |      |   | |                      |
      |     |      |   | | 3V3 -----------------------------> OLED VCC
      |     |      |   | +----------------------+
      |     |      |   |
      |     |      +----------------------------------------[RELAY MODULES]
      +--------------------------------------------------> GND (Common)
```

### 2. Controller (Remote Unit)
*   **Role:** Wakes up on button press, broadcasts a secure auth token via BLE, and immediately sleeps.
*   **Key Components:**
    *   ESP32-C3 (XIAO)
    *   Momentary Button
    *   LiPo Battery or Coin Cell

**Wiring Diagram:**
```text
                         +----------------------+
                         |  SEEED XIAO ESP32C3  |
    [BATTERY +]--------->| 3V3 / BAT            |
                         |                      |
    [BATTERY -]--+------>| GND                  |
                 |       |                      |
                 |     +>| D0 (GPIO 2)          |
                 |     | +----------------------+
                 |     |
             [BUTTON]  |
                 |     |
                 +-----+
```

---

## 💻 Code Walkthrough

### Actuator (`ESP32C3_Garage_Actuator.ino`)
The system acts as a **BLE Server** and a **WiFi Access Point**.

1.  **Security (Whitelist):**
    *   It maintains a persistent list of allowed MAC addresses in `Preferences` (Flash memory).
    *   Each MAC is assigned a permission level: `1` (Door 1), `2` (Door 2), or `3` (Both).
2.  **BLE Handler:**
    *   Listens for write requests to a specific Characteristic UUID.
    *   Extracts the MAC address from the payload (`0x01` + `MAC`).
    *   Checks the whitelist. If authorized, it triggers the corresponding relay.
3.  **Web Interface:**
    *   Hosted at `http://192.168.4.1` (SSID: `Garage_Master`).
    *   **Dashboard:** Shows the last unauthorized MAC address scan.
    *   **Enrollment:** Allows the admin to assign that unauthorized MAC to a specific door with one click.

### Controller (`ESP32C3_Garage_Controller.ino`)
The remote is designed for extreme power efficiency.

1.  **Deep Sleep:** The CPU is powered down 99.9% of the time.
2.  **Wake-up:** Triggered only when **GPIO 2** is pulled LOW (Button Press).
3.  **Fast Scan & Connect:**
    *   Upon wake-up, it aggressively scans for the `Garage-Master` BLE service.
    *   Connects -> Sends Command (`0x01` + its own MAC) -> Disconnects.
    *   Entire process typically takes <2 seconds before returning to sleep.

---

## 🚀 How to Use

### 1. Installation
1.  **Actuator:** Connect the Relay NO/COM ports to your garage door opener's wall-button terminals. Power the ESP32 via USB.
2.  **Remote:** Assemble the remote in the 3D-printed case (`.stl` files provided). Ensure the battery is charged.

### 2. First-Time Setup
1.  Power on the **Actuator**.
2.  On your phone/PC, connect to WiFi **`Garage_Master`** (Pass: `admin123`).
3.  Go to **`http://192.168.4.1`**.

### 3. Pairing a Remote
1.  **Press the button** on your new Remote. It won't work yet, but the Master will see it.
2.  Refresh the Web UI on your phone.
3.  Under **"Recent Scan"**, you will see the Remote's MAC address.
4.  Click **"Assign to Door 1"** (or Door 2).
5.  **Done!** Press the remote button again to test.

---

## 🖨️ 3D Printing
STL files for the remote enclosure are located in the `ESP32C3_Garage_Controller` folder:
*   `Garage Remote Enclosure.stl` (Base)
*   `Garage Remote Enclosure Lid.stl` (Snap-fit lid)

**Recommended Settings:**
*   Material: PLA or PETG
*   Layer Height: 0.2mm
*   Infill: 20%
*   Supports: None required
