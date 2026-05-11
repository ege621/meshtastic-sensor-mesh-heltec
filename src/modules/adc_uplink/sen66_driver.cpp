#include "sen66_driver.h"

// CRC polynomial for Sensirion sensors: x^8 + x^5 + x^4 + 1 (0x31)
#define CRC8_POLYNOMIAL 0x31
#define CRC8_INIT 0xFF

SEN66Driver::SEN66Driver() {
    _wire = nullptr;
    _initialized = false;
    _measurementStarted = false;
}

bool SEN66Driver::begin(TwoWire *wire) {
    _wire = wire;
    
    if (!_wire) {
        return false;
    }
    
    // Check if sensor is present
    if (!isConnected()) {
        return false;
    }
    
    // Do NOT reset during begin() - takes too long and may cause issues
    // User can call reset() manually if needed
    
    _initialized = true;
    return true;
}

bool SEN66Driver::isConnected() {
    if (!_wire) {
        return false;
    }
    
    _wire->beginTransmission(SEN66_I2C_ADDR);
    return (_wire->endTransmission() == 0);
}

bool SEN66Driver::startMeasurement() {
    if (!_initialized) {
        return false;
    }
    
    if (writeCommand(SEN66_CMD_START_MEASUREMENT)) {
        _measurementStarted = true;
        delay(SEN66_MEASUREMENT_DELAY_MS); // Wait for first measurement
        return true;
    }
    
    return false;
}

bool SEN66Driver::stopMeasurement() {
    if (!_initialized) {
        return false;
    }
    
    if (writeCommand(SEN66_CMD_STOP_MEASUREMENT)) {
        _measurementStarted = false;
        return true;
    }
    
    return false;
}

bool SEN66Driver::readMeasuredValues(SEN66Data &data) {
    if (!_initialized || !_measurementStarted) {
        return false;
    }
    
    // Send read command
    if (!writeCommand(SEN66_CMD_READ_MEASURED_VALUES)) {
        return false;
    }
    
    delay(20); // Short delay for sensor to prepare data
    
    // Read 27 bytes (9 values: 4 uint16 + 2 int16 + 2 int16 + 1 uint16)
    // Each 2-byte value has 1 CRC byte: 9 * 3 = 27 bytes
    uint8_t buffer[27];
    if (!readBytes(buffer, 27)) {
        return false;
    }
    
    // Parse data (each value is 2 bytes + 1 CRC byte)
    // Data format from SEN66: PM1.0, PM2.5, PM4.0, PM10.0, RH, T, VOC Index, NOx Index, CO2
    // Scale factors from Sensirion library:
    // PM: factor 10 (μg/m³ = value / 10)
    // RH: factor 100 (% = value / 100)
    // T: factor 200 (°C = value / 200)
    // VOC/NOx: factor 10 (index = value / 10)
    
    // PM values are uint16
    data.massConcentrationPm1p0 = bytesToFloat(buffer[0], buffer[1], 10.0);
    data.massConcentrationPm2p5 = bytesToFloat(buffer[3], buffer[4], 10.0);
    data.massConcentrationPm4p0 = bytesToFloat(buffer[6], buffer[7], 10.0);
    data.massConcentrationPm10p0 = bytesToFloat(buffer[9], buffer[10], 10.0);
    
    // Temperature and humidity are int16 (can be negative)
    data.ambientHumidity = bytesToFloat(buffer[12], buffer[13], 100.0);
    data.ambientTemperature = bytesToFloat(buffer[15], buffer[16], 200.0);
    
    // VOC and NOx indices are int16
    data.vocIndex = bytesToFloat(buffer[18], buffer[19], 10.0);
    data.noxIndex = bytesToFloat(buffer[21], buffer[22], 10.0);
    // CO2 equivalent (ppm) - treat as raw integer (scale 1)
    data.co2Equivalent = bytesToFloat(buffer[24], buffer[25], 1.0);
    
    return true;
}

bool SEN66Driver::getSerialNumber(char *serialNumber) {
    if (!_initialized || !serialNumber) {
        return false;
    }
    
    // Send get serial number command
    if (!writeCommand(SEN66_CMD_GET_SERIAL_NUMBER)) {
        return false;
    }
    
    delay(20);
    
    // Read 48 bytes (32 ASCII characters, each with CRC)
    uint8_t buffer[48];
    if (!readBytes(buffer, 48)) {
        return false;
    }
    
    // Extract serial number (skip CRC bytes)
    for (int i = 0; i < 32; i++) {
        int bufferIdx = i + (i / 2); // Account for CRC bytes
        serialNumber[i] = buffer[bufferIdx];
    }
    serialNumber[32] = '\0';
    
    return true;
}

bool SEN66Driver::reset() {
    if (!_wire) {
        return false;
    }
    
    return writeCommand(SEN66_CMD_RESET);
}

uint8_t SEN66Driver::calculateCRC(const uint8_t *data, size_t len) {
    uint8_t crc = CRC8_INIT;
    
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 8; bit > 0; --bit) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            } else {
                crc = (crc << 1);
            }
        }
    }
    
    return crc;
}

bool SEN66Driver::writeCommand(uint16_t cmd) {
    if (!_wire) {
        return false;
    }
    
    _wire->beginTransmission(SEN66_I2C_ADDR);
    _wire->write((uint8_t)(cmd >> 8));   // MSB
    _wire->write((uint8_t)(cmd & 0xFF)); // LSB
    
    return (_wire->endTransmission() == 0);
}

bool SEN66Driver::readBytes(uint8_t *buffer, size_t len) {
    if (!_wire || !buffer) {
        return false;
    }
    
    // Request bytes from sensor
    size_t bytesRequested = _wire->requestFrom((uint8_t)SEN66_I2C_ADDR, (uint8_t)len);
    
    if (bytesRequested != len) {
        return false;
    }
    
    // Read all bytes
    for (size_t i = 0; i < len; i++) {
        if (_wire->available()) {
            buffer[i] = _wire->read();
        } else {
            return false;
        }
    }
    
    // Verify CRC for each 2-byte data word
    // Format: [MSB] [LSB] [CRC] [MSB] [LSB] [CRC] ...
    for (size_t i = 0; i < len; i += 3) {
        uint8_t data[2] = {buffer[i], buffer[i + 1]};
        uint8_t crc = calculateCRC(data, 2);
        
        if (crc != buffer[i + 2]) {
            // CRC mismatch
            return false;
        }
    }
    
    return true;
}

float SEN66Driver::bytesToFloat(uint8_t msb, uint8_t lsb, float scaleFactor) {
    // Combine bytes to 16-bit signed integer
    int16_t raw = (int16_t)((msb << 8) | lsb);
    
    // Apply scale factor
    return (float)raw / scaleFactor;
}
