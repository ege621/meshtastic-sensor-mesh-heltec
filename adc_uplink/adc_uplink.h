#pragma once

#include "SinglePortModule.h"
#include "concurrency/OSThread.h"
#include "configuration.h"

class ADCUplinkModule : public SinglePortModule, private concurrency::OSThread
{
    bool firstTime = true;
    
  public:
    ADCUplinkModule();

  protected:
    virtual int32_t runOnce() override;
    
  private:
    // For Heltec V3: GPIO2, GPIO3, GPIO4, GPIO5, GPIO6, GPIO7 are available ADC pins
    // Avoid GPIO1 as it's used for battery monitoring
    static constexpr int   ADC_PIN      = 2;        // GPIO2 is ADC1_CH1 on ESP32-S3
    static constexpr int   ADC_BITS     = 12;       // ESP32-S3 default (0..4095)
    static constexpr float VREF_VOLTS   = 3.30f;    // adjust if you calibrate
    static constexpr int   SAMPLE_COUNT = 16;       // average to reduce noise
    static constexpr uint8_t HOPS       = 3;        // mesh hop limit
    static constexpr uint32_t PERIOD_MS = 10000;    // send every 10s
    
    void makeShortId(char out[8]);
    void sendJsonOverMesh(const char* json, size_t len);
    uint16_t readAdcAveraged();
};

extern ADCUplinkModule *adcUplinkModule;
