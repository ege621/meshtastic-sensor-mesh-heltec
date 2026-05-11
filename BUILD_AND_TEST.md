# Building and Testing SEN66 Integration

## Prerequisites

- PlatformIO installed
- Heltec V3 board connected via USB
- SEN66 and/or SHT31 sensor connected via I2C

## Build Instructions

### 1. Navigate to Firmware Directory
```bash
cd /Users/egekeskin/Desktop/firmware
```

### 2. Build the Firmware
```bash
# Build for Heltec V3
pio run -e heltec-v3

# Or use the build task if available
pio run -t build
```

### 3. Upload to Device
```bash
# Flash the firmware
pio run -e heltec-v3 -t upload

# Alternative: Use upload task
pio run -t upload
```

### 4. Monitor Serial Output
```bash
# Start serial monitor
pio device monitor

# Or with specific baud rate
pio device monitor -b 115200
```

## Expected Serial Output

### Successful SEN66 Detection
```
[INFO] Environmental Sensor Module started
[INFO] Initializing I2C sensors...
[INFO] Attempting to initialize SEN66 sensor on I2C address 0x6B
[INFO] SEN66 sensor initialized successfully
[INFO] SEN66 air quality sensor detected and initialized
[INFO] Device ID: A1B2 | Active Sensor: SEN66 (Air Quality)
[INFO] Sent sensor data: 🌡️ Air Quality [A1B2]
Temp: 23.5°C
Humidity: 45.0%
PM1.0: 5.2 μg/m³
PM2.5: 8.1 μg/m³
PM4.0: 9.3 μg/m³
PM10: 10.5 μg/m³
VOC: 120
NOx: 105
```

### Fallback to SHT31
```
[INFO] Environmental Sensor Module started
[INFO] Initializing I2C sensors...
[INFO] Attempting to initialize SEN66 sensor on I2C address 0x6B
[WARN] SEN66 sensor not found or initialization failed
[INFO] Attempting to initialize SHT31 sensor on I2C address 0x44
[INFO] SHT31 sensor initialized successfully
[INFO] SHT31 temperature/humidity sensor detected and initialized
[INFO] Device ID: A1B2 | Active Sensor: SHT31 (Temperature/Humidity)
[INFO] Sent sensor data: 🌡️ Sensor [A1B2]
Temp: 23.5°C
Humidity: 45.0%
```

### No Sensors Found
```
[INFO] Environmental Sensor Module started
[INFO] Initializing I2C sensors...
[INFO] Attempting to initialize SEN66 sensor on I2C address 0x6B
[WARN] SEN66 sensor not found or initialization failed
[INFO] Attempting to initialize SHT31 sensor on I2C address 0x44
[WARN] SHT31 sensor not found at address 0x44
[ERROR] No supported sensors found on I2C bus
[ERROR] No sensors available. Module disabled.
```

## Testing Scenarios

### Test 1: SEN66 Only
**Setup:**
- Connect SEN66 to I2C bus (GPIO17/18)
- Disconnect or remove SHT31

**Expected:**
- Module detects SEN66
- Broadcasts full air quality data every 30 seconds
- OLED shows "Air Quality" with PM2.5 and PM10 readings

**Verify:**
```bash
# Monitor and look for SEN66 initialization
pio device monitor | grep -i "sen66"
```

### Test 2: SHT31 Only
**Setup:**
- Connect SHT31 to I2C bus
- Disconnect or remove SEN66

**Expected:**
- Module detects SHT31 (after SEN66 detection fails)
- Broadcasts temperature/humidity only every 30 seconds
- OLED shows "Sensors" with T/H data

**Verify:**
```bash
# Monitor and look for SHT31 initialization
pio device monitor | grep -i "sht31"
```

### Test 3: Both Sensors Connected
**Setup:**
- Connect both SEN66 and SHT31 to I2C bus

**Expected:**
- Module detects SEN66 first (priority)
- SHT31 is ignored
- Broadcasts air quality data from SEN66 only

**Verify:**
```bash
# Should see SEN66 detected, no SHT31 attempt
pio device monitor | grep -i "active sensor"
```

### Test 4: No Sensors
**Setup:**
- Disconnect all sensors from I2C bus

**Expected:**
- Module attempts to detect both sensors
- Both detections fail
- Module logs error and disables itself

**Verify:**
```bash
# Should see "No sensors available"
pio device monitor | grep -i "no sensor"
```

### Test 5: Mesh Network Broadcasting
**Setup:**
- Two or more Meshtastic devices with sensor modules
- At least one with SEN66, one with SHT31

**Expected:**
- Each device broadcasts its sensor data
- Remote sensor data appears on OLED display
- Each node shows data from other nodes with timestamps

**Verify:**
- Check OLED display for "--- Other Nodes ---" section
- Verify mesh messages in Meshtastic app
- Look for "Received sensor data" in serial monitor

## I2C Bus Verification

If sensors are not detected, verify I2C bus:

```bash
# Run I2C scanner (if available)
pio device monitor

# Look for I2C addresses:
# 0x44 = SHT31
# 0x6B = SEN66
```

## Common Build Issues

### Issue: Missing Libraries
**Error:** `fatal error: Adafruit_SHT31.h: No such file or directory`

**Solution:**
```bash
# Clean and rebuild
pio run -t clean
pio run -e heltec-v3
```

### Issue: Upload Failed
**Error:** `Could not open port`

**Solution:**
1. Unplug and replug USB cable
2. Check device permissions (Linux)
3. Close other serial monitors
4. Try different USB port

### Issue: Module Not Starting
**Symptoms:** No log messages from sensor module

**Solution:**
1. Check that ADC_UPLINK module is enabled
2. Verify it's ESP32 platform (not NRF52 or others)
3. Check serial monitor baud rate (115200)

## OLED Display Verification

### With SEN66:
```
╔════════════════════╗
║   Air Quality      ║
╠════════════════════╣
║ [A1B2] 3s ago      ║
║ 23.5C 45% VOC:120  ║
║ PM2.5:8.1 PM10:10.5║
║ --- Other Nodes ---║
║ [C3D4] 15s ago     ║
║ PM2.5:5.2 AQI      ║
╚════════════════════╝
```

### With SHT31:
```
╔════════════════════╗
║   Sensors          ║
╠════════════════════╣
║ [A1B2] 3s ago      ║
║ 23.5°C  45%        ║
║ --- Other Nodes ---║
║ [C3D4] 15s ago     ║
║ 22.1°C  52%        ║
╚════════════════════╝
```

## Performance Metrics

**Expected behavior:**
- Sensor reading: Every 1 second
- Mesh broadcast: Every 30 seconds
- Memory usage: ~8-10KB additional RAM
- CPU usage: Minimal (<1% average)

## Debugging Tips

### Enable Verbose Logging
Modify log levels in code if needed:
```cpp
LOG_DEBUG("Detailed debug info");
LOG_INFO("General information");
LOG_WARN("Warning messages");
LOG_ERROR("Error messages");
```

### Check I2C Communication
Add delay if I2C reads are unstable:
```cpp
// In readMeasuredValues(), increase delay
delay(50); // Instead of 20ms
```

### CRC Errors
If you get frequent CRC errors:
1. Add pull-up resistors (4.7kΩ) to SDA/SCL
2. Reduce I2C clock speed
3. Check for electrical noise
4. Verify wire connections

### Sensor Not Responding
Try increasing initialization delays:
```cpp
// In initSEN66()
delay(200); // Instead of 100ms after reset
```

## Clean Build (If Issues Persist)

```bash
# Full clean and rebuild
pio run -t clean
rm -rf .pio/build
pio run -e heltec-v3
pio run -e heltec-v3 -t upload
```

## Success Criteria

✅ Firmware compiles without errors
✅ Module initializes and detects sensor(s)
✅ Sensor data appears in serial log
✅ Mesh messages broadcast every 30 seconds
✅ OLED display shows sensor readings
✅ Remote node data appears on display
✅ No CRC or I2C communication errors

## Next Steps After Successful Build

1. Test with different air quality conditions
2. Verify accuracy with reference sensors
3. Test mesh network with multiple nodes
4. Monitor long-term stability
5. Consider adding data logging
6. Implement AQI calculations if needed

## Support Files

- **SEN66_INTEGRATION_SUMMARY.md** - Complete implementation details
- **SEN66_QUICK_REFERENCE.md** - Sensor specifications
- **PS_DS_SEN6x.pdf** - Official datasheet (if available)

## Contact

For issues or questions:
- Check Meshtastic forums
- Review Sensirion documentation
- Check I2C connections and power supply
