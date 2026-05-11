# WiFi Configuration Module - Quick Start Guide

Get up and running in 5 minutes!

## 1. Copy the Module

The module is already located at:
```
/Users/egekeskin/Desktop/firmware/src/modules/WiFiConfig/
```

## 2. Add to Your Setup Function

```cpp
#include "modules/WiFiConfig/WiFiConnectivity.h"

void setup() {
    // ... existing setup code ...
    
    // Initialize WiFi module
    WiFiConnectivity::init();
}
```

## 3. Add Networks

```cpp
// Add your WiFi networks
WiFiConfigManager::addNetwork("MySSID", "MyPassword");
WiFiConfigManager::addNetwork("BackupSSID", "BackupPassword");

// Save to persistent storage (IMPORTANT!)
WiFiConfigManager::saveToNVS();
```

## 4. Connect to WiFi

```cpp
// Try to connect with 15 second timeout
if (WiFiConnectivity::connectToWiFi(15000)) {
    LOG_INFO("Connected to: %s", WiFiConnectivity::getConnectedSSID().c_str());
} else {
    LOG_WARN("Failed to connect to WiFi");
}
```

## 5. Monitor Connection

```cpp
// In your main loop
if (WiFiConnectivity::isConnected()) {
    LOG_DEBUG("WiFi signal: %d dBm", WiFiConnectivity::getSignalStrength());
}
```

## Complete Example

```cpp
#include "modules/WiFiConfig/WiFiConnectivity.h"

void setup() {
    Serial.begin(115200);
    
    // Initialize WiFi
    WiFiConnectivity::init();
    
    // Add networks (if first time setup)
    if (WiFiConfigManager::getNetworkCount() == 0) {
        WiFiConfigManager::addNetwork("Home", "home_password");
        WiFiConfigManager::addNetwork("Mobile", "mobile_password");
        WiFiConfigManager::saveToNVS();
    }
    
    // Connect
    if (WiFiConnectivity::connectToWiFi(20000)) {
        LOG_INFO("✓ WiFi connected!");
    }
}

void loop() {
    static uint32_t last_check = 0;
    
    // Check connection every 30 seconds
    if (millis() - last_check > 30000) {
        last_check = millis();
        
        if (WiFiConnectivity::isConnected()) {
            LOG_DEBUG("Connected to %s (RSSI: %d dBm)", 
                     WiFiConnectivity::getConnectedSSID().c_str(),
                     WiFiConnectivity::getSignalStrength());
        } else {
            LOG_INFO("Reconnecting to WiFi...");
            WiFiConnectivity::connectToWiFi(15000);
        }
    }
}
```

## Common Tasks

### List All Networks

```cpp
WiFiConfigManager::printNetworks();
```

Output:
```
[WiFiConfig] ===== Stored WiFi Networks =====
[WiFiConfig] [0] SSID: Home | Password: home_password
[WiFiConfig] [1] SSID: Mobile | Password: mobile_password
[WiFiConfig] ================================
```

### Remove a Network

```cpp
WiFiConfigManager::removeNetwork("Mobile");
WiFiConfigManager::saveToNVS();
```

### Clear All Networks

```cpp
WiFiConfigManager::clearAllNetworks();  // Clears and saves automatically
```

### Find Specific Network

```cpp
auto* network = WiFiConfigManager::getNetworkBySSID("Home");
if (network) {
    LOG_INFO("Found: %s", network->ssid.c_str());
}
```

### Scan Available Networks

```cpp
auto networks = WiFiConnectivity::scanNetworks();
for (const auto& ssid : networks) {
    LOG_INFO("Available: %s", ssid.c_str());
}
```

### Check Signal Strength

```cpp
if (WiFiConnectivity::isConnected()) {
    int rssi = WiFiConnectivity::getSignalStrength();
    LOG_INFO("Signal: %d dBm", rssi);
    
    // Interpret RSSI
    if (rssi > -50) LOG_INFO("Excellent signal");
    else if (rssi > -60) LOG_INFO("Good signal");
    else if (rssi > -80) LOG_INFO("Fair signal");
    else LOG_WARN("Poor signal");
}
```

## Troubleshooting

### Networks Not Saving
❌ Forgot to call `saveToNVS()`:
```cpp
WiFiConfigManager::addNetwork("SSID", "pass");
// ❌ WRONG - won't persist!
```

✅ Call save after changes:
```cpp
WiFiConfigManager::addNetwork("SSID", "pass");
WiFiConfigManager::saveToNVS();  // ✅ Correct!
```

### Connection Failures
Check the logs:
```cpp
WiFiConfigManager::printNetworks();  // Verify networks exist
WiFiConnectivity::scanNetworks();     // Check available networks
WiFiConnectivity::connectToWiFi(30000);  // Try with longer timeout
```

### No Networks Stored
First time setup - networks are empty:
```cpp
if (WiFiConfigManager::getNetworkCount() == 0) {
    LOG_WARN("No networks configured!");
    // Add default networks
    WiFiConfigManager::addNetwork("DefaultNetwork", "default_pwd");
    WiFiConfigManager::saveToNVS();
}
```

## API Reference

### WiFiConfigManager (Credential Management)

| Method | Purpose |
|--------|---------|
| `init()` | Load networks from NVS |
| `addNetwork(ssid, pass)` | Add a network |
| `removeNetwork(ssid)` | Remove a network |
| `getNetworks()` | Get all networks |
| `getNetworkCount()` | Count of networks |
| `getNetworkByIndex(i)` | Access by position |
| `getNetworkBySSID(ssid)` | Find by SSID |
| `clearAllNetworks()` | Delete all networks |
| `saveToNVS()` | Persist to storage |
| `loadFromNVS()` | Load from storage |
| `printNetworks()` | Debug output |

### WiFiConnectivity (Connection Management)

| Method | Purpose |
|--------|---------|
| `init()` | Initialize WiFi |
| `connectToWiFi(timeout)` | Connect with timeout |
| `disconnect()` | Disconnect WiFi |
| `isConnected()` | Check if connected |
| `getConnectedSSID()` | Get current SSID |
| `scanNetworks()` | Scan for networks |
| `getSignalStrength()` | Get RSSI in dBm |
| `setWiFiEnabled(bool)` | Enable/disable WiFi |
| `isWiFiEnabled()` | Check if enabled |

## Key Parameters

| Parameter | Default | Range |
|-----------|---------|-------|
| Connect Timeout | 15s | 5-60s |
| Per-Network Timeout | 5s | Fixed |
| Max Networks | 10 | 1-10 |
| RSSI Good | > -60 dBm | Typical |
| RSSI Excellent | > -50 dBm | Strong |

## Performance

- **Memory Usage**: ~2KB (10 networks @ 160 bytes each)
- **Connection Time**: 5-20 seconds (network dependent)
- **Scan Time**: 2-5 seconds
- **NVS Access Time**: < 100ms

## Best Practices

✅ Always call `saveToNVS()` after modifying networks  
✅ Initialize `WiFiConnectivity::init()` early in setup  
✅ Use 15-30 second timeout for connection attempts  
✅ Check `isConnected()` before using WiFi  
✅ Monitor signal strength regularly  
✅ Provide fallback networks  
✅ Log connection attempts for debugging  

## Files to Review

1. **[README.md](README.md)** - Full documentation
2. **[EXAMPLES.md](EXAMPLES.md)** - 10 usage examples
3. **[INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md)** - Integration steps
4. **WiFiConfig.h** - Credential manager header
5. **WiFiConnectivity.h** - Connection manager header

## Support & Debugging

Enable debug logging to see detailed operation:
```cpp
// In your platformio.ini or build settings:
// -D CORE_DEBUG_LEVEL=5   // Maximum logging

// Then check logs:
// [WiFiConfig] messages
// [WiFiConnectivity] messages
```

---

**Ready to use!** Check [README.md](README.md) for more details.
