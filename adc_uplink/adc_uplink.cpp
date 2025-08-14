#include "adc_uplink.h"
#include "MeshService.h"
#include "target_specific.h"  // for getMacAddr()
#include <Arduino.h>
#include <ArduinoJson.h>

ADCUplinkModule *adcUplinkModule;

ADCUplinkModule::ADCUplinkModule() 
    : SinglePortModule("ADCUplink", meshtastic_PortNum_TEXT_MESSAGE_APP), concurrency::OSThread("ADCUplinkModule")
{
    // Configure ADC
    analogReadResolution(ADC_BITS);
    #if defined(ARDUINO_ARCH_ESP32)
        // 11dB ≈ full 0–3.3V range on ESP32-S3
        analogSetPinAttenuation(ADC_PIN, ADC_11db);
    #endif
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

uint16_t ADCUplinkModule::readAdcAveraged() {
  uint32_t acc = 0;
  for (int i = 0; i < SAMPLE_COUNT; ++i) {
    acc += analogRead(ADC_PIN);
    delayMicroseconds(200);
  }
  return (uint16_t)(acc / SAMPLE_COUNT);
}

int32_t ADCUplinkModule::runOnce() {
  // Enable the module only for ESP32 platforms
  #ifndef ARCH_ESP32
    return disable();
  #endif
  
  if (firstTime) {
    firstTime = false;
    LOG_INFO("ADC Uplink Module started - reading from GPIO%d", ADC_PIN);
    
    // Prepare device ID once
    char shortId[8] = {0};
    makeShortId(shortId);
    LOG_INFO("Device ID: %s", shortId);
  }

  uint32_t t0 = millis();

  // Read ADC and convert to volts
  const uint16_t raw = readAdcAveraged();
  const float volts  = (raw * VREF_VOLTS) / ((1 << ADC_BITS) - 1);

  // Prepare device ID for this reading
  char shortId[8] = {0};
  makeShortId(shortId);

  // {"id":"ABCD","pin":2,"raw":1234,"V":1.23}
  StaticJsonDocument<96> doc;
  doc["id"]  = shortId;
  doc["pin"] = ADC_PIN;
  doc["raw"] = raw;
  doc["V"]   = roundf(volts * 100) / 100.0f;   // 2 decimals

  char out[96];
  size_t n = serializeJson(doc, out, sizeof(out));
  if (n) {
    sendJsonOverMesh(out, n);
    LOG_INFO("Sent sensor data: %s", out);
  }

  // Add jitter to prevent synchronized transmissions
  const uint32_t elapsed = millis() - t0;
  const uint32_t jitter  = esp_random() % 500; // 0–499 ms
  const uint32_t waitMs  = (elapsed >= PERIOD_MS) ? (1000 + jitter)
                                                  : (PERIOD_MS - elapsed + jitter);
  
  return waitMs;
}