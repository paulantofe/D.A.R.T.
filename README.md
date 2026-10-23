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
    ESP-->>Browser: HTTP 200 OK (multipart/x-mixed-replace)
    loop Continuous Live Feed
        ESP-->>Browser: Send MJPEG Frame
    end

    Note over User, DRV: PART 3: User Input & Command Routing
    User->>Browser: Press key(s) (e.g. 'W' or 'W' + 'A')
    alt Conflicting Input (e.g. 'W' + 'S')
        Browser->>Browser: JS detects conflict & ignores (No Action)
    else Valid Input (e.g. 'W' + 'A')
        Browser->>Browser: JS maps combination to a single char (e.g. 'W' or 'Q')
        Browser->>ESP: HTTP POST /control (payload: 'Q')
        ESP-->>Browser: HTTP 200 OK
        Note right of ESP: Other single or combined commands <br> follow this exact same flow
        ESP->>STM: UART TX: Send Char 'Q' 
    end

    Note over User, DRV: Part 4: Logic Processing and Hardware Actuation
    STM->>STM: Parse 'Q' & set Forward-Left state
    STM->>DRV: Generate PWM & IN1/IN2 Logic
    DRV-->>User: Rover executes forward-left movement
```

## State Machine Diagram
```mermaid
stateDiagram-v2
    [*] --> Initialization : Power On (Flip ON/OFF Switch)
    
    state Initialization {
        direction LR
        ESP_Boot(WiFi/FS) --> STM_Boot(UART/PWM)
    }

    Initialization --> Idle : Systems Ready & Connected
    Initialization --> System_Fault : Init Failed (e.g. FS files not found)

    Idle --> Moving : Valid Command Received (e.g. 'W')
    Moving --> Idle : Stop Command Received (e.g. Key Release)

    Moving --> Failsafe : UART Timeout or WiFi Lost
    Idle --> Failsafe : WiFi Lost

    Failsafe --> Idle : Connection Restored (WiFi or UART)
    System_Fault --> [*] : Hardware Reboot (Power Cycle)

    note left of System_Fault
        CRITICAL ERROR:
        1. Send error to Serial 
        2. Blink STM32 Red LED rapidly
        Requires manual Power Cycle.
    end note

    note right of Failsafe
        FAILSAFE MODE:
        1. All motors STOP (PWM 0)
        2. Turn ON Failsafe LED (STM32 Orange LED)
        Recovers to Idle when signal returns.
    end note
```
