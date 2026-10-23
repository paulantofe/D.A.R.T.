# WiFi_Rover

The *WiFi Rover* is a **Dual-MCU robotic 2WD vehicle** designed for real-time teleoperation. It bridges the gap between embedded hardware engineering and web technologies, providing a **video-game-like experience** through live first-person view streaming and low-latency WASD keyboard controls directly from a web browser.

## Project Goal

Originally conceived as a university project, this repository represents a pursuit of professional engineering standards. The main objective is to **design**, **document** and **implement** a robust, production-grade, prototype by focusing on:
* **Advanced Embedded Architecture**: Implementing a Master-Slave dual-microcontroller system (ESP32-CAM and STM32F407G-DISC1) to decouple high-bandwidth networks from deterministic hardware control.
* **Mechanical & CAD Design**: Developing a fully custom 3D-printed chassis and body work using Fusion 360.
* **Software Best Practices**: Writing modular C/C++ firmware and JavaScript web clients, complemented by UML documentation and proper Git version control.

## Architecture Diagram
```mermaid
flowchart LR
    BAT[7.4V Li-ion 18650]-->|7.4V|BUCK[5V Regulator LM2596]
    BUCK-->|5V|ESP[ESP32-CAM]
    BUCK-->|5V|STM[STM32F407G-DISC1]
    BAT-->|7.4V|DRV[Motor Driver L298N]

    DRV===>|High Current|M1((DC Motor 1))
    DRV===>|High Current|M2((DC Motor 2))

    ESP-.->|UART Communication|STM
    STM-.->|PWM|DRV
```

## Sequence Diagram
```mermaid
sequenceDiagram
    autonumber
    actor User
    participant Browser as Web Interface
    participant Router as Local Network
    participant ESP as ESP32-CAM
    participant FS as LittleFS
    participant STM as STM32F407G-DISC1
    participant DRV as Motor Driver L298N

    Note over User, DRV: Part 1: Network Initialization
    ESP->>Router: Attempt connection to saved SSID
    alt Connection Failed
        ESP->>User: Open "WiFiManager" Access Point
        User->>ESP: Provide new SSID and Password
    end
    Router-->>ESP: DHCP Assign IP Address
    Note right of ESP: ESP32 prints IP <br> to Serial Monitor

    Note over User, DRV: Part 2: Loading UI & Video Stream
    User->>Browser: Access ESP32 IP Address
    Browser->>ESP: HTTP GET /
    ESP->>FS: Request HTML/CSS/JS
    FS-->>ESP: Return static files
    ESP-->>Browser: HTTP 200 OK (Render UI)
    Browser->>ESP: HTTP GET /stream 
    loop Continuous Live Feed
        ESP-->>Browser: Send MJPEG Frame
    end

    Note over User, DRV: PART 3: User Input & Command Routing (Example: Forward)
    User->>Browser: Press 'W' Key
    Browser->>ESP: HTTP POST /control (payload: 'W')
    ESP-->>Browser: HTTP 200 OK
    Note right of ESP: Other commands follow <br> the exact same flow
    ESP->>STM: UART TX: Send Char 'W' 

    Note over User, DRV: Part 4: Logic Processing and Hardware Actuation
    STM->>STM: Parse 'W' & set Forward state
    STM->>DRV: Generate PWM & IN1/IN2 Logic
    DRV-->>User: Rover executes forward movement

```
