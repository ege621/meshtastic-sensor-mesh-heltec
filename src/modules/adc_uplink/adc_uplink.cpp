#include "adc_uplink.h"
#include "MeshService.h"
#include "target_specific.h"  // for getMacAddr()
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>

#if !defined(MESHTASTIC_EXCLUDE_SCREEN) && HAS_SCREEN
#include "graphics/ScreenFonts.h"
#include "graphics/SharedUIDisplay.h"
#include <OLEDDisplay.h>
#endif

ADCUplinkModule *adcUplinkModule;

ADCUplinkModule::ADCUplinkModule() 
    : SinglePortModule("ADCUplink", meshtastic_PortNum_TEXT_MESSAGE_APP), concurrency::OSThread("ADCUplinkModule")
{
    // SHT31 will be initialized in runOnce on first run
    sht31Available = false;
}

// Short device ID string from MAC (last two bytes)
void ADCUplinkModule::makeShortId(char out[8]) {
  uint8_t mac[6] = {0};
  getMacAddr(mac);
  // e.g. "ABCD"
  snprintf(out, 8, "%02X%02X", mac[4], mac[5]);
}

void ADCUplinkModule::sendJsonOverMesh(const char* json, size_t len) {
  meshtastic_MeshPacket *p = allocDataPacket();
  p->decoded.payload.size = len;
  memcpy(p->decoded.payload.bytes, json, len);
  p->hop_limit = HOPS;
  
  service->sendToMesh(p);
}

bool ADCUplinkModule::initSHT31() {
  LOG_INFO("Initializing SHT31 sensor on I2C address 0x%02X", SHT31_I2C_ADDR);
  
  // Initialize I2C if not already done (uses default pins for Heltec V3)
  if (!Wire.begin()) {
    LOG_ERROR("Failed to initialize I2C bus");
    return false;
  }
  
  // Try to initialize the SHT31 sensor
  if (!sht31.begin(SHT31_I2C_ADDR)) {
    LOG_ERROR("Failed to find SHT31 sensor at address 0x%02X", SHT31_I2C_ADDR);
    return false;
  }
  
  LOG_INFO("SHT31 sensor initialized successfully");
  return true;
}

bool ADCUplinkModule::readSHT31(float &temp, float &humidity) {
  if (!sht31Available) {
    return false;
  }
  
  temp = sht31.readTemperature();
  humidity = sht31.readHumidity();
  
  // Check if readings are valid
  if (isnan(temp) || isnan(humidity)) {
    LOG_WARN("Failed to read from SHT31 sensor");
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
    LOG_INFO("SHT31 Sensor Module started");
    
    // Initialize SHT31 sensor
    sht31Available = initSHT31();
    
    if (!sht31Available) {
      LOG_ERROR("SHT31 sensor initialization failed. Module will not send data.");
      return disable();
    }
    
    // Prepare device ID once
    char shortId[8] = {0};
    makeShortId(shortId);
    LOG_INFO("Device ID: %s", shortId);
    
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

  // Read SHT31 sensor data every second
  float temperature = 0.0f;
  float humidity = 0.0f;
  
  if (!readSHT31(temperature, humidity)) {
    LOG_WARN("Failed to read SHT31 sensor, skipping this cycle");
    // Retry after a shorter period
    return 5000; // 5 seconds
  }

  // Store readings for OLED display (updates every second)
  lastTemperature = temperature;
  lastHumidity = humidity;
  lastReadingTime = currentTime;

  // Check if it's time to broadcast (every 30 seconds)
  if (lastBroadcastTime == 0 || (currentTime - lastBroadcastTime) >= BROADCAST_MS) {
    // Prepare device ID for this reading
    char shortId[8] = {0};
    makeShortId(shortId);

    // Format sensor data as human-readable text for Meshtastic app
    char out[256];
    snprintf(out, sizeof(out), 
             "🌡️ Sensor [%s]\nTemp: %.1f°C\nHumidity: %.1f%%",
             shortId,
             temperature,
             humidity);
    
    size_t n = strlen(out);
    if (n > 0) {
      sendJsonOverMesh(out, n);
      LOG_INFO("Sent sensor data: %s", out);
      lastBroadcastTime = currentTime;
    }
  }

  // Return UPDATE_MS (1 second) to read sensor frequently for display updates
  return UPDATE_MS;
}

ProcessMessage ADCUplinkModule::handleReceived(const meshtastic_MeshPacket &mp) {
  // Only process messages on our port
  if (mp.decoded.portnum != meshtastic_PortNum_TEXT_MESSAGE_APP) {
    return ProcessMessage::CONTINUE;
  }

  // Extract the received sensor data
  const char* payload = (const char*)mp.decoded.payload.bytes;
  size_t payloadLen = mp.decoded.payload.size;
  
  // Create a null-terminated string for safe processing
  char dataStr[256];
  if (payloadLen < sizeof(dataStr)) {
    memcpy(dataStr, payload, payloadLen);
    dataStr[payloadLen] = '\0';
    
    // Get sender ID
    uint32_t fromNode = getFrom(&mp);
    char senderStr[16];
    snprintf(senderStr, sizeof(senderStr), "%08X", fromNode);
    
    LOG_INFO("Received sensor data from %s: %s", senderStr, dataStr);
    
    // Try to parse the sensor data (simple parsing of our format)
    // Format: "🌡️ Sensor [ID]\nTemp: XXX°C (XXX°F)\nHumidity: XX%"
    float temperature = 0.0f;
    float humidity = 0.0f;
    char nodeId[8] = {0};
    
    // Extract node ID from [XXXX]
    const char *idStart = strchr(dataStr, '[');
    const char *idEnd = strchr(dataStr, ']');
    if (idStart && idEnd && (idEnd - idStart) < 8) {
      strncpy(nodeId, idStart + 1, idEnd - idStart - 1);
      nodeId[idEnd - idStart - 1] = '\0';
    }
    
    // Extract temperature (looking for "Temp: XX.X°C")
    const char *tempStr = strstr(dataStr, "Temp: ");
    if (tempStr) {
      sscanf(tempStr, "Temp: %f", &temperature);
    }
    
    // Extract humidity (looking for "Humidity: XX.X%")
    const char *humStr = strstr(dataStr, "Humidity: ");
    if (humStr) {
      sscanf(humStr, "Humidity: %f", &humidity);
    }
    
    // Store or update remote sensor data
    if (strlen(nodeId) > 0 && temperature != 0.0f) {
      RemoteSensorData data;
      data.temperature = temperature;
      data.humidity = humidity;
      data.receivedTime = millis();
      data.nodeId = std::string(nodeId);
      
      remoteSensors[fromNode] = data;
      LOG_INFO("Stored sensor data from node %s: %.1f°C, %.1f%%", nodeId, temperature, humidity);
    }
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
  const char *titleStr = "Sensors";
  graphics::drawCommonHeader(display, x, y, titleStr);
  
  int line = 1;
  int currentY = graphics::getTextPositions(display)[line++];
  
  // Get local device ID
  char shortId[8] = {0};
  makeShortId(shortId);
  
  if (!sht31Available) {
    display->drawString(x, currentY, "No Local Sensor");
    return;
  }
  
  // Show local sensor data
  uint32_t agoSecs = (millis() - lastReadingTime) / 1000;
  
  // Local sensor header
  char localHeader[32];
  snprintf(localHeader, sizeof(localHeader), "[%s] %ds ago", shortId, (int)agoSecs);
  display->drawString(x, currentY, localHeader);
  currentY += FONT_HEIGHT_SMALL;
  
  // Local temperature and humidity
  char localData[32];
  snprintf(localData, sizeof(localData), "%.1f°C  %.0f%%", lastTemperature, lastHumidity);
  display->drawString(x, currentY, localData);
  currentY += FONT_HEIGHT_SMALL + 2;
  
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
      
      // Remote temperature and humidity
      char remoteData[32];
      snprintf(remoteData, sizeof(remoteData), "%.1f°C  %.0f%%", data.temperature, data.humidity);
      display->drawString(x, currentY, remoteData);
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