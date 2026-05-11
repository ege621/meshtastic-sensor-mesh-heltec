# I2C Sensor Troubleshooting Guide

## Current Status
Your firmware is working correctly! No sensors are detected because they're not physically connected yet.

## Expected I2C Addresses
- **SEN66:** 0x6B (107 decimal)
- **SHT31:** 0x44 (68 decimal)

## Hardware Connection Checklist

### Heltec V3 I2C Pins:
- **SDA:** GPIO17
- **SCL:** GPIO18
- **Power:** 3.3V or 5V (depending on sensor)
- **GND:** GND

### Connection Steps:

1. **Power off** the Heltec board
2. **Connect sensor** to I2C pins:
   - Red wire → VDD (3.3V or 5V)
   - Black wire → GND
   - Yellow/Green wire → SDA (GPIO17)
   - Blue/White wire → SCL (GPIO18)
3. **For SEN66:** Connect SEL pin to GND (enables I2C mode)
4. **Power on** and check serial monitor

### Common Issues:

| Issue | Solution |
|-------|----------|
| No sensor detected | Check all wire connections |
| Wire.cpp warning | Normal - I2C bus already initialized |
| Wrong address | Verify sensor datasheet for correct I2C address |
| Intermittent detection | Add 4.7kΩ pull-up resistors to SDA/SCL |
| Multiple warnings | Check power supply stability |

## Testing Without Sensors

If you want to test the code without sensors, you can temporarily modify the code to use dummy data. But the proper way is to connect actual hardware.

## What the Logs Tell Us:

```
✅ Module starts correctly
✅ I2C bus initializes
✅ Scans for SEN66 at 0x6B
✅ Falls back to SHT31 at 0x44  
✅ Properly reports "no sensors found"
✅ Disables module gracefully
```

Your firmware is **ready to go** - just connect a sensor!

## After Connecting Sensor:

Press the **RESET** button on your Heltec board, and watch the serial monitor. You should see:

**Success with SEN66:**
```
INFO  | [ADCUplinkModule] SEN66 air quality sensor detected and initialized
INFO  | [ADCUplinkModule] Device ID: XXXX | Active Sensor: SEN66 (Air Quality)
```

**Success with SHT31:**
```
INFO  | [ADCUplinkModule] SHT31 temperature/humidity sensor detected and initialized
INFO  | [ADCUplinkModule] Device ID: XXXX | Active Sensor: SHT31 (Temperature/Humidity)
```

Then every 30 seconds you'll see sensor data being broadcast to the mesh network!
