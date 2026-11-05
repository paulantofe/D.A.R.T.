# Copilot instructions

## Build and validation

Run commands from the repository root.

- Build the ESP32-CAM firmware: `pio run -d esp32_network_controller -e esp32cam`
- Upload its LittleFS web assets: `pio run -d esp32_network_controller -e esp32cam -t uploadfs`
- Upload the firmware: `pio run -d esp32_network_controller -e esp32cam -t upload`
- Run PlatformIO tests: `pio test -d esp32_network_controller -e esp32cam`
- Run one PlatformIO test suite: `pio test -d esp32_network_controller -e esp32cam -f <test_suite_name>`

The PlatformIO `test` directory currently contains no test suites. There is no configured lint command.

The STM32 project is an STM32CubeIDE project with Debug and Release configurations. Build it in STM32CubeIDE after importing `stm32_motor_controller`; its headless build application can also be used with an imported workspace:

```text
STM32CubeIDE.exe -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild -data <workspace-path> -build stm32_motor_controller/Debug
```

There are no checked-in automated STM32 tests.

## Architecture

- The repository contains two firmware targets: `esp32_network_controller` (PlatformIO/Arduino on ESP32-CAM) and `stm32_motor_controller` (STM32CubeIDE/HAL on STM32F407). The intended split puts Wi-Fi, camera streaming, and the browser server on the ESP32, with motor and sensor hardware control on the STM32.
- ESP32 startup and recurring UART/network monitoring are in `esp32_network_controller/src/main.cpp`. Camera pin mapping and initialization are in `src/camera.cpp`; HTTP routes, Wi-Fi setup, mDNS (`dart.local`), and the asynchronous MJPEG stream are in `src/server.cpp`.
- The browser dashboard is served from `esp32_network_controller/data/` through LittleFS. `index.html`, `style.css`, and `script.js` are wired to matching routes in `server.cpp`. The JavaScript maps keyboard and button input to single-character control values and polls `/status`; `/control` forwards the first submitted character over UART.
- STM32 startup enters through `Core/Src/main.c` and `STM_Boot()` in `Core/Src/stm32f4xx_hal_hardware_config.c`. That module owns peripheral handles and high-level peripheral configuration. HAL MSP callbacks in `Core/Src/stm32f4xx_hal_msp.c` handle low-level peripheral clock, GPIO, and IRQ setup. CMSIS and STM32 HAL sources are vendored under `Drivers/`.
- The intended firmware boundary is a byte-oriented UART link. Keep message values, baud rate, timing, and pin assignments consistent at both ends. In the current code, STM32 boot transmits `R` and blocks until it receives `K`, but the ESP32 has no handler that sends `K`; the motor-control/runtime protocol is also not implemented on the STM32 side. Check both endpoints before changing or relying on this protocol.

## Repository-specific conventions

- Format commit messages as `git commit -m "<type>: <description>"`.
- ESP32 modules expose their public declarations in `include/` and implementations in `src/`. Initialization APIs use `*_Init` names and return `esp_err_t`; boot treats camera/server initialization failures as fatal.
- STM32 peripheral setup functions use `*_Config` names and route HAL initialization failures to `Error_Handler()`. The public hardware setup API and shared peripheral handles are declared in `Core/Inc/stm32f4xx_hal_hardware_config.h`.
- **CRITICAL:** Do NOT use or suggest using STM32CubeMX for code generation or hardware setup. The `stm32_motor_controller.ioc` file was used strictly to bootstrap/vendor the STM32 HAL drivers into the `Drivers/` folder and should be ignored. High-level peripheral configurations are written manually in `Core/Src/stm32f4xx_hal_hardware_config.c`; low-level peripheral clock, GPIO, and IRQ setup belongs in the HAL MSP callbacks in `Core/Src/stm32f4xx_hal_msp.c`. Do not rely on, suggest, or use standard CubeMX `/* USER CODE BEGIN */` block structures.
- Keep browser asset filenames and URL paths aligned between `data/` and the explicit static routes in `src/server.cpp`. The ESP32 build uses LittleFS, as set in `platformio.ini`.
- ESP32 state shared between modules is declared `extern` in `include/globals.h` and defined in `src/main.cpp`; preserve the existing ownership and timeout semantics when changing client or failsafe handling.
