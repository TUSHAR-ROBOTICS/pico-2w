#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// BMP280 I2C address is usually 0x76 (or 0x77 if SDO is tied high)
#define BMP280_ADDR _u(0x76)

// Hardware registers
#define REG_CONFIG                  _u(0xF5)
#define REG_CTRL_MEAS               _u(0xF4)
#define REG_STATUS                  _u(0xF3)
#define REG_RESET                   _u(0xE0)
#define REG_ID                      _u(0xD0)
#define REG_CALIB_FIRST             _u(0x88)
#define REG_DIG_T1                  _u(0x88)

// I2C pins configuration
#define I2C_PORT i2c0
#define PIN_SDA 4
#define PIN_SCL 5

// Calibration coefficients stored on the sensor chip
uint16_t dig_T1;
int16_t  dig_T2, dig_T3;
uint16_t dig_P1;
int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
int32_t t_fine;

void bmp280_read_calib_params() {
    uint8_t buf[24];
    uint8_t reg = REG_CALIB_FIRST;
    i2c_write_blocking(I2C_PORT, BMP280_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, BMP280_ADDR, buf, 24, false);

    dig_T1 = (buf[1] << 8) | buf[0];
    dig_T2 = (buf[3] << 8) | buf[2];
    dig_T3 = (buf[5] << 8) | buf[4];

    dig_P1 = (buf[7] << 8) | buf[6];
    dig_P2 = (buf[9] << 8) | buf[8];
    dig_P3 = (buf[11] << 8) | buf[10];
    dig_P4 = (buf[13] << 8) | buf[12];
    dig_P5 = (buf[15] << 8) | buf[14];
    dig_P6 = (buf[17] << 8) | buf[16];
    dig_P7 = (buf[19] << 8) | buf[18];
    dig_P8 = (buf[21] << 8) | buf[20];
    dig_P9 = (buf[23] << 8) | buf[22];
}

void bmp280_init() {
    // Configure: Temperature oversampling x1, Pressure oversampling x1, Normal mode
    uint8_t config[2] = {REG_CTRL_MEAS, 0x27};
    i2c_write_blocking(I2C_PORT, BMP280_ADDR, config, 2, false);
    
    // Read compensation coefficients
    bmp280_read_calib_params();
}

// Compensation formula provided by Bosch datasheet
int32_t bmp280_compensate_T(int32_t adc_T) {
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

// Compensation formula provided by Bosch datasheet
uint32_t bmp280_compensate_P(int32_t adc_P) {
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;
    
    if (var1 == 0) {
        return 0; // avoid exception caused by division by zero
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
    return (uint32_t)p;
}

void bmp280_read_raw(int32_t *p_adc, int32_t *t_adc) {
    uint8_t buf[6];
    uint8_t reg = 0xF7;
    i2c_write_blocking(I2C_PORT, BMP280_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, BMP280_ADDR, buf, 6, false);

    *p_adc = (buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4);
    *t_adc = (buf[3] << 12) | (buf[4] << 4) | (buf[5] >> 4);
}

int main() {
    stdio_init_all();
    
    // Initialize I2C at 100 kHz
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);

    sleep_ms(1000); // Give the system a second to start serial monitoring
    printf("Initializing BMP280 sensor...\n");
    bmp280_init();

    int32_t raw_pressure, raw_temperature;

    while (true) {
        bmp280_read_raw(&raw_pressure, &raw_temperature);
        
        int32_t comp_target_T = bmp280_compensate_T(raw_temperature);
        uint32_t comp_target_P = bmp280_compensate_P(raw_pressure);

        // Convert data into human-readable floats
        float temperature = comp_target_T / 100.0f;
        float pressure = comp_target_P / 256.0f; // Pressure in Pa

        printf("Temperature: %.2f °C | Pressure: %.2f hPa\n", temperature, pressure / 100.0f);
        
        sleep_ms(1000);
    }
}