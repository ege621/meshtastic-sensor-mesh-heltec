# WiFi Configuration Module

Complete WiFi credential management system for Meshtastic firmware on ESP32. Stores multiple SSID/password pairs persistently and manages automatic connection attempts.

## Features

- **Multiple Network Support**: Store up to 10 WiFi networks with automatic fallback
- **Persistent Storage**: Uses ESP32 NVS (Non-Volatile Storage) for credentials across reboots
- **Automatic Connection**: Try networks in order until one succeeds
- **Signal Monitoring**: Check connection status and signal strength (RSSI)
- **Network Scanning**: Discover available WiFi networks
- **Debug Logging**: Comprehensive LOG_INFO/LOG_WARN/LOG_DEBUG output

## Components

### WiFiConfigManager
Core credential management system:
- `init()` - Initialize and load from NVS
- `addNetwork(ssid, password)` - Add a new network
- `removeNetwork(ssid)` - Remove a network by SSID
- `getNetworks()` - Get all stored networks
- `getNetworkCount()` - Get number of networks
- `getNetworkByIndex(i)` - Get network at index
- `getNetworkBySSID(ssid)` - Find network by SSID
- `clearAllNetworks()` - Remove all networks
- `saveToNVS()` - Persist to NVS
- `loadFromNVS()` - Load from NVS
- `printNetworks()` - Debug print all networks

### WiFiConnectivity
WiFi connection manager:
- `init()` - Initialize WiFi module
- `connectToWiFi(timeoutMs)` - Connect to stored networks
- `disconnect()` - Disconnect and disable WiFi
- `isConnected()` - Check connection status
- `getConnectedSSID()` - Get current SSID
- `scanNetworks()` - Scan for available networks
- `getSignalStrength()` - Get RSSI in dBm
- `setWiFiEnabled(bool)` - Enable/disable WiFi
- `isWiFiEnabled()` - Check if WiFi enabled

## Usage Example

```cpp
// In main setup or initialization:
WiFiConnectivity::init();

// Add networks programmatically:
WiFiConfigManager::addNetwork("HomeWiFi", "password123");
WiFiConfigManager::addNetwork("MobileHotspot", "hotspot_pwd");
WiFiConfigManager::addNetwork("Office", "office_pass");

// Persist to NVS:
WiFiConfigManager::saveToNVS();

// Connect to WiFi (tries each network in order, 15 second timeout):
if (WiFiConnectivity::connectToWiFi(15000)) {
    LOG_INFO("Connected to WiFi!");
    LOG_INFO("IP: %s", WiFi.localIP().toString().c_str());
    LOG_INFO("Signal: %d dBm", WiFiConnectivity::getSignalStrength());
} else {
    LOG_WARN("Could not connect to any WiFi network");
}

// Later, disconnect:
WiFiConnectivity::disconnect();

// Manage networks:
WiFiConfigManager::removeNetwork("HomeWiFi");
WiFiConfigManager::printNetworks();
```

## Integration with Meshtastic Admin Commands

To integrate with meshtastic admin commands (future work):

```cpp
// Add command to add network
// ADMIN_COMMAND: add_wifi <ssid> <password>

// Add command to remove network
// ADMIN_COMMAND: remove_wifi <ssid>

// Add command to list networks
// ADMIN_COMMAND: list_wifi_networks

// Add command to connect now
// ADMIN_COMMAND: wifi_connect
```

## NVS Storage Format

Credentials are stored in the "wifi_config" namespace:
- `net_count` (uint32) - Number of stored networks
- `ssid_0`, `ssid_1`, ..., `ssid_9` (string) - Network SSIDs
- `pass_0`, `pass_1`, ..., `pass_9` (string) - Network passwords

Example NVS contents:
```
Namespace: wifi_config
  net_count = 3
  ssid_0 = "HomeWiFi"
  pass_0 = "password123"
  ssid_1 = "MobileHotspot"
  pass_1 = "hotspot_pwd"
  ssid_2 = "Office"
  pass_2 = "office_pass"
```

## Limits

- Maximum networks: 10
- Maximum SSID length: 32 characters
- Maximum password length: 64 characters
- Connection timeout per network: 5 seconds
- Overall connection timeout: Configurable (default 15 seconds)

## Logging Output Example

```
[WiFiConfig] Initializing WiFi Configuration Manager
[WiFiConfig] Loaded 3 WiFi networks from NVS
[WiFiConfig] ===== Stored WiFi Networks =====
[WiFiConfig] [0] SSID: HomeWiFi | Password: password123
[WiFiConfig] [1] SSID: MobileHotspot | Password: hotspot_pwd
[WiFiConfig] [2] SSID: Office | Password: office_pass
[WiFiConfig] ================================
[WiFiConnectivity] Initializing WiFi connectivity manager
[WiFiConnectivity] WiFi initialized in STA mode
[WiFiConnectivity] Attempting to connect to WiFi (3 networks available)
[WiFiConnectivity] Trying network [1/3]: HomeWiFi
[WiFiConnectivity] Successfully connected to: HomeWiFi
[WiFiConnectivity] IP Address: 192.168.1.100
[WiFiConnectivity] RSSI: -52 dBm
```

## Files

- `WiFiConfig.h` - Credential manager header
- `WiFiConfig.cpp` - Credential manager implementation
- `WiFiConnectivity.h` - Connection manager header
- `WiFiConnectivity.cpp` - Connection manager implementation
- `README.md` - This file

## Future Enhancements

1. Priority ordering of networks
2. RSSI-based network selection
3. 5GHz/2.4GHz band selection
4. WPA3 support
5. Meshtastic admin command integration
6. Web UI for credential management
7. Network-specific settings (hostname, static IP, etc.)
8. Connection failure callbacks
