/**
 * EXAMPLE: WiFi Configuration Usage in Meshtastic Firmware
 * 
 * This file demonstrates how to use the WiFiConfig module in your application.
 * Copy and adapt these patterns to your specific use case.
 */

#include "WiFiConfig/WiFiConnectivity.h"
#include "WiFiConfig/WiFiConfig.h"

/**
 * EXAMPLE 1: Basic WiFi Setup
 * 
 * Call once during firmware initialization to set up WiFi module.
 * This loads stored credentials from NVS and prepares the WiFi radio.
 */
void example_basic_setup() {
    // Initialize the WiFi connectivity manager
    // This loads stored networks from NVS
    WiFiConnectivity::init();
    
    LOG_INFO("[EXAMPLE] WiFi module initialized");
}

/**
 * EXAMPLE 2: Add Networks Programmatically
 * 
 * Store multiple WiFi networks with fallback order.
 * The device will try to connect to them in this order.
 */
void example_add_networks() {
    // Add networks in priority order
    // Device will try HomeNetwork first, then fallback to others
    
    WiFiConfigManager::addNetwork("HomeNetwork", "home_password_123");
    LOG_INFO("[EXAMPLE] Added HomeNetwork");
    
    WiFiConfigManager::addNetwork("OfficeWiFi", "office_secure_pass");
    LOG_INFO("[EXAMPLE] Added OfficeWiFi");
    
    WiFiConfigManager::addNetwork("MobileHotspot", "phone_hotspot_pwd");
    LOG_INFO("[EXAMPLE] Added MobileHotspot");
    
    // CRITICAL: Save to NVS for persistence across reboots!
    WiFiConfigManager::saveToNVS();
    LOG_INFO("[EXAMPLE] Saved networks to NVS");
}

/**
 * EXAMPLE 3: Connect to Stored WiFi
 * 
 * Attempt to connect to any available stored network.
 * Tries networks in order, with per-network timeout of 5 seconds.
 */
void example_connect_to_wifi() {
    LOG_INFO("[EXAMPLE] Attempting WiFi connection...");
    
    // Try to connect with 20 second overall timeout
    if (WiFiConnectivity::connectToWiFi(20000)) {
        LOG_INFO("[EXAMPLE] ✓ Connected!");
        LOG_INFO("[EXAMPLE]   SSID: %s", WiFiConnectivity::getConnectedSSID().c_str());
        LOG_INFO("[EXAMPLE]   IP: %s", WiFi.localIP().toString().c_str());
        LOG_INFO("[EXAMPLE]   RSSI: %d dBm", WiFiConnectivity::getSignalStrength());
    } else {
        LOG_WARN("[EXAMPLE] ✗ Failed to connect to any network");
    }
}

/**
 * EXAMPLE 4: Check Connection Status
 * 
 * Monitor WiFi connection in your main loop.
 */
void example_monitor_connection() {
    static uint32_t last_check = 0;
    
    // Check connection status every 10 seconds
    if (millis() - last_check > 10000) {
        last_check = millis();
        
        if (WiFiConnectivity::isConnected()) {
            LOG_DEBUG("[EXAMPLE] WiFi connected to: %s (Signal: %d dBm)",
                     WiFiConnectivity::getConnectedSSID().c_str(),
                     WiFiConnectivity::getSignalStrength());
        } else {
            LOG_DEBUG("[EXAMPLE] WiFi not connected");
        }
    }
}

/**
 * EXAMPLE 5: Scan for Available Networks
 * 
 * Discover what WiFi networks are currently available nearby.
 */
void example_scan_networks() {
    LOG_INFO("[EXAMPLE] Scanning for available networks...");
    
    auto available_networks = WiFiConnectivity::scanNetworks();
    
    LOG_INFO("[EXAMPLE] Found %d networks:", available_networks.size());
    for (size_t i = 0; i < available_networks.size(); i++) {
        LOG_INFO("[EXAMPLE]   [%d] %s", i, available_networks[i].c_str());
    }
    
    // You could use this to show users what networks are available
    // or to auto-select the best network
}

/**
 * EXAMPLE 6: Manage Networks
 * 
 * Add, remove, and list stored networks.
 */
void example_manage_networks() {
    // Show current networks
    LOG_INFO("[EXAMPLE] Current stored networks:");
    WiFiConfigManager::printNetworks();
    
    // Get count
    LOG_INFO("[EXAMPLE] Total networks: %d", WiFiConfigManager::getNetworkCount());
    
    // Access individual networks
    for (size_t i = 0; i < WiFiConfigManager::getNetworkCount(); i++) {
        auto* net = WiFiConfigManager::getNetworkByIndex(i);
        if (net) {
            LOG_INFO("[EXAMPLE]   Network %d: %s", i, net->ssid.c_str());
        }
    }
    
    // Find specific network
    auto* home_net = WiFiConfigManager::getNetworkBySSID("HomeNetwork");
    if (home_net) {
        LOG_INFO("[EXAMPLE] Found HomeNetwork!");
    }
    
    // Remove a network
    WiFiConfigManager::removeNetwork("OldNetwork");
    WiFiConfigManager::saveToNVS();
    LOG_INFO("[EXAMPLE] Removed OldNetwork");
}

/**
 * EXAMPLE 7: Integration with Main Loop
 * 
 * This shows how to integrate WiFi into your main firmware loop.
 */
void example_main_loop() {
    static bool wifi_initialized = false;
    static uint32_t last_connection_attempt = 0;
    static uint32_t last_status_report = 0;
    
    // One-time initialization
    if (!wifi_initialized) {
        WiFiConnectivity::init();
        
        // Pre-populate with default networks
        WiFiConfigManager::addNetwork("DefaultHome", "default_pwd");
        WiFiConfigManager::saveToNVS();
        
        wifi_initialized = true;
    }
    
    // Try to connect every 60 seconds if not connected
    if (!WiFiConnectivity::isConnected() && 
        (millis() - last_connection_attempt > 60000)) {
        last_connection_attempt = millis();
        
        LOG_INFO("[EXAMPLE] Attempting WiFi connection...");
        WiFiConnectivity::connectToWiFi(30000); // 30 second timeout
    }
    
    // Report status every 5 minutes
    if (millis() - last_status_report > 300000) {
        last_status_report = millis();
        
        if (WiFiConnectivity::isConnected()) {
            LOG_INFO("[EXAMPLE] WiFi Status: Connected to %s (RSSI: %d dBm)",
                     WiFiConnectivity::getConnectedSSID().c_str(),
                     WiFiConnectivity::getSignalStrength());
        } else {
            LOG_INFO("[EXAMPLE] WiFi Status: Not connected");
        }
    }
}

/**
 * EXAMPLE 8: Dynamic Network Addition via Message
 * 
 * This shows how you might add networks from received messages
 * (e.g., admin commands or BLE configuration)
 */
void example_add_network_from_message(const char* message) {
    // Parse message format: "ADD_WIFI:SSID:PASSWORD"
    // e.g., "ADD_WIFI:HomeNetwork:secure_password_123"
    
    const char* delimiter = ":";
    char* copy = strdup(message);
    
    char* command = strtok(copy, delimiter);
    if (strcmp(command, "ADD_WIFI") == 0) {
        char* ssid = strtok(NULL, delimiter);
        char* password = strtok(NULL, delimiter);
        
        if (ssid && password) {
            if (WiFiConfigManager::addNetwork(ssid, password)) {
                WiFiConfigManager::saveToNVS();
                LOG_INFO("[EXAMPLE] Network added: %s", ssid);
            } else {
                LOG_WARN("[EXAMPLE] Failed to add network: %s", ssid);
            }
        }
    }
    
    free(copy);
}

/**
 * EXAMPLE 9: Emergency Network Configuration
 * 
 * In case of configuration issues, allow adding a fallback network.
 */
void example_emergency_network_config() {
    // Check if we have any networks configured
    if (WiFiConfigManager::getNetworkCount() == 0) {
        LOG_WARN("[EXAMPLE] No networks configured - adding emergency fallback");
        
        // Add a fallback network that's commonly available
        WiFiConfigManager::addNetwork("EMERGENCY_MESH", "mesh_password");
        WiFiConfigManager::saveToNVS();
        
        LOG_INFO("[EXAMPLE] Emergency network added");
    }
}

/**
 * EXAMPLE 10: Clean Shutdown
 * 
 * Properly disconnect and cleanup when shutting down.
 */
void example_shutdown() {
    LOG_INFO("[EXAMPLE] Shutting down WiFi...");
    WiFiConnectivity::disconnect();
    WiFiConnectivity::setWiFiEnabled(false);
    LOG_INFO("[EXAMPLE] WiFi disabled and disconnected");
}

/**
 * RECOMMENDED INTEGRATION PATTERN
 * 
 * Use this pattern in your actual firmware:
 */

// In your setup() or initialization function:
void my_firmware_setup() {
    // ... other setup code ...
    
    // Initialize WiFi module
    WiFiConnectivity::init();
    
    // Add any default networks
    // (First time setup, NVS will be empty)
    if (WiFiConfigManager::getNetworkCount() == 0) {
        example_add_networks();
    }
}

// In your main loop():
void my_firmware_loop() {
    static uint32_t last_wifi_attempt = 0;
    
    // Try WiFi connection periodically if not connected
    if (!WiFiConnectivity::isConnected() && 
        (millis() - last_wifi_attempt > 30000)) {
        last_wifi_attempt = millis();
        WiFiConnectivity::connectToWiFi(15000);
    }
    
    // ... rest of main loop ...
}
