#pragma once

#include "SinglePortModule.h"
#include "concurrency/OSThread.h"
#include "configuration.h"
#include "Observer.h"
#include <Adafruit_SHT31.h>
#include "sen66_driver.h"
#include "modules/WiFiConfig/WiFiConnectivity.h"
#include <map>
#include <string>

// Sensor type enumeration
enum class SensorType {
    NONE = 0,
    SHT31 = 1,
    SEN66 = 2
};

struct RemoteSensorData {
    float temperature;
    float humidity;
    uint32_t receivedTime;
    std::string nodeId;
    
    // Air quality parameters (for SEN66)
    bool hasAirQuality;
    float pm1p0;
    float pm2p5;
    float pm4p0;
    float pm10p0;
    float vocIndex;
    float noxIndex;
  float co2eq;
};

class ADCUplinkModule : public SinglePortModule, private concurrency::OSThread, public Observable<const UIFrameEvent *>
{
    bool firstTime = true;
    
    // Sensor objects
    Adafruit_SHT31 sht31;
    SEN66Driver sen66;
    
    // Active sensor type
    SensorType activeSensor = SensorType::NONE;
    
    // Store latest sensor readings for OLED display
    float lastTemperature = 0.0f;
    float lastHumidity = 0.0f;
    
    // Air quality data (SEN66 only)
    float lastPM1p0 = 0.0f;
    float lastPM2p5 = 0.0f;
    float lastPM4p0 = 0.0f;
    float lastPM10p0 = 0.0f;
    float lastVOCIndex = 0.0f;
    float lastNOxIndex = 0.0f;
  float lastCO2 = 0.0f;
    
    uint32_t lastReadingTime = 0;
    uint32_t lastBroadcastTime = 0;  // Track when we last broadcasted
    
    // Store remote sensor data from other nodes
    std::map<uint32_t, RemoteSensorData> remoteSensors;
    
    // WiFi connectivity status (non-blocking)
    bool wifiInitialized = false;
    uint32_t lastWifiAttempt = 0;
    uint32_t lastWifiSendAttempt = 0;
    
    // Buffer for WiFi data queue (non-blocking)
    char wifiDataBuffer[512] = {0};
    size_t wifiDataLen = 0;
    bool wifiDataPending = false;
    
  public:
    ADCUplinkModule();

    #if !defined(MESHTASTIC_EXCLUDE_SCREEN) && HAS_SCREEN
    // Tell the screen system we want a UI frame
    virtual bool wantUIFrame() override { return true; }
    
    // Draw sensor data on OLED
    void drawFrame(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
    #endif

  protected:
    virtual int32_t runOnce() override;
    virtual ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;
    
  private:
    // I2C Configuration
    // For Heltec V3: I2C is on GPIO17 (SDA) and GPIO18 (SCL) by default
    static constexpr uint8_t SHT31_I2C_ADDR = 0x44;
    
    // Timing Configuration
    static constexpr uint8_t HOPS            = 3;      // mesh hop limit
    static constexpr uint32_t UPDATE_MS      = 1000;   // read sensor every 1s (for display)
    static constexpr uint32_t BROADCAST_MS   = 30000;  // broadcast every 30s
    
    void makeShortId(char out[8]);
    void sendJsonOverMesh(const char* json, size_t len);
    void queueJsonForWiFi(const char* json, size_t len);
    void processPendingWiFiData();
    void initWiFiConnectivity();
    void storeSensorData(uint32_t nodeId, const char* sensorJson, const char* shortId);
    
    // Sensor initialization and reading methods
    bool initSensors();
    bool initSHT31();
    bool initSEN66();
    bool readSHT31(float &temp, float &humidity);
    bool readSEN66(SEN66Data &data);
};

extern ADCUplinkModule *adcUplinkModule;
