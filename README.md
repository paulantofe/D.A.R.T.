# D.A.R.T. Rover
**Dual Architecture Rover Technology**

Current Project Status: *in progress*

The *D.A.R.T. Rover* is a **Dual-MCU robotic 2WD vehicle** designed for real-time teleoperation. It bridges the gap between embedded hardware engineering and web technologies, providing a **video-game-like experience** through live first-person view streaming and low-latency WASD keyboard controls directly from a web browser.

## Project Goal

Originally conceived as a university project, this repository represents a pursuit of professional engineering standards. The main objective is to **design**, **document** and **implement** a robust, production-grade, prototype by focusing on:
* **Advanced Embedded Architecture:** Implementing a Master-Slave dual-microcontroller system (ESP32-CAM and STM32F407G-DISC1) to decouple high-bandwidth networks from deterministic hardware control.
* **Mechanical & CAD Design:** Developing a fully custom 3D-printed chassis and body work using Fusion 360.
* **Software Best Practices:** Writing modular C/C++ firmware and JavaScript web clients, complemented by UML documentation and proper Git version control.

## Key Features

* **Dual-MCU Architecture:** ESP32-CAM (Network & Video) + STM32F407G-DISC1 (Real-time Motor Control) providing minimal latency and fine control over speed.
* **Defensive Web Client:** JS-level input filtering for conflicting commands (e.g. 'W' + 'S').
* **Heartbeat Failsafe:** Real-time connection monitoring that forces motors to halt upon signal loss.
* **Security**: Only one driver at the time is permitted and it's automatically disconnected after 2 seconds of no status request (no GET /status performed by browser)
* **mDNS Discovery:** Plug-and-play network access via `http://rover.local`. eliminating the need for hardcoded IPs or Serial Monitor debugging.
* **Magnetic Quick-Release Bodywork:** Custom 3D-printed shell secured with neodymium magnets for instant hardware access and LED diagnostics

## How to Run

*(in progress)* 

## Hardware Wiring

*(in progress)*

## Architecture Diagram
```mermaid
flowchart LR
    BAT[7.4V Li-ion 18650]-->|7.4V|BUCK[5V Regulator LM2596]
    BUCK-->|5V|ESP[ESP32-CAM <br> Web Server]
    BUCK-->|5V|STM[STM32F407G-DISC1 <br> PID Motor Controller]
    BAT-->|7.4V|DRV[Motor Driver L298N]

    DRV==>|High Current|M1((DC Motor 1))
    DRV==>|High Current|M2((DC Motor 2))

    ESP-.->|UART: Target Speed|STM
    STM-.->|PWM & Dir|DRV

    M1-.->|Mechanical Rotation|E1[Encoder 1]
    E1-.->|Pulses Ch A/B <br> PID Feedback|STM
    M2-.->|Mechanical Rotation|E2[Encoder 2]
    E2-.->|Pulses Ch A/B <br> PID Feedback|STM

    STM-->|3.3V|E1
    STM-->|3.3V|E2
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

    Note over User, DRV: Part 0: System Boot & Hardware Check
    ESP->>ESP: Run ESP_Boot() (Init Camera & Server)
    alt Initialization Failed
        ESP->>STM: UART TX: Send System Fault ('X')
        STM->>STM: Trigger System_Fault (Red LED ON, PWM 0)
        STM->>DRV: Halt Motors
        ESP->>ESP: Freeze Execution (Error_Handler)
    end

    Note over User, DRV: Part 1: Network Initialization
    ESP->>Router: Attempt connection to saved SSID
    alt Connection Failed
        ESP->>ESP: Start Access Point Mode (Broadcast SSID)
        User->>ESP: Connect to AP via Phone/PC 
        User->>ESP: Provide new SSID and Password
        ESP->>Router: Connect using new credentials
    end
    Router-->>ESP: DHCP Assign IP Address
    ESP->>Router: Broadcast mDNS Name ("dart.local")
    Note right of ESP: No Serial Monitor <br> needed for IP discovery.

    Note over User, DRV: Part 2: Loading UI & Video Stream
    User->>Browser: Access http://rover.local
    Browser->>ESP: mDNS Resolve IP & HTTP GET /
    ESP->>FS: Request HTML/CSS/JS
    FS-->>ESP: Return static files
    ESP-->>Browser: HTTP 200 OK (Render UI)
    Browser->>ESP: HTTP GET /stream 
    ESP-->>Browser: HTTP 200 OK (multipart/x-mixed-replace)
    loop Continuous Live Feed
        ESP-->>Browser: Send MJPEG Frame
    end

    Note over User, DRV: Part 3: User Input & Command Routing
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

    loop WiFi Heartbeat Status & Failsafe (Every 2000ms)
        Browser->>ESP: HTTP GET /status (Browser Heartbeat)
        ESP-->>Browser: HTTP 200 JSON (Battery, WiFi & Mode info)
        
        alt Browser Disconnected (>2500ms without HTTP GET)
            ESP->>STM: UART TX: Send Failsafe ('F')
            STM->>STM: Trigger Network Failsafe (PWM 0, Orange LED ON)
            STM->>DRV: Halt Motors
            DRV-->>User: Rover safely stops
        end
    end
    loop UART Heartbeat & Failsafe (Every 200ms)
        ESP->>STM: UART TX: Heartbeat Signal ('H')

        alt ESP-STM UART Broken (>500ms without 'H')
            STM->>STM: Trigger Hardware Failsafe (PWM 0, Orange LED ON)
            STM->>DRV: Halt Motors
            DRV-->>User: Rover safely stops
        end
    end
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
    Initialization --> System_Fault : Init Failed (Camera/Server Error)

    Idle --> Moving : Valid Command Received (e.g. 'W')
    Moving --> Idle : Stop Command Received (e.g. Key Release)

    Moving --> Failsafe : Heartbeat Timeout (>500ms UART <br> >2500ms WiFi)
    Idle --> Failsafe : Heartbeat Timeout (>500ms UART <br> >2500ms WiFi)

    Failsafe --> Idle : Heartbeat Restored <br> (WiFi & UART OK)
    System_Fault --> [*] : Hardware Reboot (Power Cycle)

    note left of System_Fault
        CRITICAL ERROR:
        1. Send 'X' to STM32
        2. Freeze ESP32 Execution
        3. Turn ON STM32 Red LED
        Requires manual Power Cycle.
    end note

    note right of Failsafe
        FAILSAFE MODE:
        1. All motors STOP (PWM 0)
        2. Turn ON Failsafe LED (STM32 Orange LED)
        Recovers to Idle ONLY when Heartbeat 
        confirms both WiFi and UART are active.
    end note
```
