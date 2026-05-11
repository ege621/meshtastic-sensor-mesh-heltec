#pragma once

#include <vector>
#include <string>
#include <cstdint>

/**
 * Structure to hold WiFi network credentials
 */
struct WiFiNetwork {
    std::string ssid;
    std::string password;
    
    WiFiNetwork() = default;
    WiFiNetwork(const char* s, const char* p) : ssid(s), password(p) {}
};

/**
 * WiFiConfigManager - Manages WiFi network credentials storage and retrieval
 * Stores credentials in ESP32 NVS (Non-Volatile Storage) for persistence across reboots
 */
class WiFiConfigManager {
public:
    /**
     * Initialize the WiFi config manager
     * Loads existing credentials from NVS
     */
    static void init();
    
    /**
     * Add a new WiFi network to the list
     * @param ssid Network SSID
     * @param password Network password
     * @return true if added successfully
     */
    static bool addNetwork(const char* ssid, const char* password);
    
    /**
     * Remove a WiFi network by SSID
     * @param ssid Network SSID to remove
     * @return true if removed successfully
     */
    static bool removeNetwork(const char* ssid);
    
    /**
     * Get all stored WiFi networks
     * @return Vector of WiFiNetwork structures
     */
    static const std::vector<WiFiNetwork>& getNetworks();
    
    /**
     * Get the number of stored networks
     * @return Number of networks
     */
    static size_t getNetworkCount();
    
    /**
     * Get a specific network by index
     * @param index Network index
     * @return Pointer to WiFiNetwork or nullptr if index out of bounds
     */
    static WiFiNetwork* getNetworkByIndex(size_t index);
    
    /**
     * Get network credentials by SSID
     * @param ssid SSID to search for
     * @return Pointer to WiFiNetwork or nullptr if not found
     */
    static WiFiNetwork* getNetworkBySSID(const char* ssid);
    
    /**
     * Clear all stored networks
     * @return true if cleared successfully
     */
    static bool clearAllNetworks();
    
    /**
     * Save credentials to NVS
     * Must be called after adding/removing networks to persist changes
     * @return true if saved successfully
     */
    static bool saveToNVS();
    
    /**
     * Load credentials from NVS
     * Called automatically on init()
     * @return true if loaded successfully
     */
    static bool loadFromNVS();
    
    /**
     * Print all stored networks (for debugging)
     */
    static void printNetworks();

private:
    static constexpr const char* NVS_NAMESPACE = "wifi_config";
    static constexpr const char* NVS_COUNT_KEY = "net_count";
    static constexpr const char* NVS_SSID_KEY_PREFIX = "ssid_";
    static constexpr const char* NVS_PASS_KEY_PREFIX = "pass_";
    static constexpr size_t WIFICONFIG_MAX_NETWORKS = 10;
    static constexpr size_t WIFICONFIG_MAX_SSID_LENGTH = 32;
    static constexpr size_t WIFICONFIG_MAX_PASSWORD_LENGTH = 64;
    
    static std::vector<WiFiNetwork> networks;
    static bool initialized;
};
