# ESP32 PiKVM Controller

This project uses an **ESP32-DevKit** running native **ESP-IDF (bare-metal C with FreeRTOS)** to talk to a [PiKVM](https://pikvm.org/) over its HTTP API.  
The ESP32 connects to Wi-Fi, authenticates to PiKVM, and can send **power on/off** requests.

It’s all **baremetal C** because we don’t take shortcuts with Arduino here. 

---

## Functionality (so far)

- **Wi-Fi STA connection** using ESP-IDF APIs
- **Credentials pulled from `menuconfig`** (no hardcoding in source)
- **PiKVM integration**:
    - `pikvm_power_on(host, user, pass)`
    - `pikvm_power_off(host, user, pass)`

The ESP32 confirms success/failure over serial logs.

---

## Project structure
```angular2html
src/
main.c              # entrypoint, calls Wi-Fi + PiKVM functions
wifi.c/h            # Wi-Fi station setup + connect
pikvm.c/h           # PiKVM API wrappers
Kconfig.projbuild   # defines App Config menu for secrets
```
---

## Setup

### 1. Install toolchain
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
- Or if you’re using **PlatformIO**: `pio run`, `pio run -t upload`, `pio run -t menuconfig` work directly.

### 2. Configure your secrets
This project defines its own `App Config` section in `menuconfig`.

Run:
```bash
pio run -t menuconfig