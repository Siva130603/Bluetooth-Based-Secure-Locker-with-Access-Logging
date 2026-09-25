
<div align="center">

# 🔒 Bluetooth-Based Secure Locker with Access Logging

### A Two-Factor Embedded Access-Control System on the LPC2148 (ARM7)

*A dual-authentication embedded security system built on the ARM7 (LPC2148), combining Bluetooth and physical keypad verification with real-time tamper detection and RTC-based audit logging.*

![Platform](https://img.shields.io/badge/Platform-ARM7%20LPC2148-blue?style=flat-square)
![Language](https://img.shields.io/badge/Language-Embedded%20C-00599C?style=flat-square)
![IDE](https://img.shields.io/badge/IDE-Keil%20µVision-orange?style=flat-square)
![Bluetooth](https://img.shields.io/badge/Wireless-HC--05%20Bluetooth-0082FC?style=flat-square)
![Status](https://img.shields.io/badge/Status-Complete-brightgreen?style=flat-square)

</div>

---

## 📖 Overview

This project is a two-factor embedded access-control system built on the **NXP LPC2148 (ARM7TDMI-S)**. A user first sends a 4-digit password from a phone over **Bluetooth** (Level-1); if correct, they're prompted to enter a second 4-digit password on a **physical keypad** (Level-2). Only when both match does the system drive a **DC motor** to open — and then automatically close — the locker. Every significant event is timestamped via the on-chip RTC and streamed out over UART0 as an audit log.

---
## Block Diagram




## ✨ Highlights

| | Feature | Description |
|---|---|---|
| 📱 | **Bluetooth Authentication** | Level-1 password sent via HC-05 (UART1) from any Bluetooth serial app |
| 🔢 | **Keypad Authentication** | Level-2 password entered on a 4×4 matrix keypad after Level-1 succeeds |
| ⚙️ | **Motor-Driven Locking** | L293D H-bridge drives a DC motor to physically open/close the locker |
| 🛡️ | **Tamper Detection** | Active-LOW switch triggers an instant alert if the enclosure is opened |
| 🕒 | **RTC Audit Logging** | Every event is timestamped and streamed over UART0 for a PC-side access log |
| 💾 | **EEPROM Password Storage** | Passwords persist across power cycles via I2C (AT24C256) |
| 🧑‍💻 | **Admin Configuration Menu** | Edit the RTC date/time or change either password — all on-device |
| 🔊 | **Buzzer Alerts** | Sounds on failed authentication or tamper detection |
| 🖥️ | **LCD Status Display** | 16×2 LCD shows live system state — standby, prompts, results, alerts |

---

## 🔌 Hardware & Pin Connections

| Module | Signal | LPC2148 Pin |
|---|---|---|
| 🖥️ **LCD (16×2, 4-bit mode)** | RS | P0.16 |
| | EN | P0.17 |
| | D4 – D7 | P0.18 – P0.21 |
| 🖧 **UART0** (debug/log → PC) | TXD0 | P0.0 |
| | RXD0 | P0.1 |
| 💾 **I2C0** (AT24C256 EEPROM) | SCL0 | P0.2 |
| | SDA0 | P0.3 |
| 🛡️ **Tamper Switch** | Signal (active LOW) | P0.4 |
| 🧑‍💻 **Admin Button** | EINT2 (falling edge) | P0.7 |
| 📶 **UART1** (HC-05 Bluetooth) | TXD1 | P0.8 |
| | RXD1 | P0.9 |
| 🔢 **Keypad (4×4 matrix)** | Rows | P1.16 – P1.19 |
| | Columns | P1.20 – P1.23 |
| ⚙️ **Motor Driver (L293D)** | IN1 | P1.24 |
| | IN2 | P1.25 |
| 🔊 **Buzzer** | Signal | P1.26 |

**Core clock:** 60 MHz (12 MHz crystal) · **PCLK:** 15 MHz · **UART baud rate:** 9600

---

## 🔑 Default Credentials

| Level | Method | Factory Default |
|---|---|---|
| Level 1 | Bluetooth | `1234` |
| Level 2 | Keypad | `5678` |

> Written to EEPROM automatically on first boot only. **Change both via the admin menu before real-world use.**

---

## 🧑‍💻 Admin Menu

Triggered by a dedicated push-button on **EINT2**, which suspends normal operation and opens the configuration menu on the LCD:

```
1. EDIT RTC        → hour / minute / second / date / month / year / day
2. EDIT PASSWORDS  → change Level-1 or Level-2 (old → new → confirm)
3. EXIT            → return to normal locker operation
```

---

## 🗂️ Project Structure

```
Bluetooth-Based-Secure-Locker/
├── README.md
├── locker_project.uvproj
├── src/
│   ├── main.c          # System init, main state machine, admin menu wiring
│   ├── menu.c            # Admin menu (Edit RTC / Edit Passwords / Exit)
│   ├── bluetooth.c        # HC-05 UART1 command parsing (Level-1 password)
│   ├── keypad.c            # 4x4 matrix keypad driver (Level-2 password)
│   ├── lcd.c                # 16x2 LCD driver (4-bit mode)
│   ├── eeprom.c              # AT24C256 I2C read/write (passwords + RTC backup)
│   ├── rtc.c                  # On-chip RTC read/write, timestamp formatting
│   ├── motor.c                  # L293D motor control (open/close locker)
│   ├── buzzer.c                   # Buzzer alert patterns
│   ├── security.c                  # Tamper detection + UART0 event logging
│   ├── uart.c                        # UART0/UART1 low-level driver
│   ├── delay.c                         # Software delay routines
│   └── Startup.s                        # ARM startup assembly
├── include/                               # Header files (one per module)
└── documentation/                           # Diagrams and test screenshots
```

---

## 📸 Hardware Prototype

![Hardware prototype](documentation/04_access_granted.jpg)

*Complete hardware implementation of the Bluetooth-Based Secure Locker on the Vector India LPC2148 development board — LCD, 4×4 keypad, HC-05 Bluetooth, MAX232, EEPROM, and L293D motor driver all wired and functional.*

---

## 🔌 Hardware Block Diagram

![Hardware block diagram](documentation/circuit_block_diagram.png)

*System block diagram — LPC2148 connected to keypad, LCD, buzzer, L293D/DC motor, EEPROM, HC-05 Bluetooth, and PC via MAX232.*

---

## 🔍 How It Works — Full Walkthrough

1️⃣ **Power On** — LPC2148 initializes all peripherals (LCD, UART, I2C, RTC, keypad, motor, buzzer). On first-ever boot, default passwords (`1234` Bluetooth / `5678` keypad) are written to EEPROM.

2️⃣ **Standby** — LCD shows an idle screen; system waits for a Bluetooth password or an admin button press.

3️⃣ **Level-1 (Bluetooth)** — User sends a 4-digit password via HC-05, terminated with `#` (e.g. `1234#`). Wrong → access denied, buzzer alert, event logged. Correct → proceeds to Level-2.

4️⃣ **Level-2 (Keypad)** — User enters the second password on the keypad. Wrong → same denial flow. Correct → access granted.

5️⃣ **Unlock** — Motor (via L293D) rotates forward to open the locker.

6️⃣ **Auto-Lock** — After the configured access period, the motor reverses and re-locks automatically.

7️⃣ **Logging** — Every event (boot, BT request, pass/fail, open/close, tamper, admin action) is timestamped via RTC and streamed over UART0 to a PC terminal.

8️⃣ **Tamper Detection** — Monitored continuously in the background; opening the enclosure triggers an immediate alarm regardless of locker state.

9️⃣ **Admin Mode** — Pressing the admin button opens the on-device menu to edit the RTC or update either password.

---

## 🧩 Software Architecture

The firmware is split into focused, single-responsibility modules:

- **Application Layer** (`main.c`) — initializes peripherals and runs the main state machine that sequences the two-factor auth flow, motor control, and admin menu.
- **Logic Layer** (`security.c`, `menu.c`) — tamper detection, default password initialization, and event logging (`security.c`); the admin menu for RTC and password edits (`menu.c`).
- **Driver Layer** — one focused file per peripheral: `bluetooth.c`, `keypad.c`, `lcd.c`, `eeprom.c`, `rtc.c`, `motor.c`, `buzzer.c`, `uart.c`. Each only knows how to talk to its own hardware.

This separation means changing one peripheral's driver — say, swapping the LCD — only touches that file, not the rest of the codebase.

---

## 🔐 Security Design

- **Two-Factor Authentication** — a Bluetooth password alone, or a keypad password alone, is not enough; both are required.
- **Persistent Storage** — passwords live in EEPROM (I2C), not volatile memory, so they survive power loss and can only be changed via the admin menu.
- **Tamper Detection** — a dedicated switch is monitored continuously and independently of authentication state.
- **Full Audit Trail** — every event (logins, denials, tamper, admin actions) is RTC-timestamped and logged over UART0.

---

## 📡 Communication Protocol

- **Bluetooth (Level-1)** — HC-05 over UART1 @ 9600 baud. Password sent as digits + `#` terminator (e.g. `1234#`). If digits arrive without the `#` terminator within `BT_PENDING_TIMEOUT_MS` (3000 ms), the incomplete command is cleared automatically so a stale partial entry can't merge with the next attempt.
- **Keypad (Level-2)** — row/column matrix scanning on P1.16–P1.23.
- **PC Audit Log (UART0)** — a separate channel streaming a live, human-readable log line per event, viewable on a laptop via USB-to-TTL/MAX232.

---

## 📋 UART Event Logging

Every significant event is transmitted over UART0 as a timestamped log line, giving a complete real-time audit trail visible on a connected PC terminal:

- Records Bluetooth (Level-1) and keypad (Level-2) authentication attempts
- Logs successful and failed password matches
- Displays locker open and close operations
- Shows administrator activity such as password updates
- Maintains RTC-based timestamps for every event
- Provides a complete access history for security review

![UART event log](documentation/06_uart_event_log.png)

*Captured UART0 output from a live session — showing the full Level-1 Bluetooth match, Level-2 keypad match, locker open/close, and return to idle, each with an RTC timestamp.*

---

## 🧪 Testing Results

The complete authentication and locker-control flow was tested end-to-end on hardware.

**1. Idle state — waiting for Bluetooth password**

![Waiting for Bluetooth](documentation/01_waiting_bluetooth.jpg)

LCD displays "Waiting BT Pwd, Send from #", confirming the system correctly waits for a Level-1 Bluetooth password.

This confirms the LPC2148 boots correctly, initializes all peripherals, and enters a stable idle state without requiring any manual reset. The UART1 receive interrupt for the HC-05 module was verified to be armed and listening in this state — no password digits are accepted or buffered until the system is in this idle/waiting mode, which prevents stray Bluetooth data from being misread as a password attempt before the system is ready.

**2. Level-1 authentication success**

![Level-1 auth success](documentation/02_level1_auth_success.jpg)

On receiving the correct Bluetooth password, the LCD updates to "L1 Auth OK! Enter L2 Pass:", confirming successful Bluetooth authentication and transition to keypad entry.

The password sent from the Bluetooth terminal app (`1234#`) was correctly parsed by the UART1 interrupt handler, matched against the Level-1 password stored in the 24C256 EEPROM, and the state machine transitioned to Level-2 entry within the configured response time. This also confirms the `#` terminator logic is working correctly — the system waited for the full 4 digits plus terminator before attempting the comparison, rather than validating digit-by-digit.

**3. Level-2 keypad entry**

![Keypad password entry](documentation/03_keypad_password_entry.jpg)

The LCD shows "KEYPAD PWD:" while the second password is entered on the 4x4 matrix keypad.

Each key press on the 4x4 matrix (rows P1.16–P1.19, columns P1.20–P1.23) was correctly scanned and debounced, with the LCD updating in real time as digits were entered. This test also confirmed that the system only accepts keypad input after Level-1 has already succeeded — pressing keys before Bluetooth authentication has no effect, verifying the two-factor sequence cannot be bypassed by starting from the keypad.

**4. Access granted**

![Access granted](documentation/04_access_granted.jpg)

On correct Level-2 entry, the LCD displays "ACCESS GRANTED! Opening Locker..", and the motor drives to unlock the locker.

With both Level-1 and Level-2 passwords verified, the LPC2148 drove the L293D IN1/IN2 pins (P1.24/P1.25) to rotate the DC motor forward for the configured `MOTOR_ROTATE_MS` duration. The motor responded immediately after the second password match, with no noticeable delay between authentication success and motor activation, confirming the access-granted event and motor trigger are tightly coupled in the state machine.

**5. Locker open state**

![Locker open](documentation/05_locker_open.jpg)

The LCD confirms "Locker OPEN, Take your item", verifying the motor successfully opened the locker and the system tracks the open state correctly.

The system correctly held in the "open" state for the configured access period before initiating auto-close, and the LCD message remained stable throughout — confirming the state machine does not re-trigger the motor or re-prompt for passwords while the locker is already open. This also verified that the corresponding "locker open" event was logged over UART0 with an RTC timestamp, matching the entry visible in the UART event log.

All LCD prompts, Bluetooth Level-1 authentication, keypad Level-2 authentication, and motor-driven unlocking were verified working as designed on the Vector India LPC2148 development board.

---

## 🧗 Challenges Faced

- **Bluetooth pairing and baud rate mismatch** — Initially, the HC-05 module either failed to respond or returned garbled characters over UART1. This was traced to a mismatch between the module's configured baud rate and the LPC2148's UART1 initialization. Fixing both sides to a consistent 9600 baud resolved reliable Level-1 password reception.
- **Wiring congestion** — With this many peripherals (keypad, LCD, EEPROM, Bluetooth, motor driver, buzzer) all connected via jumper wires to a single development board, the dense wiring occasionally caused intermittent resets or incorrect readings from a loose or shorted connection. Reseating and re-routing wires more carefully, and double-checking each peripheral's connections individually before full integration, fixed these intermittent faults.

---

## 🛠️ Build & Flash

1. Open `locker_project.uvproj` in **Keil µVision**.
2. Confirm the LPC2148 target is selected, then build to generate the `.hex` file.
3. Flash the `.hex` to the LPC2148 using **Flash Magic** (board in ISP mode).
4. Wire up the hardware per the pin table above.
5. Open a UART0 terminal on the PC to view the live access log.
6. Pair your phone with the HC-05 module and send the Level-1 password from a Bluetooth terminal app.

---

## 📈 Future Enhancements

- Dedicated mobile app UI instead of a generic Bluetooth terminal
- Wi-Fi/cloud logging alongside UART0
- OLED/graphical display instead of 16×2 LCD
- RTC battery backup (VBAT) so time survives full power loss
- EEPROM wear-leveling for frequently written values

---

## 👤 Author

**Mohini Gomare**

Embedded Systems Project — LPC2148 ARM7 Microcontroller

### 🚀 Tech Stack

`Embedded C` · `ARM7TDMI-S (LPC2148)` · `Keil µVision` · `UART` · `I2C` · `GPIO Interrupts` · `RTC` · `HC-05 Bluetooth`

---
