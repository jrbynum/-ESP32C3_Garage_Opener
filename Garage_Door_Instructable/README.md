# Secure Dual-Channel Garage Door Opener (ESP32-C3)

## Project Overview
This project allows you to build a secure, Bluetooth Low Energy (BLE) based garage door opener system. It consists of two parts:
1.  **The Actuator (Master):** Installed in your garage, physically connected to your garage door opener motor(s). It hosts a WiFi access point for configuration and listens for BLE signals.
2.  **The Controller (Remote):** A small, battery-powered remote that you keep in your car. It wakes up only when the button is pressed, sends a secure signal, and goes back to sleep.

## Key Features
*   **Dual Channel:** Control up to two separate garage doors.
*   **Secure Pairing:** Remotes must be explicitly authorized via a Web Interface.
*   **OLED Display:** The Master unit shows real-time status and IP address.
*   **Low Power Remote:** The remote uses Deep Sleep, lasting months on a battery.
*   **Web Management:** Manage paired remotes via a simple web page hosted on the Master.

---

## Bill of Materials (BoM)

### Master Unit (Actuator)
*   1x **Seeed Studio XIAO ESP32C3**
*   2x **Relay Modules** (5V or 3.3V, compatible with microcontroller logic)
*   1x **SSD1306 OLED Display** (0.96" I2C)
*   1x **5V USB Power Supply** (e.g., old phone charger)
*   Connecting wires / Breadboard / PCB

### Remote Unit (Controller)
*   1x **Seeed Studio XIAO ESP32C3**
*   1x **Momentary Push Button**
*   1x **Battery** (3.7V LiPo recommended for stability, or Coin Cell if capable of high peak current)
*   **3D Printed Case:** Files `Garage Remote Enclosure.stl` and `Garage Remote Enclosure Lid.stl` located in `../ESP32C3_Garage_Controller/`.

---

## Assembly Instructions

**See `Wiring_Diagram.txt` in this folder for a visual guide.**

### 1. Wiring the Master
1.  **Relays:**
    *   Connect **Relay 1 Control** to Pin **D1 (GPIO 3)**.
    *   Connect **Relay 2 Control** to Pin **D2 (GPIO 4)**.
    *   Connect VCC/GND appropriate for your relay module.
2.  **OLED Display:**
    *   Connect **SDA** to Pin **D4 (GPIO 6)**.
    *   Connect **SCL** to Pin **D5 (GPIO 7)**.
    *   Connect VCC to 3.3V and GND to GND.
3.  **Garage Door Motor:**
    *   Connect the **Common (COM)** and **Normally Open (NO)** terminals of the Relay to the "Push Button" terminals on your Garage Door Opener motor.

### 2. Wiring the Remote
1.  **Button:**
    *   Connect one leg of the button to **D0 (GPIO 2)**.
    *   Connect the other leg to **GND**.
2.  **Battery:**
    *   Connect battery to the BAT/GND pads (underneath) or 5V/GND pins if using a booster.

---

## Software Installation

### Prerequisites
*   Arduino IDE (Legacy or 2.0+)
*   **Board Package:** `esp32` by Espressif Systems.
*   **Libraries:** 
    *   `Adafruit_GFX`
    *   `Adafruit_SSD1306`

### Flashing the Master
1.  Open `../ESP32C3_Garage_Actuator/ESP32C3_Garage_Actuator.ino`.
2.  Select Board: **Seeed XIAO ESP32C3**.
3.  Connect the Master unit via USB.
4.  Upload the code.
5.  *Note:* The OLED should light up and show "Garage Master" and "IP: 192.168.4.1".

### Flashing the Remote
1.  Open `../ESP32C3_Garage_Controller/ESP32C3_Garage_Controller.ino`.
2.  Select Board: **Seeed XIAO ESP32C3**.
3.  Connect the Remote unit via USB.
4.  Upload the code.
5.  *Note:* Open the Serial Monitor immediately after upload to see the "CONTROLLER ID". Write this down if you wish, but the Master will also auto-detect it.

---

## Setup & Pairing Guide

### 1. Initial Power Up
Power on the **Master Unit**. It will create a WiFi Access Point named **`Garage_Master`**.

### 2. Connect to Web Interface
1.  Connect your phone or laptop to the WiFi network `Garage_Master`.
2.  Password: `admin123`.
3.  Open a browser and navigate to `http://192.168.4.1`.

### 3. Pair a New Remote
1.  Press the button on your **Remote Unit**. 
    *   *The remote will wake up, try to connect, fail (because it's not allowed yet), and go back to sleep.*
2.  Look at the **Master Unit's OLED Display** or the **Web Interface**.
    *   The Web Interface will show a "Recent Scan" with a MAC address (e.g., `AA:BB:CC:11:22:33`).
3.  On the Web Interface, click one of the buttons under "Assign to:":
    *   **Door 1:** Remote triggers Relay 1.
    *   **Door 2:** Remote triggers Relay 2.
    *   **Both:** Remote triggers both (useful if you have double doors or lighting).
4.  The remote is now paired!

### 4. Testing
Press the button on the Remote again.
*   **Master Display:** Should show "Remote Added" or "Opening...".
*   **Relay:** You should hear the relay click.

---

## Security & Maintenance
*   **WiFi Password:** You can change the AP password from the Web Interface.
*   **Revoking Access:** Click "Clear All" on the Web Interface to remove all paired remotes.
*   **Battery:** If the remote stops working, check the battery voltage. The remote sleeps 99% of the time, so battery life should be significant.
