#include "WiFiConnectivity.h"
#include <WiFi.h>
#include "configuration.h"

// Initialize static members
bool WiFiConnectivity::wifiEnabled = true;
std::string WiFiConnectivity::connectedSSID = "";

void WiFiConnectivity::init() {
    LOG_INFO("[WiFiConnectivity] Initializing WiFi connectivity manager");
    
    // Initialize WiFiConfigManager to load stored credentials
    WiFiConfigManager::init();
    
    // Start WiFi in station mode
    WiFi.mode(WIFI_STA);
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    
    LOG_INFO("[WiFiConnectivity] WiFi initialized in STA mode");
}

bool WiFiConnectivity::connectToWiFi(uint32_t timeoutMs) {
    if (!wifiEnabled) {
        LOG_DEBUG("[WiFiConnectivity] WiFi is disabled, skipping connection attempt");
        return false;
    }
    
    if (isConnected()) {
        LOG_DEBUG("[WiFiConnectivity] Already connected to: %s", getConnectedSSID().c_str());
        return true;
    }
    
    const auto& networks = WiFiConfigManager::getNetworks();
    
    if (networks.empty()) {
        LOG_WARN("[WiFiConnectivity] No WiFi networks configured");
        return false;
    }
    
    LOG_INFO("[WiFiConnectivity] Attempting to connect to WiFi (%d networks available)", networks.size());
    
    uint32_t startTime = millis();
    
    // Try each network in order
    for (size_t i = 0; i < networks.size(); i++) {
        if (millis() - startTime > timeoutMs) {
            LOG_WARN("[WiFiConnectivity] Connection timeout reached");
            return false;
        }
        
        const WiFiNetwork& net = networks[i];
        
        LOG_DEBUG("[WiFiConnectivity] Trying network [%d/%d]: %s", 
                  i + 1, networks.size(), net.ssid.c_str());
        
        WiFi.begin(net.ssid.c_str(), net.password.c_str());
        
        // Wait for connection with per-network timeout
        uint32_t networkStartTime = millis();
        uint32_t perNetworkTimeout = 5000; // 5 seconds per network
        
        while (WiFi.status() != WL_CONNECTED && 
               (millis() - networkStartTime) < perNetworkTimeout &&
               (millis() - startTime) < timeoutMs) {
            delay(500);
            LOG_DEBUG("[WiFiConnectivity] Connecting... status=%d", WiFi.status());
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            connectedSSID = net.ssid;
            LOG_INFO("[WiFiConnectivity] Successfully connected to: %s", net.ssid.c_str());
            LOG_INFO("[WiFiConnectivity] IP Address: %s", WiFi.localIP().toString().c_str());
            LOG_INFO("[WiFiConnectivity] RSSI: %d dBm", WiFi.RSSI());
            return true;
        }
        
        LOG_DEBUG("[WiFiConnectivity] Failed to connect to: %s", net.ssid.c_str());
        WiFi.disconnect(false); // false = don't turn off WiFi
    }
    
    LOG_WARN("[WiFiConnectivity] Failed to connect to any available network");
    return false;
}

void WiFiConnectivity::disconnect() {
    LOG_DEBUG("[WiFiConnectivity] Disconnecting from WiFi");
    WiFi.disconnect(true); // true = turn off WiFi radio
    connectedSSID = "";
}

bool WiFiConnectivity::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

std::string WiFiConnectivity::getConnectedSSID() {
    if (isConnected()) {
        return WiFi.SSID().c_str();
    }
    return "";
}

std::vector<std::string> WiFiConnectivity::scanNetworks() {
    std::vector<std::string> results;
    
    LOG_DEBUG("[WiFiConnectivity] Starting WiFi scan...");
    int numNetworks = WiFi.scanNetworks(false, true); // async scan, show hidden
    
    LOG_INFO("[WiFiConnectivity] Found %d networks", numNetworks);
    
    for (int i = 0; i < numNetworks; i++) {
        results.push_back(WiFi.SSID(i).c_str());
        LOG_DEBUG("[WiFiConnectivity] [%d] %s (RSSI: %d)", 
                  i, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    }
    
    return results;
}

int32_t WiFiConnectivity::getSignalStrength() {
    if (isConnected()) {
        return WiFi.RSSI();
    }
    return 0;
}

void WiFiConnectivity::setWiFiEnabled(bool enabled) {
    wifiEnabled = enabled;
    LOG_INFO("[WiFiConnectivity] WiFi %s", enabled ? "enabled" : "disabled");
    
    if (!enabled) {
        disconnect();
    }
}

bool WiFiConnectivity::isWiFiEnabled() {
    return wifiEnabled;
}
