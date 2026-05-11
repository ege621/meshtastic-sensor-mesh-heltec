#include "WiFiConfig.h"
#include <Preferences.h>
#include <Arduino.h>
#include <algorithm>
#include "configuration.h"

// Initialize static members
std::vector<WiFiNetwork> WiFiConfigManager::networks;
bool WiFiConfigManager::initialized = false;

// Preferences instance for NVS access
static Preferences preferences;

void WiFiConfigManager::init() {
    if (initialized) {
        return;
    }
    
    LOG_INFO("[WiFiConfig] Initializing WiFi Configuration Manager");
    loadFromNVS();
    initialized = true;
    
    LOG_INFO("[WiFiConfig] Loaded %d WiFi networks from NVS", networks.size());
    printNetworks();
}

bool WiFiConfigManager::addNetwork(const char* ssid, const char* password) {
    if (!ssid || !password) {
        LOG_WARN("[WiFiConfig] Invalid SSID or password (null pointer)");
        return false;
    }
    
    if (networks.size() >= WIFICONFIG_MAX_NETWORKS) {
        LOG_WARN("[WiFiConfig] Maximum number of networks (%d) reached", WIFICONFIG_MAX_NETWORKS);
        return false;
    }
    
    // Check if network already exists
    for (const auto& net : networks) {
        if (net.ssid == ssid) {
            LOG_WARN("[WiFiConfig] Network %s already exists", ssid);
            return false;
        }
    }
    
    networks.emplace_back(ssid, password);
    LOG_INFO("[WiFiConfig] Added network: %s", ssid);
    
    return saveToNVS();
}

bool WiFiConfigManager::removeNetwork(const char* ssid) {
    if (!ssid) {
        LOG_WARN("[WiFiConfig] Invalid SSID (null pointer)");
        return false;
    }
    
    auto it = std::find_if(networks.begin(), networks.end(),
                          [ssid](const WiFiNetwork& net) { return net.ssid == ssid; });
    
    if (it == networks.end()) {
        LOG_WARN("[WiFiConfig] Network %s not found", ssid);
        return false;
    }
    
    networks.erase(it);
    LOG_INFO("[WiFiConfig] Removed network: %s", ssid);
    
    return saveToNVS();
}

const std::vector<WiFiNetwork>& WiFiConfigManager::getNetworks() {
    return networks;
}

size_t WiFiConfigManager::getNetworkCount() {
    return networks.size();
}

WiFiNetwork* WiFiConfigManager::getNetworkByIndex(size_t index) {
    if (index >= networks.size()) {
        LOG_WARN("[WiFiConfig] Index %d out of bounds (max: %d)", index, networks.size() - 1);
        return nullptr;
    }
    
    return &networks[index];
}

WiFiNetwork* WiFiConfigManager::getNetworkBySSID(const char* ssid) {
    if (!ssid) {
        return nullptr;
    }
    
    for (auto& net : networks) {
        if (net.ssid == ssid) {
            return &net;
        }
    }
    
    LOG_DEBUG("[WiFiConfig] Network %s not found", ssid);
    return nullptr;
}

bool WiFiConfigManager::clearAllNetworks() {
    networks.clear();
    LOG_INFO("[WiFiConfig] All networks cleared");
    
    return saveToNVS();
}

bool WiFiConfigManager::saveToNVS() {
    try {
        preferences.begin(NVS_NAMESPACE, false); // false = read-write mode
        
        // Save network count
        preferences.putUInt(NVS_COUNT_KEY, networks.size());
        
        // Save each network
        for (size_t i = 0; i < networks.size(); i++) {
            char ssid_key[32];
            char pass_key[32];
            
            snprintf(ssid_key, sizeof(ssid_key), "%s%d", NVS_SSID_KEY_PREFIX, i);
            snprintf(pass_key, sizeof(pass_key), "%s%d", NVS_PASS_KEY_PREFIX, i);
            
            preferences.putString(ssid_key, networks[i].ssid.c_str());
            preferences.putString(pass_key, networks[i].password.c_str());
        }
        
        preferences.end();
        LOG_DEBUG("[WiFiConfig] Saved %d networks to NVS", networks.size());
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("[WiFiConfig] Failed to save to NVS: %s", e.what());
        preferences.end();
        return false;
    }
}

bool WiFiConfigManager::loadFromNVS() {
    try {
        networks.clear();
        preferences.begin(NVS_NAMESPACE, true); // true = read-only mode
        
        // Get network count
        uint32_t count = preferences.getUInt(NVS_COUNT_KEY, 0);
        
        // Load each network
        for (size_t i = 0; i < count && i < WIFICONFIG_MAX_NETWORKS; i++) {
            char ssid_key[32];
            char pass_key[32];
            
            snprintf(ssid_key, sizeof(ssid_key), "%s%d", NVS_SSID_KEY_PREFIX, i);
            snprintf(pass_key, sizeof(pass_key), "%s%d", NVS_PASS_KEY_PREFIX, i);
            
            String ssid = preferences.getString(ssid_key, "");
            String password = preferences.getString(pass_key, "");
            
            if (ssid.length() > 0) {
                networks.emplace_back(ssid.c_str(), password.c_str());
            }
        }
        
        preferences.end();
        LOG_DEBUG("[WiFiConfig] Loaded %d networks from NVS", networks.size());
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("[WiFiConfig] Failed to load from NVS: %s", e.what());
        preferences.end();
        return false;
    }
}

void WiFiConfigManager::printNetworks() {
    LOG_INFO("[WiFiConfig] ===== Stored WiFi Networks =====");
    
    if (networks.empty()) {
        LOG_INFO("[WiFiConfig] No networks configured");
        return;
    }
    
    for (size_t i = 0; i < networks.size(); i++) {
        LOG_INFO("[WiFiConfig] [%d] SSID: %s | Password: %s", 
                 i, networks[i].ssid.c_str(), networks[i].password.c_str());
    }
    
    LOG_INFO("[WiFiConfig] ================================");
}
