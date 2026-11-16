# STM32F429 Gyroscope FreeRTOS Project

This project was developed as part of a university assignment. It demonstrates working with STM32F429 peripherals without using automatic code generation via CubeMX (peripheral initialization is written manually using registers and HAL).

## Hardware
* **Microcontroller:** STM32F429ZIT6 (Cortex-M4)
* **Development board:** STM32F429I-DISCOVERY
* **Sensor:** 3-axis gyroscope L3GD20 (connected via SPI5)
* **Communication interface:** USART1 (PA9/PA10) via external USB-UART

## Project Architecture
The project is divided into layers (following the OSI principle) to ensure modularity and clean code:
* **BSP (Board Support Package):** `bsp_gpio.c`, `bsp_uart.c`, `bsp_gyro.c` — low-level drivers for working with registers.
* **Application (Logic):** `freertos.c` — application business logic implemented as 6 independent FreeRTOS tasks.
* **Entry Point:** `main.c` — clock initialization, BSP calls, and scheduler startup.

## FreeRTOS Tasks (6 total)
1. **GyroTask** — polling the gyroscope via SPI (100 Hz).
2. **UartTxTask** — sending readings to the terminal.
3. **UartRxTask** — starting asynchronous UART reception.
4. **ButtonTask** — handling button presses (USER).
5. **ValidationTask** — parsing and validating user-entered commands.
6. **LedTask** — LED control.

## Synchronization Mechanisms
* **Message Queues:** `GyroQueue` (data transfer from the sensor) and `UartRxQueue` (string transfer from UART).
* **Mutex:** `SharedDataMutex` to protect the shared variables `current_mode` and `period_ms`.
* **Task Notifications:** for instantly waking up `ButtonTask` from the EXTI interrupt.

## Profiling (Percepio Tracealyzer)
The **TraceRecorder** library is integrated into the project to visualize the FreeRTOS scheduler's operation.
* **Recording mode:** Snapshot (Ring Buffer).
* **How to capture a dump:** The program is halted via the debugger (GDB/OpenOCD), the buffer contents are dumped to a `trace.bin` file, and it is opened in Percepio Tracealyzer.
* **What is visualized:** Task switching timeline (Gantt Chart), CPU load, queue and mutex events, as well as interrupt triggers.

## Functionality
* **Mode 1:** Red LED is on. The data output period can be set via UART (from 10 to 10000 ms).
* **Mode 2:** Green LED is on. X, Y, Z coordinates are continuously output to the terminal at the specified period.
* Mode switching is performed by a short press of the USER button.

## Build and Flash
The project is built using `arm-none-eabi-gcc` and supports two build systems: **CMake** (recommended) and **Makefile**.

### Option 1: CMake (Recommended)
The project includes a `CMakePresets.json` file. This allows seamless integration with **VS Code** and the **CMake Tools** extension. 
* It automatically applies the correct `arm-none-eabi.cmake` toolchain file.
* It sets the build directory to `build_cmake`.
* It resolves bare-metal compilation issues (like missing `_exit`).

To build via terminal:
```bash
cmake --preset arm-debug
cmake --build build_cmake -j4
```

(Or simply click the "Build" button in the VS Code CMake Tools status bar).

Note on Cross-Platform: The CMakePresets.json is currently configured for Linux/macOS using the Unix Makefiles generator. If you want to build this project on Windows, you will need to change the generator in CMakePresets.json to MinGW Makefiles or Ninja.

### Option 2: Makefile
If you prefer the traditional approach without CMake presets:
```bash
make -j4
```

### Flashing
To flash the compiled binary to the MCU using ST-Link:
```bash
st-flash --reset write build_cmake/GyroRTOS_native.bin 0x8000000
```
