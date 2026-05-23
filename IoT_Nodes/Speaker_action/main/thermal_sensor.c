#include "thermal_sensor.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <stdbool.h>

#define I2C_MASTER_SDA_IO      18//21
#define I2C_MASTER_SCL_IO      19//22
#define I2C_MASTER_NUM         I2C_NUM_0
#define I2C_MASTER_FREQ_HZ     100000
#define I2C_MASTER_TIMEOUT_MS  1000

#define MLX90614_ADDR          0x5A
#define MLX90614_TA_REG        0x06
#define MLX90614_TOBJ1_REG     0x07

static const char *TAG = "THERMAL";

// --------------------------------------------------
// I2C Initialization
// --------------------------------------------------
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// --------------------------------------------------
// Low-level MLX read
// --------------------------------------------------
static float mlx90614_read(uint8_t reg)
{
    uint8_t data[3] = {0};

    esp_err_t ret = i2c_master_write_read_device(
        I2C_MASTER_NUM,
        MLX90614_ADDR,
        &reg, 1,
        data, 3,
        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS)
    );

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C read failed");
        return -1000.0;  // Error value
    }

    uint16_t raw = (data[1] << 8) | data[0];

    // Conversion formula from datasheet
    float temp = (raw * 0.02) - 273.15;

    return temp;
}

// --------------------------------------------------
// Public APIs
// --------------------------------------------------

esp_err_t thermal_init(void)
{
    return i2c_master_init();
}

float thermal_read_ambient(void)
{
    return mlx90614_read(MLX90614_TA_REG);
}

float thermal_read_object(void)
{
    return mlx90614_read(MLX90614_TOBJ1_REG);
}

bool thermal_detect_presence(float threshold_deg)
{
    float ambient = thermal_read_ambient();
    float object  = thermal_read_object();

    if (ambient < -100 || object < -100)
        return false;

    if ((object - ambient) > threshold_deg)
        return true;

    return false;
}