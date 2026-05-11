#pragma once

#include <Arduino.h>
#include <Wire.h>

// SEN66 I2C address (fixed)
#define SEN66_I2C_ADDR 0x6B

// SEN66 Commands (from Sensirion datasheet)
#define SEN66_CMD_START_MEASUREMENT     0x0021
#define SEN66_CMD_STOP_MEASUREMENT      0x0104
#define SEN66_CMD_READ_MEASURED_VALUES  0x0300  // Read as integers
#define SEN66_CMD_GET_DATA_READY        0x0202
#define SEN66_CMD_GET_SERIAL_NUMBER     0xD033
#define SEN66_CMD_RESET                 0xD304
#define SEN66_CMD_GET_VERSION           0xD100

// Timing constants
#define SEN66_MEASUREMENT_DELAY_MS      1100    // Time to wait after starting measurement
#define SEN66_RESET_DELAY_MS            100     // Time to wait after reset

/**
 * @brief SEN66 Air Quality Sensor Data Structure
 * 
 * Contains all air quality parameters measurable by the SEN66 sensor
 */
struct SEN66Data {
    float massConcentrationPm1p0;    // PM1.0 [μg/m³]
    float massConcentrationPm2p5;    // PM2.5 [μg/m³]
    float massConcentrationPm4p0;    // PM4.0 [μg/m³]
    float massConcentrationPm10p0;   // PM10.0 [μg/m³]
    float ambientHumidity;           // Relative Humidity [%]
    float ambientTemperature;        // Temperature [°C]
    float vocIndex;                  // VOC Index (0-500)
    float noxIndex;                  // NOx Index (0-500)
    float co2Equivalent;             // CO2 equivalent [ppm]
};

/**
 * @brief SEN66 Air Quality Sensor Driver
 * 
 * Driver for the Sensirion SEN66 all-in-one environmental sensor
 * Supports PM1.0, PM2.5, PM4.0, PM10.0, Temperature, Humidity, VOC, and NOx measurements
 */
class SEN66Driver {
public:
    SEN66Driver();
    
    /**
     * @brief Initialize the SEN66 sensor
     * @param wire Pointer to the I2C Wire interface (default: &Wire)
     * @return true if initialization successful, false otherwise
     */
    bool begin(TwoWire *wire = &Wire);
    
    /**
     * @brief Check if sensor is connected and responding
     * @return true if sensor is detected, false otherwise
     */
    bool isConnected();
    
    /**
     * @brief Start continuous measurement mode
     * @return true if command successful, false otherwise
     */
    bool startMeasurement();
    
    /**
     * @brief Stop continuous measurement mode
     * @return true if command successful, false otherwise
     */
    bool stopMeasurement();
    
    /**
     * @brief Read all measured values from the sensor
     * @param data Reference to SEN66Data structure to store results
     * @return true if read successful, false otherwise
     */
    bool readMeasuredValues(SEN66Data &data);
    
    /**
     * @brief Get sensor serial number
     * @param serialNumber Buffer to store serial number (min 32 bytes)
     * @return true if successful, false otherwise
     */
    bool getSerialNumber(char *serialNumber);
    
    /**
     * @brief Reset the sensor
     * @return true if successful, false otherwise
     */
    bool reset();

private:
    TwoWire *_wire;
    bool _initialized;
    bool _measurementStarted;
    
    /**
     * @brief Calculate CRC-8 checksum for I2C communication
     * @param data Pointer to data array
     * @param len Length of data
     * @return CRC-8 checksum value
     */
    uint8_t calculateCRC(const uint8_t *data, size_t len);
    
    /**
     * @brief Write command to sensor
     * @param cmd 16-bit command code
     * @return true if successful, false otherwise
     */
    bool writeCommand(uint16_t cmd);
    
    /**
     * @brief Read bytes from sensor with CRC checking
     * @param buffer Buffer to store read data
     * @param len Number of bytes to read (including CRC bytes)
     * @return true if successful and CRC valid, false otherwise
     */
    bool readBytes(uint8_t *buffer, size_t len);
    
    /**
     * @brief Convert two bytes to float (Sensirion format)
     * @param msb Most significant byte
     * @param lsb Least significant byte
     * @return Converted float value
     */
    float bytesToFloat(uint8_t msb, uint8_t lsb, float scaleFactor);
};
