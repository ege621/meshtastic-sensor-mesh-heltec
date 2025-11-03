#pragma once

#include "SinglePortModule.h"
#include "concurrency/OSThread.h"
#include "configuration.h"
#include "Observer.h"
#include <Adafruit_SHT31.h>
#include <map>
#include <string>

struct RemoteSensorData {
    float temperature;
    float humidity;
    uint32_t receivedTime;
    std::string nodeId;
};

class ADCUplinkModule : public SinglePortModule, private concurrency::OSThread, public Observable<const UIFrameEvent *>
{
    bool firstTime = true;
    Adafruit_SHT31 sht31;
    bool sht31Available = false;
    
    // Store latest sensor readings for OLED display
    float lastTemperature = 0.0f;
    float lastHumidity = 0.0f;
    uint32_t lastReadingTime = 0;
    uint32_t lastBroadcastTime = 0;  // Track when we last broadcasted
    
    // Store remote sensor data from other nodes
    std::map<uint32_t, RemoteSensorData> remoteSensors;
    
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
    // SHT31 I2C Configuration
    // For Heltec V3: I2C is on GPIO17 (SDA) and GPIO18 (SCL) by default
    // SHT31 default I2C address is 0x44
    static constexpr uint8_t SHT31_I2C_ADDR = 0x44;
    
    // Timing Configuration
    static constexpr uint8_t HOPS            = 3;      // mesh hop limit
    static constexpr uint32_t UPDATE_MS      = 1000;   // read sensor every 1s (for display)
    static constexpr uint32_t BROADCAST_MS   = 30000;  // broadcast every 30s
    
    void makeShortId(char out[8]);
    void sendJsonOverMesh(const char* json, size_t len);
    bool initSHT31();
    bool readSHT31(float &temp, float &humidity);
};

extern ADCUplinkModule *adcUplinkModule;
