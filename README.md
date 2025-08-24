# ESP32 PiKVM Controller

This project uses an **ESP32-DevKit** running native **ESP-IDF (bare-metal C with FreeRTOS)** to talk to a [PiKVM](https://pikvm.org/) over its HTTP API.  
The ESP32 connects to Wi-Fi, authenticates to PiKVM, and can send **power on/off** requests — now also controlled by a **physical toggle switch** with a **status LED indicator**.

And yes, it’s all **baremetal C**; because we don’t take shortcuts with Arduino here. 

---

## Functionality (so far)

- **Wi-Fi STA connection** using ESP-IDF APIs
- **Credentials pulled from `menuconfig`** (no hardcoding in source)
- **PiKVM integration**:
  - `pikvm_power_on(host, user, pass)`
  - `pikvm_power_off(host, user, pass)`
  - `pikvm_get_atx_state(host, user, pass, *is_on)` → confirm actual ATX power state
- **Hardware switch integration**:
  - Toggle switch flips PiKVM power ON/OFF
  - Status LED lights up **only after ATX confirms ON**, and turns off **only after ATX confirms OFF**
- **Modular code structure** (separated into `wifi.c`, `pikvm.c`, `control.c`)
---

## Project structure
```text
src/
  main.c            # entrypoint, init Wi-Fi and start control task
  wifi.c/h          # Wi-Fi station setup + connect
  pikvm.c/h         # PiKVM API wrappers (power on/off, ATX state)
  control.c/h       # Hardware switch & LED control logic
  Kconfig.projbuild # defines App Config menu for secrets
```
---

## Setup

### 1. Install toolchain
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
- Or if you’re using **PlatformIO**: 
```bash
pio run
pio run -t upload 
pio run -t menuconfig
```
### 2. Configure your secrets
This project defines its own `App Config` section in `menuconfig`.

Run:
```bash
pio run -t menuconfig