#pragma once

#include "WiFiConfig.h"

/**
 * WiFiConnectivity - Handles WiFi connection attempts using stored credentials
 * Tries to connect to networks in order of priority
 */
class WiFiConnectivity {
public:
    /**
     * Initialize WiFi connectivity manager
     * Should be called once during setup()
     */
    static void init();
    
    /**
     * Attempt to connect to WiFi using stored credentials
     * Tries each network in order until one succeeds
     * @param timeoutMs Maximum time to wait for connection
     * @return true if successfully connected, false otherwise
     */
    static bool connectToWiFi(uint32_t timeoutMs = 15000);
    
    /**
     * Disconnect from current WiFi network
     */
    static void disconnect();
    
    /**
     * Check if currently connected to WiFi
     * @return true if connected
     */
    static bool isConnected();
    
    /**
     * Get the currently connected SSID
     * @return SSID string, empty if not connected
     */
    static std::string getConnectedSSID();
    
    /**
     * Perform a WiFi scan for available networks
     * @return Vector of found SSIDs
     */
    static std::vector<std::string> scanNetworks();
    
    /**
     * Get the current WiFi signal strength (RSSI)
     * @return RSSI value in dBm, or 0 if not connected
     */
    static int32_t getSignalStrength();
    
    /**
     * Enable/disable WiFi module
     * @param enabled true to enable, false to disable
     */
    static void setWiFiEnabled(bool enabled);
    
    /**
     * Get WiFi module enabled state
     * @return true if WiFi is enabled
     */
    static bool isWiFiEnabled();

private:
    static bool wifiEnabled;
    static std::string connectedSSID;
};
