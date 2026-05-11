# SEN66 Sensor Quick Reference

## Sensor Overview
**Sensirion SEN66** - All-in-one environmental sensor module

## Measurement Parameters

| Parameter | Range | Accuracy | Resolution |
|-----------|-------|----------|------------|
| **PM1.0** | 0-1000 μg/m³ | ±10 μg/m³ | 0.1 μg/m³ |
| **PM2.5** | 0-1000 μg/m³ | ±10 μg/m³ | 0.1 μg/m³ |
| **PM4.0** | 0-1000 μg/m³ | ±25 μg/m³ | 0.1 μg/m³ |
| **PM10** | 0-1000 μg/m³ | ±25 μg/m³ | 0.1 μg/m³ |
| **Temperature** | -10 to +50°C | ±0.5°C | 0.01°C |
| **Humidity** | 0-100% RH | ±3% RH | 0.01% RH |
| **VOC Index** | 0-500 | N/A | 1 |
| **NOx Index** | 0-500 | N/A | 1 |

## I2C Communication

**Address:** 0x6B (fixed, non-configurable)
**Clock Speed:** Up to 100 kHz (standard mode)
**CRC:** 8-bit CRC for data integrity

## Key I2C Commands

| Command | Code | Description |
|---------|------|-------------|
| Start Measurement | 0x0021 | Begin continuous measurement |
| Stop Measurement | 0x0104 | Stop measurements |
| Read Values | 0x03C4 | Read all measured values |
| Get Serial | 0xD033 | Read sensor serial number |
| Reset | 0xD304 | Software reset |
| Get Version | 0xD100 | Read firmware version |

## Power Requirements

- **Supply Voltage:** 4.5-5.5V DC
- **Average Current:** 16 mA
- **Peak Current:** 80 mA

## Timing

- **Warm-up Time:** ~1 second after start measurement
- **Measurement Interval:** 1 second (continuous mode)
- **Reset Time:** ~100 ms

## Data Format

Each measurement value is transmitted as:
- 2 bytes (16-bit signed integer)
- 1 byte CRC-8
- Scale factors applied to convert to physical units

**Example: Temperature**
```
Raw value: 0x0BB8 (3000 decimal)
Scale factor: 200
Result: 3000 / 200 = 15.0°C
```

## Air Quality Index (VOC/NOx)

- **Range:** 0-500
- **100:** Clean air baseline
- **>200:** Poor air quality
- **>300:** Very poor air quality
- **>400:** Hazardous

## Particulate Matter (PM) Levels

| PM2.5 (μg/m³) | Air Quality | Health Impact |
|---------------|-------------|---------------|
| 0-12 | Good | Minimal |
| 12-35 | Moderate | Acceptable |
| 35-55 | Unhealthy (sensitive) | Some concern |
| 55-150 | Unhealthy | Health effects |
| 150-250 | Very Unhealthy | Serious effects |
| >250 | Hazardous | Emergency |

## Physical Dimensions

- **Size:** 41 × 41 × 13 mm
- **Weight:** ~6g
- **Mounting:** 4x mounting holes (M2 screws)

## Wiring (Heltec V3)

```
SEN66         Heltec V3
------        ---------
VDD    <-->   5V
GND    <-->   GND
SDA    <-->   GPIO17 (SDA)
SCL    <-->   GPIO18 (SCL)
SEL    <-->   GND (I2C mode)
```

## Initialization Sequence

1. Power on sensor
2. Wait 100ms
3. Send reset command (optional)
4. Wait 100ms
5. Send start measurement command
6. Wait 1000ms for first reading
7. Poll read values command every 1+ seconds

## Best Practices

1. **Warm-up:** Allow 30 seconds for sensor stabilization
2. **Ventilation:** Ensure good air flow around sensor
3. **Mounting:** Keep away from heat sources
4. **Cleaning:** Do not clean with liquids or compressed air
5. **Storage:** Store in dry environment, <70% RH

## Common Issues

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| Not detected | Wrong I2C address | Check address is 0x6B |
| CRC errors | Electrical noise | Add pull-up resistors (4.7kΩ) |
| Unstable readings | Poor ventilation | Improve air circulation |
| High PM readings | Contaminated sensor | Check environment |

## Datasheet Reference

Full specifications available in: **PS_DS_SEN6x.pdf**

## Driver Implementation

Our custom driver implements:
- ✅ I2C communication with CRC validation
- ✅ Automatic sensor detection
- ✅ Continuous measurement mode
- ✅ All 8 parameter readings
- ✅ Error handling and retry logic
- ✅ Mesh network broadcasting

## Support

For technical questions about the SEN66 sensor:
- Sensirion Support: https://www.sensirion.com/support
- Datasheet: PS_DS_SEN6x.pdf
- Arduino Library: https://github.com/Sensirion/arduino-i2c-sen66
