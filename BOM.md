# Bill of Materials (BOM) & Part Selection

## 🔋 Recommended Battery for Controller

For the **Seeed Studio XIAO ESP32C3**, the **highly recommended** battery is a **3.7V Single-Cell (1S) LiPo Battery**.

### Why not a Coin Cell (CR2032)?
While the ESP32-C3 is efficient, WiFi/BLE transmission creates short current spikes (~150mA-300mA). A standard CR2032 coin cell has high internal resistance and can drop the voltage too low during these spikes, causing the chip to reset ("brownout").
*   **Recommendation:** Use a small LiPo. The XIAO ESP32C3 has **built-in battery charging** capabilities via the USB-C port, making this the easiest and most reliable option.

### Specific Battery Specs
*   **Type:** Lithium Polymer (LiPo)
*   **Voltage:** 3.7V Nominal (4.2V Max)
*   **Capacity:** 150mAh to 400mAh (Trade-off between size and runtime).
*   **Size (Max Dimensions for Case):**
    *   Ideally approx **30mm x 20mm x 6mm** to fit comfortably in a small enclosure.
    *   Common standard sizes: **502030** (300mAh) or **402025** (150mAh).
*   **Connector:** JST-PH 2.0mm (standard) or bare wires (solder directly to XIAO battery pads on the back).

---

## 🛒 EasyEDA / LCSC Compatible Parts List

This list includes both the **Modules** (for assembling with wires) and the **Discrete Components** (if you decide to design a custom PCB carrier board in EasyEDA).

### 1. Actuator (Master Unit)

| Component | Description | Quantity | LCSC Part # (for PCB Design) | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Microcontroller** | Seeed Studio XIAO ESP32C3 | 1 | *Module* | Search "XIAO ESP32C3" on Digikey/Mouser |
| **OLED Display** | 0.96" SSD1306 I2C (128x64) | 1 | *Module* | Standard 4-pin header pinout (GND, VCC, SCL, SDA) |
| **Relay** | 5V Relay (SRD-05VDC-SL-C) | 2 | **C3790** | Most common relay component |
| **Relay Driver** | NPN Transistor (2N2222 or S8050) | 2 | **C11853** (S8050) | Required if driving raw relay from MCU |
| **Flyback Diode** | 1N4148 (Protection for Relay) | 2 | **C81598** | Place across relay coil |
| **Terminal Block** | 2-Pin Screw Terminal (5.08mm) | 2 | **C8425** | For connecting garage wires |
| **Resistor** | 1kΩ (Base Resistor for Transistor) | 2 | **C17513** (0603 SMD) | |
| **Buck Converter** | 12V/24V to 5V (MP1584 or LM2596) | 1 | *Module* | Optional: To power from Garage Opener's 12V/24V line |

### 2. Controller (Remote Unit)

| Component | Description | Quantity | LCSC Part # (for PCB Design) | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Microcontroller** | Seeed Studio XIAO ESP32C3 | 1 | *Module* | |
| **Button** | Tactile Switch 6x6x5mm | 1 | **C318884** | Standard through-hole or SMD |
| **Battery Socket** | JST-PH 2.0mm 2-Pin Connector | 1 | **C137171** | Optional: To make battery removable |
| **Slide Switch** | SPDT Slide Switch (Power Cutoff) | 1 | **C29079** | Optional: To turn off remote completely |

### 3. PCB Carrier Board (Optional Design)
If you design a PCB in EasyEDA to hold the XIAO, Relays, and Terminals, use these footprints:

*   **XIAO Headers:** 1x7 2.54mm Female Header (**C2255**) - You need 2 rows.
*   **OLED Header:** 1x4 2.54mm Female Header (**C2255**).
*   **Mounting Holes:** M2 or M3 screw holes for the enclosure.

---

## 🔌 Connection Summary for BOM

### Actuator Wiring
*   **XIAO 5V** -> Relay VCC & OLED VCC
*   **XIAO GND** -> Relay GND & OLED GND
*   **XIAO D1** -> Relay 1 IN
*   **XIAO D2** -> Relay 2 IN
*   **XIAO D4** -> OLED SDA
*   **XIAO D5** -> OLED SCL

### Controller Wiring
*   **Battery +** -> XIAO BAT+ (Solder pad on back)
*   **Battery -** -> XIAO BAT- (Solder pad on back)
*   **Button** -> Pin D0 (GPIO 2) to GND
