# Integration Guide - ADC Uplink Module

This guide shows how to integrate the ADC Uplink module into the Meshtastic firmware.

## 📁 **File Structure**

After integration, your Meshtastic firmware should have:
```
src/modules/
├── adc_uplink/
│   ├── adc_uplink.h
│   └── adc_uplink.cpp
├── Modules.cpp      (modified)
├── Modules.h        (modified)
└── ...
```

## 🔧 **Integration Steps**

### **1. Copy Module Files**
```bash
cp -r adc_uplink /path/to/meshtastic/firmware/src/modules/
```

### **2. Register Module in Modules.h**

Add to `/src/modules/Modules.h`:
```cpp
#if defined(ARCH_ESP32) && HAS_WIFI
#include "modules/adc_uplink/adc_uplink.h"
#endif
```

### **3. Initialize Module in Modules.cpp**

Add to `/src/modules/Modules.cpp`:
```cpp
#if defined(ARCH_ESP32) && HAS_WIFI
    // ADC Uplink Module for Heltec V3
    adcUplinkModule = new ADCUplinkModule();
    modules.push_back(adcUplinkModule);
#endif
```

### **4. Update platformio.ini**

Ensure ArduinoJson dependency:
```ini
lib_deps = 
    ...
    bblanchon/ArduinoJson@^6.21.4
```

Set default environment for Heltec V3:
```ini
default_envs = heltec-v3
```

### **5. Build and Upload**

```bash
pio run -e heltec-v3 -t upload
```

## 🎯 **Hardware Configuration**

The module is configured for:
- **Target**: Heltec WiFi LoRa 32 V3 (ESP32-S3)
- **ADC Pin**: GPIO2 (ADC1_CH1)
- **Resolution**: 12-bit (0-4095)
- **Voltage Range**: 0-3.3V with 11dB attenuation

## 📊 **Module Parameters**

Default configuration in `adc_uplink.h`:
```cpp
#define ADC_PIN 2           // GPIO2 for analog input
#define ADC_BITS 12         // 12-bit resolution
#define VREF_VOLTS 3.3f     // Reference voltage
#define PERIOD_MS 10000     // 10-second intervals
#define SAMPLE_COUNT 8      // Average 8 readings
#define HOPS 3              // Maximum mesh hops
```

## 🔍 **Verification**

After successful integration:
1. Serial output shows: `ADC Uplink Module started - reading from GPIO2`
2. Device ID displayed: `Device ID: ABCD`
3. JSON sensor data transmitted: `{"id":"ABCD","pin":2,"raw":1234,"V":1.23}`
4. Data appears on other nodes' OLED displays

## 🐛 **Troubleshooting**

### **Build Errors**
- Ensure ArduinoJson is in dependencies
- Check that all files are in correct locations
- Verify ESP32 architecture flags

### **Runtime Issues**
- Module only runs on ESP32 platforms
- GPIO2 must be available (not used by other modules)
- Check serial output for error messages

### **Mesh Issues**
- Verify LoRa connectivity between nodes
- Check hop limit settings
- Ensure all nodes have same firmware version

## 📋 **Required Changes Summary**

1. **New files**: `adc_uplink.h`, `adc_uplink.cpp`
2. **Modified**: `Modules.h`, `Modules.cpp`, `platformio.ini`
3. **Dependencies**: ArduinoJson library
4. **Target**: ESP32/Heltec V3 only

This integration maintains compatibility with existing Meshtastic features while adding distributed sensor capabilities.
