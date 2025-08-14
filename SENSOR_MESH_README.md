# ADC Sensor Mesh for Heltec LoRa V3 Boards

This firmware modification creates a sensor mesh network where each Heltec LoRa V3 board reads analog values from GPIO2 and broadcasts them to the mesh network along with their unique device ID.

## What's been modified:

### 1. ADC Uplink Module (`src/modules/adc_uplink/`)
- **adc_uplink.h**: Module class definition with configurable parameters
- **adc_uplink.cpp**: Implementation that reads GPIO2, formats JSON, and sends over mesh

### 2. Module Registration (`src/modules/Modules.cpp`)
- Added ADC uplink module to the module system
- Module is conditionally compiled for ESP32 platforms only

### 3. Build Configuration
- **platformio.ini**: Added ArduinoJson library dependency
- **variants/esp32s3/heltec_v3/platformio.ini**: Enabled ADC uplink module for Heltec V3
- **variants/esp32s3/heltec_wsl_v3/platformio.ini**: Enabled ADC uplink module for Heltec WSL V3
- **src/configuration.h**: Added MESHTASTIC_EXCLUDE_ADC_UPLINK compile flag

## Configuration:

The module is configured with these default settings:
- **ADC Pin**: GPIO2 (ADC1_CH1 on ESP32-S3)
- **ADC Resolution**: 12 bits (0-4095)
- **Reference Voltage**: 3.3V
- **Sample Count**: 16 (averaged for noise reduction)
- **Transmission Period**: 10 seconds
- **Mesh Hop Limit**: 3 hops
- **Jitter**: 0-499ms random delay to prevent synchronized transmissions

## JSON Output Format:

Each node broadcasts JSON messages in this format:
```json
{
  "id": "ABCD",     // Last 2 bytes of MAC address (4 hex digits)
  "pin": 2,         // GPIO pin number
  "raw": 1234,      // Raw ADC reading (0-4095)
  "V": 1.23         // Calculated voltage (2 decimal places)
}
```

## Hardware Setup:

1. **Sensor Connection**: Connect your analog sensor to GPIO2 on each Heltec LoRa V3 board
2. **Voltage Range**: Ensure sensor output is 0-3.3V (or adjust VREF_VOLTS in code)
3. **Power**: GPIO2 can be used directly with voltage divider circuits

## Building and Flashing:

1. **Install PlatformIO**: If not already installed
   ```bash
   pip install platformio
   ```

2. **Build for Heltec V3**:
   ```bash
   cd /path/to/firmware
   pio run -e heltec-v3
   ```

3. **Flash to devices**:
   ```bash
   pio run -e heltec-v3 -t upload
   ```

## Viewing Sensor Data:

The sensor data will appear as text messages in:
- **Meshtastic mobile app**: Check the text message channel
- **Serial console**: Connect to USB and view at 115200 baud
- **Other mesh nodes**: All nodes will receive the sensor broadcasts

## Customization:

To modify the sensor reading behavior, edit `src/modules/adc_uplink/adc_uplink.h`:

```cpp
// Change these values as needed:
static constexpr int   ADC_PIN      = 2;        // GPIO pin
static constexpr uint32_t PERIOD_MS = 10000;    // Transmission interval (ms)
static constexpr float VREF_VOLTS   = 3.30f;    // Reference voltage
```

## Device Identification:

Each device uses the last 2 bytes of its MAC address as a unique identifier. This ensures you can distinguish between your 5 boards in the mesh network.

## Mesh Network Benefits:

- **Redundant paths**: Sensor data can reach any node via multiple routes
- **Range extension**: Nodes act as repeaters for each other
- **Real-time monitoring**: All sensor readings are broadcast to all nodes
- **Automatic discovery**: New nodes automatically join the mesh

Your 5 Heltec LoRa V3 boards will now form a sensor mesh where each board:
1. Reads an analog sensor on GPIO2 every 10 seconds
2. Broadcasts the reading with its device ID to the mesh
3. Receives and can display readings from all other boards
4. Acts as a repeater to extend the mesh range

The mesh will be self-organizing and fault-tolerant, automatically routing around any failed nodes.
