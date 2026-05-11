# SEN66 Air Quality Sensor Integration Summary

## Overview
Successfully integrated the Sensirion SEN66 all-in-one air quality sensor into the Meshtastic firmware with automatic sensor detection and prioritization.

## Implementation Details

### 1. SEN66 Driver (`sen66_driver.h` & `sen66_driver.cpp`)
Created a custom I2C driver for the SEN66 sensor that implements:

**Features:**
- Full I2C communication with CRC-8 validation
- Measurement of 8 environmental parameters:
  - PM1.0, PM2.5, PM4.0, PM10.0 (particulate matter in μg/m³)
  - Temperature (°C)
  - Humidity (%)
  - VOC Index (0-500)
  - NOx Index (0-500)

**Key Functions:**
- `begin()` - Initialize sensor and verify connection
- `startMeasurement()` - Start continuous measurement mode
- `readMeasuredValues()` - Read all sensor data with CRC validation
- `reset()` - Reset sensor to known state

**I2C Address:** 0x6B (fixed)

### 2. Updated ADC Uplink Module (`adc_uplink.h` & `adc_uplink.cpp`)

**Sensor Detection Logic:**
1. First attempts to initialize SEN66 (priority sensor)
2. If SEN66 not found, falls back to SHT31
3. If neither sensor is found, logs error and disables module

**Features:**
- Automatic sensor detection on startup
- Real-time sensor data broadcasting every 30 seconds
- OLED display support showing:
  - Local sensor readings
  - Remote node data from mesh network
  - Air quality parameters (when SEN66 is active)
- Mesh message parsing for both simple T/H data and full air quality data

**Sensor Priority:**
```
SEN66 (Air Quality) > SHT31 (Temperature/Humidity) > NONE
```

### 3. Data Structure Enhancements

**SensorType Enum:**
```cpp
enum class SensorType {
    NONE = 0,
    SHT31 = 1,
    SEN66 = 2
};
```

**RemoteSensorData Structure:**
Extended to support both basic and air quality data:
- Temperature & Humidity (all sensors)
- PM1.0, PM2.5, PM4.0, PM10.0 (SEN66 only)
- VOC Index & NOx Index (SEN66 only)

### 4. Mesh Broadcasting

**SEN66 Format:**
```
🌡️ Air Quality [XXXX]
Temp: XX.X°C
Humidity: XX.X%
PM1.0: XX.X μg/m³
PM2.5: XX.X μg/m³
PM4.0: XX.X μg/m³
PM10: XX.X μg/m³
VOC: XXX
NOx: XXX
```

**SHT31 Format:**
```
🌡️ Sensor [XXXX]
Temp: XX.X°C
Humidity: XX.X%
```

### 5. OLED Display Updates
- Dynamic title based on active sensor ("Air Quality" or "Sensors")
- Compact display showing most critical parameters
- Support for displaying remote node data
- Timestamp showing seconds since last reading

## Hardware Configuration

**I2C Bus (Heltec V3):**
- SDA: GPIO17
- SCL: GPIO18

**Sensor Addresses:**
- SHT31: 0x44
- SEN66: 0x6B

## Timing Configuration
- Sensor reading interval: 1 second (for display updates)
- Mesh broadcast interval: 30 seconds
- Mesh hop limit: 3 hops

## Log Messages

The module provides comprehensive logging:

**Success:**
```
SEN66 air quality sensor detected and initialized
Device ID: XXXX | Active Sensor: SEN66 (Air Quality)
```

**Fallback:**
```
SEN66 sensor not found or initialization failed
SHT31 temperature/humidity sensor detected and initialized
Device ID: XXXX | Active Sensor: SHT31 (Temperature/Humidity)
```

**Error:**
```
No supported sensors found on I2C bus
No sensors available. Module disabled.
```

## Building and Flashing

1. **Build the firmware:**
   ```bash
   cd /Users/egekeskin/Desktop/firmware
   pio run -e heltec-v3
   ```

2. **Flash to device:**
   ```bash
   pio run -e heltec-v3 -t upload
   ```

3. **Monitor serial output:**
   ```bash
   pio device monitor
   ```

## Testing

1. **With SEN66 only:** Module will use SEN66 and broadcast full air quality data
2. **With SHT31 only:** Module will use SHT31 and broadcast temperature/humidity
3. **With both sensors:** Module will prioritize SEN66 (SHT31 will be ignored)
4. **With no sensors:** Module will log error and disable itself

## Files Modified/Created

**New Files:**
- `src/modules/adc_uplink/sen66_driver.h` - SEN66 driver header
- `src/modules/adc_uplink/sen66_driver.cpp` - SEN66 driver implementation

**Modified Files:**
- `src/modules/adc_uplink/adc_uplink.h` - Added SEN66 support and air quality fields
- `src/modules/adc_uplink/adc_uplink.cpp` - Implemented sensor prioritization logic

**No Changes Required:**
- `platformio.ini` - Adafruit SHT31 library already included; custom SEN66 driver doesn't need external library

## Notes

- The SEN66 driver implements the complete Sensirion I2C protocol including CRC-8 validation
- All measurements are calibrated according to the SEN66 datasheet specifications
- The module is designed for ESP32 platforms only
- Remote sensor data is stored with timestamps to show data freshness
- The implementation supports mesh networks with multiple sensor nodes

## Troubleshooting

**If SEN66 is not detected:**
1. Verify I2C connections (SDA to GPIO17, SCL to GPIO18)
2. Check sensor power supply (3.3V)
3. Verify sensor address (should be 0x6B)
4. Check serial monitor for initialization messages

**If module shows "No Sensor Found":**
1. Check both sensor I2C addresses
2. Verify I2C bus is functioning (run I2C scanner)
3. Check for loose connections
4. Ensure sensors are powered correctly

## Future Enhancements

Possible improvements:
- Add sensor calibration options
- Implement data logging to SD card
- Add configurable broadcast intervals
- Support for additional Sensirion sensors (SEN5x family)
- Air quality index (AQI) calculation from PM2.5 values
