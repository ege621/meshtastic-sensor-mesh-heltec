# Meshtastic Firmware Architecture Overview

## 🏗️ **High-Level Software Pipeline**

```
Hardware Layer → Platform Abstraction → Core Services → Modules → User Interface
```

## 📂 **Main Directory Structure**

### **Core Architecture**
```
src/
├── main.cpp                 # Entry point, hardware initialization
├── configuration.h          # Global compile-time settings
├── MeshService.h/cpp        # Central mesh networking service
├── NodeDB.h/cpp            # Device/node database management
└── PowerFSM.h/cpp          # Power state machine
```

### **Platform Abstraction Layer**
```
src/platform/
├── esp32/                  # ESP32-specific code (GPIO, timers, etc.)
├── nrf52/                  # Nordic nRF52 specific
├── portduino/              # Linux/simulation platform
├── rp2xx0/                 # Raspberry Pi Pico
└── stm32/                  # STM32 microcontrollers
```

### **Hardware Variants**
```
variants/
├── esp32/
│   ├── heltec_v3/          # Your board configuration!
│   │   ├── platformio.ini  # Build settings, pins, flags
│   │   └── variant.h       # Pin definitions, hardware config
│   └── ttgo_t_beam/        # Other ESP32 boards
├── esp32s3/
└── nrf52840/
```

### **Mesh Networking Core**
```
src/mesh/
├── Router.h/cpp            # Packet routing logic
├── MeshService.h/cpp       # Main mesh service
├── RadioInterface.h/cpp    # LoRa radio abstraction
├── SX126xInterface.cpp     # Your radio chip driver
├── PacketHistory.cpp       # Duplicate detection
├── FloodingRouter.cpp      # Mesh flooding algorithm
└── generated/              # Protocol buffer definitions
    └── meshtastic/         # Auto-generated from .proto files
```

## 🧩 **Module System Architecture**

### **Module Types**
```
src/modules/
├── Modules.h/cpp           # Module registration system
├── AdminModule.cpp         # Device administration
├── PositionModule.cpp      # GPS location sharing
├── TextMessageModule.cpp   # Text messaging
├── adc_uplink/            # Your sensor module!
│   ├── adc_uplink.h       # Module interface
│   └── adc_uplink.cpp     # Implementation
└── Telemetry/             # Sensor telemetry modules
    ├── DeviceTelemetry.cpp # Battery, system stats
    └── EnvironmentTelemetry.cpp # Environmental sensors
```

### **Module Base Classes**
- `MeshModule` - Base for all modules
- `SinglePortModule` - Modules using one protocol buffer port
- `ProtobufModule` - Structured data modules
- `OSThread` - Threaded execution

## 🔄 **Software Execution Flow**

### **1. Boot Sequence** (`main.cpp`)
```cpp
setup() {
    // Hardware initialization
    initializeHardware();
    
    // Platform-specific setup
    platformSetup();
    
    // Core services
    nodeDB = new NodeDB();
    service = new MeshService();
    
    // Module registration
    setupModules();  // ← Your ADC module registered here
    
    // Start mesh networking
    startMeshService();
}
```

### **2. Main Loop**
```cpp
loop() {
    // Handle radio packets
    service->loop();
    
    // Process module tasks
    for (auto module : modules) {
        module->runOnce();  // ← Your ADC reading happens here
    }
    
    // Power management
    powerFSM.run();
    
    // UI updates
    screen->loop();
}
```

### **3. Module Execution** (Your ADC Module)
```cpp
ADCUplinkModule::runOnce() {
    // Read sensor → Create JSON → Send to mesh
    return 10000; // Sleep 10 seconds
}
```

## 📡 **Mesh Networking Pipeline**

### **Outgoing Messages** (Your sensor data)
```
Your Module → MeshService → Router → RadioInterface → LoRa Radio → Air
```

### **Incoming Messages**
```
LoRa Radio → RadioInterface → Router → MeshService → Target Module
```

### **Packet Structure**
```
MeshPacket {
    from: NodeID,
    to: NodeID,
    hop_limit: 3,
    decoded: {
        portnum: TEXT_MESSAGE_APP,  // Your module's port
        payload: "{\"id\":\"A1B2\",\"raw\":1234}"  // Your JSON
    }
}
```

## 🛠️ **Build System (PlatformIO)**

### **Configuration Hierarchy**
```
platformio.ini              # Global settings, libraries
├── arch/esp32/esp32.ini    # ESP32-specific settings
└── variants/esp32s3/heltec_v3/platformio.ini  # Board-specific
```

### **Compilation Flags**
- `HELTEC_V3` - Enables your board variant
- `ARCH_ESP32` - Platform selection
- Library dependencies automatically resolved

## 🔧 **Key Concepts**

### **Protocol Buffers**
- Structured data format for mesh communication
- Definitions in `protobufs/*.proto`
- Auto-generated C++ in `src/mesh/generated/`

### **Thread System**
- Each module runs in its own thread (`OSThread`)
- Cooperative multitasking with `runOnce()` pattern
- Return value = sleep time until next execution

### **Port Numbers**
- Each module communicates on a specific "port"
- `TEXT_MESSAGE_APP` = user-visible text messages
- `TELEMETRY_APP` = sensor data
- Custom ports for specialized modules

### **Power Management**
- `PowerFSM` handles sleep/wake cycles
- Modules can influence power state
- Battery monitoring integrated

## 📋 **Your ADC Module in Context**

```
Hardware: Heltec V3 GPIO2 → ADC reading
   ↓
Platform: ESP32-S3 ADC driver
   ↓
Module: ADCUplinkModule::runOnce()
   ↓
JSON: {"id":"A1B2","pin":2,"raw":1234,"V":1.23}
   ↓
MeshService: Packet creation and routing
   ↓
RadioInterface: SX1262 LoRa transmission
   ↓
Air: 915MHz LoRa signal
   ↓
Other nodes: Receive and display/forward
```

## 🎯 **Development Workflow**

1. **Hardware Definition** - `variants/esp32s3/heltec_v3/variant.h`
2. **Module Creation** - `src/modules/your_module/`
3. **Module Registration** - `src/modules/Modules.cpp`
4. **Build Configuration** - `platformio.ini` libraries
5. **Compilation** - `pio run -e heltec-v3`
6. **Flash** - `pio run -e heltec-v3 -t upload`

This architecture makes Meshtastic incredibly flexible - you can add new hardware platforms, radio types, sensors, and communication protocols by following these established patterns!
