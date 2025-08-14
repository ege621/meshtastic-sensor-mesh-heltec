# Meshtastic Sensor Mesh - Heltec V3 ADC Module

A distributed sensor mesh network built on Meshtastic firmware for Heltec LoRa V3 boards. Each node reads analog sensor data and shares it across the mesh network via LoRa radio.

## 🎯 **Project Overview**

This project extends the Meshtastic firmware with a custom ADC module that:
- Reads analog sensor data from GPIO2 on Heltec V3 boards
- Broadcasts sensor readings in JSON format across the mesh
- Displays all network sensor data on each node's OLED
- Supports multi-hop routing for extended range coverage

## 📡 **Network Architecture**

- **Decentralized mesh**: Each board acts as both sensor and router
- **Automatic routing**: Data hops through intermediate nodes automatically
- **Self-healing**: Network reroutes around failed nodes
- **Range**: 1-5km per hop (depending on terrain)

## 🔧 **Hardware Requirements**

- 5x Heltec WiFi LoRa 32 V3 boards (ESP32-S3)
- Analog sensors connected to GPIO2
- USB-C cables for programming

## 📋 **Features**

### **Sensor Module (`adc_uplink`)**
- Reads 12-bit ADC values from GPIO2
- Averages multiple samples for stability
- Converts to voltage (0-3.3V range)
- Unique device ID from MAC address
- 10-second transmission interval with jitter

### **Data Format**
```json
{"id":"ABCD","pin":2,"raw":1234,"V":1.23}
```

### **Mesh Networking**
- Up to 7 hops supported
- Automatic best-path routing
- Real-time data display on all nodes
- Fault-tolerant operation

## 🚀 **Getting Started**

### **1. Firmware Installation**

This module integrates with the official Meshtastic firmware. Follow these steps:

1. Clone the Meshtastic firmware:
```bash
git clone https://github.com/meshtastic/firmware.git
cd firmware
```

2. Copy the ADC module:
```bash
cp -r /path/to/this/repo/adc_uplink src/modules/
```

3. Apply the integration changes (see `INTEGRATION.md` for details)

4. Build and upload:
```bash
pio run -e heltec-v3 -t upload
```

### **2. Hardware Setup**

1. Connect analog sensors to GPIO2 on each Heltec board
2. Ensure proper power supply (USB or battery)
3. Place boards within LoRa range for mesh connectivity

### **3. Operation**

- Each board displays its device ID on startup
- Sensor data updates every 10 seconds
- OLED shows data from all nodes in the mesh
- Serial monitor shows detailed logs

## 📊 **Data Flow**

```
Sensor → ADC (GPIO2) → ESP32-S3 → JSON → LoRa → Mesh → All Nodes
```

Each node receives and displays data from all other nodes, creating a fully distributed sensor network.

## 🛠 **Configuration**

Key parameters in `adc_uplink.h`:
- `ADC_PIN`: GPIO pin for sensor (default: 2)
- `PERIOD_MS`: Transmission interval (default: 10000ms)
- `SAMPLE_COUNT`: ADC averaging samples (default: 8)
- `HOPS`: Maximum mesh hops (default: 3)

## 📈 **Monitoring**

Connect to any node via serial to monitor:
- Device startup and ID
- Sensor readings
- Mesh packet transmission
- Network topology changes

## 🔍 **Troubleshooting**

- **No data on OLED**: Check LoRa connectivity between nodes
- **Sensor errors**: Verify GPIO2 connection and sensor power
- **Upload fails**: Ensure serial monitor is closed, check USB connection

## 📚 **Documentation**

- `ARCHITECTURE_OVERVIEW.md`: Detailed system architecture
- `SENSOR_MESH_README.md`: Sensor-specific documentation
- `adc_uplink/`: Module source code and headers

## 🤝 **Contributing**

1. Fork this repository
2. Create a feature branch
3. Make your changes
4. Test on hardware
5. Submit a pull request

## 📄 **License**

This project extends Meshtastic firmware under the GPL v3 license.

## 🙏 **Acknowledgments**

- [Meshtastic Project](https://meshtastic.org) for the excellent mesh networking platform
- Heltec for the LoRa development boards
- ESP32 Arduino framework

## 🔗 **Related Projects**

- [Official Meshtastic Firmware](https://github.com/meshtastic/firmware)
- [Meshtastic Documentation](https://meshtastic.org/docs)
- [Heltec Board Documentation](https://heltec.org)

---

**Built with ❤️ for distributed sensor networks**
