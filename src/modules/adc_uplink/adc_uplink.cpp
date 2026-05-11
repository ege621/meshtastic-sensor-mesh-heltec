#include "adc_uplink.h"
#include "MeshService.h"
#include "target_specific.h"  // for getMacAddr()
#include "sen66_driver.h"
#include "modules/WiFiConfig/WiFiConnectivity.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

#if !defined(MESHTASTIC_EXCLUDE_SCREEN) && HAS_SCREEN
#include "graphics/ScreenFonts.h"
#include "graphics/SharedUIDisplay.h"
#include <OLEDDisplay.h>
#endif

ADCUplinkModule *adcUplinkModule;

ADCUplinkModule::ADCUplinkModule() 
    : SinglePortModule("ADCUplink", meshtastic_PortNum_TEXT_MESSAGE_APP), concurrency::OSThread("ADCUplinkModule")
{
    // Sensors will be initialized in runOnce on first run
    activeSensor = SensorType::NONE;
}

// Short device ID string from MAC (last two bytes)
void ADCUplinkModule::makeShortId(char out[8]) {
  uint8_t mac[6] = {0};
  getMacAddr(mac);
  // e.g. "ABCD"
  snprintf(out, 8, "%02X%02X", mac[4], mac[5]);
}

void ADCUplinkModule::sendJsonOverMesh(const char* json, size_t len) {
  // Send only as broadcast message (to 0xffffffff)
  // This way it appears in the general chat and won't be filtered out when connected to this device
  LOG_DEBUG("[ADCUplink] sendJsonOverMesh: Broadcasting to mesh, len=%d", len);
  meshtastic_MeshPacket *p = allocDataPacket();
  p->decoded.payload.size = len;
  memcpy(p->decoded.payload.bytes, json, len);
  p->hop_limit = HOPS;
  
  LOG_DEBUG("[ADCUplink] Mesh packet: to=0x%08x, portnum=%d, size=%d", p->to, p->decoded.portnum, p->decoded.payload.size);
  service->sendToMesh(p);
  LOG_DEBUG("[ADCUplink] Sent to mesh broadcast 0xffffffff");
}

void ADCUplinkModule::initWiFiConnectivity() {
  if (wifiInitialized) {
    return;
  }
  
  LOG_INFO("[ADCUplink] Initializing WiFi connectivity (non-blocking)...");
  
  // Initialize WiFi connectivity manager
  WiFiConnectivity::init();
  
  // Add cairtp network if not already present
  if (WiFiConfigManager::getNetworkBySSID("cairtp") == nullptr) {
    LOG_INFO("[ADCUplink] Adding cairtp network credentials");
    if (WiFiConfigManager::addNetwork("cairtp", "corensis")) {
      WiFiConfigManager::saveToNVS();
      LOG_INFO("[ADCUplink] Successfully added cairtp network");
    } else {
      LOG_WARN("[ADCUplink] Failed to add cairtp network");
    }
  } else {
    LOG_DEBUG("[ADCUplink] cairtp network already configured");
  }
  
  wifiInitialized = true;
}

void ADCUplinkModule::queueJsonForWiFi(const char* json, size_t len) {
  // Non-blocking queue: Just copy data to buffer, don't send immediately
  if (len >= sizeof(wifiDataBuffer)) {
    LOG_WARN("[ADCUplink] WiFi data too large, dropping: %d bytes", len);
    return;
  }
  
  // Copy data to buffer for later asynchronous sending
  memcpy(wifiDataBuffer, json, len);
  wifiDataLen = len;
  wifiDataPending = true;
  
  LOG_DEBUG("[ADCUplink] Queued JSON for WiFi transmission (%d bytes)", len);
}

void ADCUplinkModule::processPendingWiFiData() {
  // Non-blocking WiFi processor - called periodically from runOnce()
  // This prevents blocking the mesh operations
  
  if (!wifiDataPending) {
    return;
  }
  
  // Only attempt WiFi sending if WiFi is enabled
  if (!WiFiConnectivity::isWiFiEnabled()) {
    LOG_DEBUG("[ADCUplink] WiFi disabled, skipping WiFi send");
    return;
  }
  
  uint32_t currentTime = millis();
  
  // Try to connect if not already connected (max once per 60 seconds)
  if (!WiFiConnectivity::isConnected()) {
    if (lastWifiAttempt == 0 || (currentTime - lastWifiAttempt) > 60000) {
      lastWifiAttempt = currentTime;
      LOG_INFO("[ADCUplink] Attempting WiFi connection to cairtp...");
      // Non-blocking attempt - returns quickly
      if (!WiFiConnectivity::connectToWiFi(10000)) {
        LOG_WARN("[ADCUplink] Failed to connect to WiFi");
        return;  // Don't clear pending flag, will retry later
      }
    } else {
      // Not yet time to retry connection
      return;
    }
  }
  
  // Only send if we actually have a connection
  if (!WiFiConnectivity::isConnected()) {
    return;
  }
  
  // Limit WiFi send attempts to once per 35 seconds (don't overwhelm network)
  if (lastWifiSendAttempt != 0 && (currentTime - lastWifiSendAttempt) < 35000) {
    return;
  }
  
  // Try to send data over HTTP (non-blocking with short timeout)
  lastWifiSendAttempt = currentTime;
  
  try {
    HTTPClient http;
    String serverURL = "http://192.168.1.100:8080/sensor_data";
    
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);  // 5 second timeout to avoid blocking
    
    // Send POST request
    int httpResponseCode = http.POST((uint8_t*)wifiDataBuffer, wifiDataLen);
    
    if (httpResponseCode > 0) {
      LOG_INFO("[ADCUplink] WiFi POST sent. Response: %d", httpResponseCode);
      wifiDataPending = false;  // Clear the pending flag after successful send
    } else {
      LOG_WARN("[ADCUplink] WiFi POST failed. Code: %d", httpResponseCode);
      // Keep pending flag set, will retry later
    }
    
    http.end();
    
  } catch (const std::exception& e) {
    LOG_WARN("[ADCUplink] WiFi send exception: %s", e.what());
    // Keep pending flag set for retry
  }
}

void ADCUplinkModule::storeSensorData(uint32_t nodeId, const char* sensorJson, const char* shortId) {
  // Parse sensor data - try JSON first
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, sensorJson);
  
  if (!error) {
    // Successfully parsed JSON
    const char* nodeIdStr = doc["id"];
    const char* type = doc["type"];
    float temperature = doc["temp"];
    float humidity = doc["hum"];
    
    if (nodeIdStr && temperature != 0.0f) {
      RemoteSensorData data;
      data.temperature = temperature;
      data.humidity = humidity;
      data.receivedTime = millis();
      data.nodeId = std::string(nodeIdStr);
      
      // Check if this is air quality data
      if (type && strcmp(type, "air_quality") == 0) {
        data.hasAirQuality = true;
        data.pm1p0 = doc["pm1"];
        data.pm2p5 = doc["pm25"];
        data.pm4p0 = doc["pm4"];
        data.pm10p0 = doc["pm10"];
        data.vocIndex = doc["voc"];
        data.noxIndex = doc["nox"];
        data.co2eq = doc["co2"];
        LOG_INFO("Stored air quality data from node %s (PM2.5:%.1f CO2:%.0f)", nodeIdStr, data.pm2p5, data.co2eq);
      } else {
        data.hasAirQuality = false;
        LOG_INFO("Stored sensor data from node %s: %.1f°C, %.1f%%", nodeIdStr, temperature, humidity);
      }
      
      remoteSensors[nodeId] = data;
    }
  } else {
    // JSON parsing failed, try legacy text format for backward compatibility
    LOG_DEBUG("JSON parse failed, trying legacy format");
    
    float temperature = 0.0f;
    float humidity = 0.0f;
    char legacyNodeId[8] = {0};
    bool hasAirQuality = false;
    float pm1p0 = 0.0f, pm2p5 = 0.0f, pm4p0 = 0.0f, pm10p0 = 0.0f;
    float vocIndex = 0.0f, noxIndex = 0.0f;
    float co2eq = 0.0f;
    
    // Extract node ID from [XXXX]
    const char *idStart = strchr(sensorJson, '[');
    const char *idEnd = strchr(sensorJson, ']');
    if (idStart && idEnd && (idEnd - idStart) < 8) {
      strncpy(legacyNodeId, idStart + 1, idEnd - idStart - 1);
      legacyNodeId[idEnd - idStart - 1] = '\0';
    }
    
    // Extract temperature (looking for "Temp: XX.X°C")
    const char *tempStr = strstr(sensorJson, "Temp: ");
    if (tempStr) {
      sscanf(tempStr, "Temp: %f", &temperature);
    }
    
    // Extract humidity (looking for "Humidity: XX.X%")
    const char *humStr = strstr(sensorJson, "Humidity: ");
    if (humStr) {
      sscanf(humStr, "Humidity: %f", &humidity);
    }
    
    // Check if this is air quality data (has PM values)
    const char *pm1Str = strstr(sensorJson, "PM1.0: ");
    if (pm1Str) {
      hasAirQuality = true;
      sscanf(pm1Str, "PM1.0: %f", &pm1p0);
      
      const char *pm2Str = strstr(sensorJson, "PM2.5: ");
      if (pm2Str) sscanf(pm2Str, "PM2.5: %f", &pm2p5);
      
      const char *pm4Str = strstr(sensorJson, "PM4.0: ");
      if (pm4Str) sscanf(pm4Str, "PM4.0: %f", &pm4p0);
      
      const char *pm10Str = strstr(sensorJson, "PM10: ");
      if (pm10Str) sscanf(pm10Str, "PM10: %f", &pm10p0);
      
      const char *vocStr = strstr(sensorJson, "VOC: ");
      if (vocStr) sscanf(vocStr, "VOC: %f", &vocIndex);
      
      const char *noxStr = strstr(sensorJson, "NOx: ");
      if (noxStr) sscanf(noxStr, "NOx: %f", &noxIndex);
      
      const char *co2Str = strstr(sensorJson, "CO2: ");
      if (co2Str) {
        sscanf(co2Str, "CO2: %f", &co2eq);
      }
    }
    
    // Store or update remote sensor data
    if (strlen(legacyNodeId) > 0 && temperature != 0.0f) {
      RemoteSensorData data;
      data.temperature = temperature;
      data.humidity = humidity;
      data.receivedTime = millis();
      data.nodeId = std::string(legacyNodeId);
      data.hasAirQuality = hasAirQuality;
      
      if (hasAirQuality) {
        data.pm1p0 = pm1p0;
        data.pm2p5 = pm2p5;
        data.pm4p0 = pm4p0;
        data.pm10p0 = pm10p0;
        data.vocIndex = vocIndex;
        data.noxIndex = noxIndex;
        data.co2eq = co2eq;
        LOG_INFO("Stored air quality data from node %s", legacyNodeId);
      } else {
        LOG_INFO("Stored sensor data from node %s: %.1f°C, %.1f%%", legacyNodeId, temperature, humidity);
      }
      
      remoteSensors[nodeId] = data;
    }
  }
}

bool ADCUplinkModule::initSensors() {
  LOG_INFO("Initializing I2C sensors...");
  
  // Initialize I2C if not already done (uses default pins for Heltec V3)
  if (!Wire.begin()) {
    LOG_ERROR("Failed to initialize I2C bus");
    return false;
  }
  
  // Try SEN66 first (priority sensor)
  if (initSEN66()) {
    LOG_INFO("SEN66 air quality sensor detected and initialized");
    activeSensor = SensorType::SEN66;
    return true;
  }
  
  // Fall back to SHT31
  if (initSHT31()) {
    LOG_INFO("SHT31 temperature/humidity sensor detected and initialized");
    activeSensor = SensorType::SHT31;
    return true;
  }
  
  // No sensors found
  LOG_ERROR("No supported sensors found on I2C bus");
  activeSensor = SensorType::NONE;
  return false;
}

bool ADCUplinkModule::initSEN66() {
  LOG_INFO("Attempting to initialize SEN66 sensor on I2C address 0x%02X", SEN66_I2C_ADDR);
  
  if (!sen66.begin(&Wire)) {
    LOG_WARN("SEN66 sensor not found or initialization failed");
    return false;
  }
  
  // Start continuous measurement
  if (!sen66.startMeasurement()) {
    LOG_ERROR("Failed to start SEN66 measurement");
    return false;
  }
  
  LOG_INFO("SEN66 sensor initialized successfully");
  return true;
}

bool ADCUplinkModule::initSHT31() {
  LOG_INFO("Attempting to initialize SHT31 sensor on I2C address 0x%02X", SHT31_I2C_ADDR);
  
  // Try to initialize the SHT31 sensor
  if (!sht31.begin(SHT31_I2C_ADDR)) {
    LOG_WARN("SHT31 sensor not found at address 0x%02X", SHT31_I2C_ADDR);
    return false;
  }
  
  LOG_INFO("SHT31 sensor initialized successfully");
  return true;
}

bool ADCUplinkModule::readSHT31(float &temp, float &humidity) {
  temp = sht31.readTemperature();
  humidity = sht31.readHumidity();
  
  // Check if readings are valid
  if (isnan(temp) || isnan(humidity)) {
    LOG_WARN("Failed to read from SHT31 sensor");
    return false;
  }
  
  return true;
}

bool ADCUplinkModule::readSEN66(SEN66Data &data) {
  if (!sen66.readMeasuredValues(data)) {
    LOG_WARN("Failed to read from SEN66 sensor");
    return false;
  }
  
  return true;
}

int32_t ADCUplinkModule::runOnce() {
  // Enable the module only for ESP32 platforms
  #ifndef ARCH_ESP32
    return disable();
  #endif
  
  if (firstTime) {
    firstTime = false;
    LOG_INFO("Environmental Sensor Module started");
    
    // Initialize sensors (SEN66 priority, fallback to SHT31)
    if (!initSensors()) {
      LOG_ERROR("No sensors available. Module disabled.");
      return disable();
    }
    
    // Initialize WiFi connectivity
    initWiFiConnectivity();
    
    // Log which sensor is active
    const char* sensorName = "UNKNOWN";
    switch (activeSensor) {
      case SensorType::SEN66:
        sensorName = "SEN66 (Air Quality)";
        break;
      case SensorType::SHT31:
        sensorName = "SHT31 (Temperature/Humidity)";
        break;
      default:
        sensorName = "NONE";
        break;
    }
    
    // Prepare device ID once
    char shortId[8] = {0};
    makeShortId(shortId);
    LOG_INFO("Device ID: %s | Active Sensor: %s", shortId, sensorName);
    
    // Request focus to make this module the default screen
    requestFocus();
    
    // Notify the screen system to regenerate frames with our module focused
    #if HAS_SCREEN
    UIFrameEvent e;
    e.action = UIFrameEvent::Action::REGENERATE_FRAMESET;
    notifyObservers(&e);
    #endif
  }

  uint32_t currentTime = millis();

  // Read sensor data based on active sensor type
  bool readSuccess = false;
  
  if (activeSensor == SensorType::SEN66) {
    // Read SEN66 air quality sensor
    SEN66Data sen66Data;
    readSuccess = readSEN66(sen66Data);
    
    if (readSuccess) {
      // Store all readings for OLED display
      lastTemperature = sen66Data.ambientTemperature;
      lastHumidity = sen66Data.ambientHumidity;
      lastPM1p0 = sen66Data.massConcentrationPm1p0;
      lastPM2p5 = sen66Data.massConcentrationPm2p5;
      lastPM4p0 = sen66Data.massConcentrationPm4p0;
      lastPM10p0 = sen66Data.massConcentrationPm10p0;
      lastVOCIndex = sen66Data.vocIndex;
      lastNOxIndex = sen66Data.noxIndex;
      lastCO2 = sen66Data.co2Equivalent;
      lastReadingTime = currentTime;
    }
  } else if (activeSensor == SensorType::SHT31) {
    // Read SHT31 temperature/humidity sensor
    float temperature = 0.0f;
    float humidity = 0.0f;
    readSuccess = readSHT31(temperature, humidity);
    
    if (readSuccess) {
      lastTemperature = temperature;
      lastHumidity = humidity;
      lastReadingTime = currentTime;
    }
  }
  
  if (!readSuccess) {
    LOG_WARN("Failed to read sensor data, skipping this cycle");
    return 5000; // Retry after 5 seconds
  }

  // Check if it's time to broadcast (every 30 seconds)
  if (lastBroadcastTime == 0 || (currentTime - lastBroadcastTime) >= BROADCAST_MS) {
    // Prepare device ID for this reading
    char shortId[8] = {0};
    makeShortId(shortId);

    // Format sensor data based on active sensor - use JSON for structured data
    char out[512];
    
    if (activeSensor == SensorType::SEN66) {
      // Format comprehensive air quality data as JSON
      snprintf(out, sizeof(out), 
               "{\"id\":\"%s\",\"type\":\"air_quality\",\"temp\":%.1f,\"hum\":%.1f,"
               "\"pm1\":%.1f,\"pm25\":%.1f,\"pm4\":%.1f,\"pm10\":%.1f,"
               "\"voc\":%.0f,\"nox\":%.0f,\"co2\":%.0f}",
               shortId,
               lastTemperature,
               lastHumidity,
               lastPM1p0,
               lastPM2p5,
               lastPM4p0,
               lastPM10p0,
               lastVOCIndex,
               lastNOxIndex,
               lastCO2);
    } else if (activeSensor == SensorType::SHT31) {
      // Format simple temperature/humidity data as JSON
      snprintf(out, sizeof(out), 
               "{\"id\":\"%s\",\"type\":\"sensor\",\"temp\":%.1f,\"hum\":%.1f}",
               shortId,
               lastTemperature,
               lastHumidity);
    }
    
    size_t n = strlen(out);
    if (n > 0) {
      // Send to mesh (primary transport)
      sendJsonOverMesh(out, n);
      LOG_INFO("Sent to Mesh: %s", out);
      
      // Queue for WiFi sending (non-blocking - doesn't delay mesh operations)
      queueJsonForWiFi(out, n);
      
      // Also store locally so this device can see its own data
      // Use a fixed node ID for the local device (0xFFFFFFFF represents self)
      storeSensorData(0xFFFFFFFF, out, shortId);
      
      lastBroadcastTime = currentTime;
    }
  }
  
  // Process pending WiFi data asynchronously (non-blocking)
  // This runs every cycle but won't block mesh operations
  processPendingWiFiData();

  // Return UPDATE_MS (1 second) to read sensor frequently for display updates
  return UPDATE_MS;
}

ProcessMessage ADCUplinkModule::handleReceived(const meshtastic_MeshPacket &mp) {
  // Only process messages on our port
  LOG_DEBUG("[ADCUplink] handleReceived: portnum=%d, from=0x%08x, to=0x%08x", mp.decoded.portnum, getFrom(&mp), mp.to);
  
  if (mp.decoded.portnum != meshtastic_PortNum_TEXT_MESSAGE_APP) {
    LOG_DEBUG("[ADCUplink] handleReceived: Ignoring portnum %d (not TEXT_MESSAGE_APP=1)", mp.decoded.portnum);
    return ProcessMessage::CONTINUE;
  }

  // Extract the received sensor data
  const char* payload = (const char*)mp.decoded.payload.bytes;
  size_t payloadLen = mp.decoded.payload.size;
  
  LOG_DEBUG("[ADCUplink] handleReceived: payload size=%d", payloadLen);
  
  // Create a null-terminated string for safe processing
  char dataStr[512];
  if (payloadLen < sizeof(dataStr)) {
    memcpy(dataStr, payload, payloadLen);
    dataStr[payloadLen] = '\0';
    
    // Get sender ID
    uint32_t fromNode = getFrom(&mp);
    char senderStr[16];
    snprintf(senderStr, sizeof(senderStr), "%08X", fromNode);
    
    LOG_INFO("Received sensor data from %s: %s", senderStr, dataStr);
    LOG_DEBUG("[ADCUplink] handleReceived: Storing data from node 0x%08x", fromNode);
    
    // Use helper method to store sensor data
    char shortId[8] = {0};
    makeShortId(shortId);
    storeSensorData(fromNode, dataStr, shortId);
  } else {
    LOG_WARN("Received sensor data too large to display (%d bytes)", payloadLen);
  }

  return ProcessMessage::CONTINUE;
}

#if !defined(MESHTASTIC_EXCLUDE_SCREEN) && HAS_SCREEN
void ADCUplinkModule::drawFrame(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(FONT_SMALL);
  
  // Draw header
  const char *titleStr = (activeSensor == SensorType::SEN66) ? "Air Quality" : "Sensors";
  graphics::drawCommonHeader(display, x, y, titleStr);
  
  int line = 1;
  int currentY = graphics::getTextPositions(display)[line++];
  
  // Get local device ID
  char shortId[8] = {0};
  makeShortId(shortId);
  
  if (activeSensor == SensorType::NONE) {
    display->drawString(x, currentY, "No Sensor Found");
    return;
  }
  
  // Show local sensor data
  uint32_t agoSecs = (millis() - lastReadingTime) / 1000;
  
  // Local sensor header
  char localHeader[32];
  snprintf(localHeader, sizeof(localHeader), "[%s] %ds ago", shortId, (int)agoSecs);
  display->drawString(x, currentY, localHeader);
  currentY += FONT_HEIGHT_SMALL;
  
  if (activeSensor == SensorType::SEN66) {
    // Display air quality data (compact format)
    char line1[32];
  snprintf(line1, sizeof(line1), "%.1fC %.0f%% VOC:%.0f CO2:%.0fppm", 
       lastTemperature, lastHumidity, lastVOCIndex, lastCO2);
    display->drawString(x, currentY, line1);
    currentY += FONT_HEIGHT_SMALL;
    
    char line2[32];
    snprintf(line2, sizeof(line2), "PM2.5:%.1f PM10:%.1f", 
             lastPM2p5, lastPM10p0);
    display->drawString(x, currentY, line2);
    currentY += FONT_HEIGHT_SMALL + 2;
    
  } else if (activeSensor == SensorType::SHT31) {
    // Display temperature and humidity only
    char localData[32];
    snprintf(localData, sizeof(localData), "%.1f°C  %.0f%%", lastTemperature, lastHumidity);
    display->drawString(x, currentY, localData);
    currentY += FONT_HEIGHT_SMALL + 2;
  }
  
  // Show remote sensors
  if (!remoteSensors.empty()) {
    display->drawString(x, currentY, "--- Other Nodes ---");
    currentY += FONT_HEIGHT_SMALL;
    
    int nodeCount = 0;
    for (auto it = remoteSensors.begin(); it != remoteSensors.end() && nodeCount < 2; ++it, ++nodeCount) {
      const RemoteSensorData &data = it->second;
      uint32_t remoteAgoSecs = (millis() - data.receivedTime) / 1000;
      
      // Remote node header
      char remoteHeader[32];
      snprintf(remoteHeader, sizeof(remoteHeader), "[%s] %ds ago", data.nodeId.c_str(), (int)remoteAgoSecs);
      display->drawString(x, currentY, remoteHeader);
      currentY += FONT_HEIGHT_SMALL;
      
      if (data.hasAirQuality) {
        // Show air quality data (PM2.5 and CO2)
        char remoteData[48];
        snprintf(remoteData, sizeof(remoteData), "PM2.5:%.1f CO2:%.0fppm", data.pm2p5, data.co2eq);
        display->drawString(x, currentY, remoteData);
      } else {
        // Show temperature/humidity
        char remoteData[32];
        snprintf(remoteData, sizeof(remoteData), "%.1f°C  %.0f%%", data.temperature, data.humidity);
        display->drawString(x, currentY, remoteData);
      }
      currentY += FONT_HEIGHT_SMALL;
    }
    
    if (remoteSensors.size() > 2) {
      char moreStr[32];
      snprintf(moreStr, sizeof(moreStr), "...+%d more", (int)(remoteSensors.size() - 2));
      display->drawString(x, currentY, moreStr);
    }
  }
}
#endif