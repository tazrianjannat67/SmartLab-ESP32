# Smart Chemistry Lab

An ESP32-based laboratory management and safety system that automates lighting, environmental
hazard response, door access control, and equipment tracking for a chemistry laboratory. Built in
**C/C++ (Arduino framework)** on a single ESP32 with a PIR sensor, MQ-2 gas/smoke sensor, IR flame
sensor, relay-driven fan and water pump, buzzer, and an RC522 RFID reader with an SG90 servo door
lock. The ESP32 also hosts a **web dashboard** over Wi-Fi. The prototype is built inside a small
wooden model of a laboratory.

## Features implemented

- **Automatic light control.** The PIR sensor (HC-SR501) detects movement and the ESP32 turns the
  LED (the lab light) ON. If no movement is detected for **30 seconds**, the LED is turned OFF. Any
  new movement turns it ON again and restarts the timer.
- **Gas, smoke and flame protection.** The MQ-2 sensor and the IR flame sensor are read every
  500 ms and printed on the serial terminal.
  - **Normal:** fan, pump, and buzzer stay OFF and the terminal prints `Environment normal`.
  - **Smoke/gas:** when the MQ-2 value is above **2000**, the terminal prints `SMOKE DETECTED!` and
    the ESP32 turns ON the **fan and buzzer**. The pump stays OFF.
  - **Flame:** when the flame value is below **1000**, the terminal prints `FIRE DETECTED!` and the
    ESP32 turns ON the **water pump and buzzer** and turns the fan OFF.
  - **Fire priority:** the flame condition is checked first, so a fire always gets the fire
    response, even when the MQ-2 reading is also high.
- **RFID door access.** The RC522 reader sends the UID of a scanned card to the ESP32 over SPI. An
  authorized UID unlocks the door with the SG90 servo (90 degrees for six seconds, then back to
  0 degrees). An unauthorized card keeps the door locked and sounds a buzzer.
- **Web dashboard.** The ESP32 connects to Wi-Fi and serves a dashboard (dark and light theme) with:
  - Light and fan toggle switches (the fan toggle is blocked while a smoke or fire alarm is active,
    because the safety logic has priority)
  - Live MQ-2 value and status (Normal / Warning / Danger), flame status, PIR motion status, and
    the state of the fan, buzzer, and pump
  - Wi-Fi status, IP address, and uptime
  - Date and time in Bangladesh time (UTC+6) synchronized online over NTP
- **Equipment tracking (web dashboard).** A table of 12 chemistry lab items (Test Tube, Beaker,
  Conical Flask, Measuring Cylinder, pH Meter, Volumetric Flask, Pipette, Burette, Funnel, Glass Rod,
  Watch Glass, Thermometer). A student enters their name and ID and selects an available item to
  receive it. The receive time is recorded automatically. Pressing **Component Back** records the
  submit time and clears the student's name and ID. Counters show total, borrowed, and available
  items.
- **Relay isolation.** A 2-channel optocoupler relay module switches the fan and the pump, so the
  ESP32 pins never drive these loads directly. The pump runs from a separate 18650 Li-ion battery.

## Dashboard-only placeholders

These parts of the dashboard are shown in the interface but are **not connected to real hardware
in the dashboard code**:

- **Temperature** is a fixed value (28.4 C). No temperature sensor is used.
- **Door status and last access** show a fixed "Locked" state and no access history in the
  dashboard sketch. The RFID and servo logic is not part of the dashboard code.
- **Cloud database** shows "Not Configured".

## Planned (not implemented)

- Saving equipment records to a **cloud database** (the current records are kept in the ESP32's RAM
  and are **lost when the board restarts**)
- Linking the RFID cards to equipment borrowing and returns
- Showing the real RFID access log on the dashboard

## Hardware components

| No. | Component | Model / Type |
|-----|-----------|--------------|
| 1 | ESP32 Dev Board | ESP-32S 30P NodeMCU |
| 2 | Breadboard | 830-point solderless breadboard |
| 3 | Jumper Wire | Male-to-female and female-to-female |
| 4 | Gas Sensor | MQ-2 Gas Sensor module |
| 5 | Flame Sensor | IR flame module (YG1006-based) |
| 6 | Buzzer | Active buzzer module, 5 V |
| 7 | Cooling Fan | 5 V DC cooling fan |
| 8 | Water Pump | Mini DC 3-6 V submersible pump |
| 9 | Battery | 18650 Li-ion, 3.7 V (for the pump) |
| 10 | Battery Holder | 18650 holder |
| 11 | Relay Module | 2-channel relay with optocoupler |
| 12 | PIR Sensor | HC-SR501 |
| 13 | LED | 5 mm red LED (represents the lab light) |
| 14 | RFID Reader and Card | RC522 module + 13.56 MHz card/tag kit |
| 15 | Servo Motor | SG90 micro servo, 9 g |
| 16 | USB Cable | B-type (powers the ESP32) |

## Pin connections

Taken from the dashboard sketch:

| Device | ESP32 Pin | Notes |
|--------|-----------|-------|
| PIR sensor (OUT) | GPIO 27 | Input |
| LED (lab light) | GPIO 2 | Active-HIGH |
| MQ-2 (AO) | GPIO 34 | Analog input |
| Flame sensor (AO) | GPIO 25 | Analog input, active-LOW signal |
| Buzzer | GPIO 26 | Active-LOW (HIGH = OFF) |
| Relay IN1 (fan) | GPIO 32 | Active-LOW (HIGH = OFF) |
| Relay IN2 (pump) | GPIO 33 | Active-LOW (HIGH = OFF) |
| Servo signal | GPIO `__` | Fill in from the RFID sketch |

## Setup

1. **Install the Arduino IDE** and add the ESP32 board package (Boards Manager).

2. **Install the libraries** from the Library Manager:
   - [MFRC522](https://github.com/miguelbalboa/rfid) for the RC522 RFID reader
   - [ESP32Servo](https://github.com/madhephaestus/ESP32Servo) for the SG90 servo

   The dashboard sketch itself only uses the libraries that come with the ESP32 core (`WiFi`,
   `WebServer`, `time`).

3. **Clone this repository:**
   ```bash
   git clone https://github.com/YOUR_USERNAME/YOUR_REPO.git
   ```

4. **Wire the circuit** using the pin table above. Power the ESP32 from USB and the water pump from
   the 18650 battery through the relay.

5. **Enter your Wi-Fi details** in the code. Use your own network name and password:
   ```cpp
   const char* ssid     = "YOUR_WIFI_NAME";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
   Never commit your real Wi-Fi password to a public repository.

6. **Check the thresholds** and adjust them for your sensors and room:
   ```cpp
   const int smokeThreshold = 2000;   // MQ-2 above this = smoke
   const int flameThreshold = 1000;   // flame value below this = fire
   const unsigned long motionDelay = 30000UL;  // light OFF after 30 s without motion
   ```

7. **Upload the code,** open the Serial Monitor at **115200** baud, and note the IP address that
   is printed after `WiFi Connected!`.

8. **Open the dashboard:** type that IP address in a browser on the same Wi-Fi network.

## Try the end-to-end test

1. **Normal state.** Open the Serial Monitor. You should see lines like
   `MQ-2: 312 | Flame: 2890` followed by `Environment normal`, with the fan, pump, and buzzer OFF.
2. **Light control.** Move your hand in front of the PIR sensor. The terminal prints
   `Movement detected -> LED ON` and the LED turns ON. Stay away for 30 seconds and it prints
   `No movement for 30 seconds -> LED OFF`.
3. **Smoke test.** Hold a gas source (for example an unlit lighter releasing gas) near the MQ-2
   sensor. The terminal prints `SMOKE DETECTED!` and the fan and buzzer turn ON. The dashboard
   shows the gas status as Danger.
4. **Fire test.** Bring a small flame near the flame sensor at a safe distance, away from wires and
   the wooden model. The terminal prints `FIRE DETECTED!` and the pump and buzzer turn ON while the
   fan turns OFF.
5. **Dashboard controls.** Use the Light and Fan switches. Try the Fan switch during a smoke alarm:
   it is refused, because the safety logic has priority.
6. **Equipment tracking.** Enter a student name and ID, choose an available item, and press
   **Receive Equipment**. The item shows as Borrowed with the receive time. Press **Component Back**
   to return it and record the submit time.
7. **RFID door.** Scan the authorized card to open the door, and an unauthorized card to see
   `Access Denied`.

### Sample serial output

```
MQ-2: 312 | Flame: 2890
Environment normal

Movement detected -> LED ON
No movement for 30 seconds -> LED OFF

MQ-2: 2450 | Flame: 2870
SMOKE DETECTED!
Smoke -> Fan ON + Buzzer ON

MQ-2: 2480 | Flame: 850
FIRE DETECTED!
Fire priority -> Pump ON + Buzzer ON
```

## Dashboard routes

| Route | Purpose |
|-------|---------|
| `/` | The dashboard page |
| `/status` | JSON with all sensor values, output states, time, and equipment list |
| `/fan/toggle` | Toggle the fan (refused during a smoke or fire alarm) |
| `/light/toggle` | Toggle the light |
| `/equipment/receive?index=&name=&id=` | Record that a student received an item |
| `/equipment/return?index=` | Record that an item was returned |

## Project structure

> Edit this to match your repository.

```
smart-chemistry-lab/
  SmartLabDashboard/
    SmartLabDashboard.ino   # Wi-Fi dashboard, safety logic, equipment tracking
  SmartLabRFID/
    SmartLabRFID.ino        # RFID door access with servo
  images/                    # Prototype photos and serial monitor screenshots
  docs/                      # Project paper (LaTeX / PDF)
  README.md
```

## Notes on scope

- This is a **prototype built in a small model**, not a certified laboratory safety system. It does
  not replace responsible laboratory supervision or real fire-protection equipment.
- The dashboard has **no login**. Anyone on the same Wi-Fi network who knows the IP address can
  toggle the light and fan and edit the equipment records.
- Equipment records are stored in RAM only and are lost on restart.
- The light uses a **fixed 30-second** inactivity period, and the MQ-2 and flame thresholds are
  **fixed values** that may need calibration for other sensors or environments.
- The RFID check compares the card **UID** only. UIDs of basic cards can be copied, so this is a
  demonstration of access control, not a high-security lock.

## Future work

- Cloud database for equipment records, and RFID-based borrowing and returns
- Real RFID access log and door status on the dashboard
- A temperature sensor in place of the dashboard placeholder
- Login for the dashboard
- Sensor calibration and more robust hazard confirmation


