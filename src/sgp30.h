#include <stdio.h>
#include <string.h>

#include <esp_types.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "sensors_readings.h"



/***********************************
    Default I2C address
************************************/
#define SGP30_ADDR  (0x58) 


/****** Commands and constants ******/
#define SGP30_FEATURESET 0x0020    /**< The required set for this library */
#define SGP30_CRC8_POLYNOMIAL 0x31 /**< Seed for SGP30's CRC polynomial */
#define SGP30_CRC8_INIT 0xFF       /**< Init value for CRC */
#define SGP30_WORD_LEN 2           /**< 2 bytes per word */

//!!!
#define NULL_REG 0xFF //????? 
//!!!

static const char * MI_TOPIC_CO2 = "/Informatica/1/9/CO2/3";
static const char * MI_TOPIC_TVOC = "/Informatica/1/9/TVOC/3";

#define WINDOW_SIZE_CO2 CONFIG_WINDOW_SIZE_CO2 //Window size no se podrá modificar en ejecución porque es el tamaño de un array
int SAMPLE_FREQ_CO2 = CONFIG_SAMPLE_FREQ_CO2;
int SEND_FREQ_CO2 = CONFIG_SEND_FREQ_CO2;
int N_SAMPLES_CO2 = CONFIG_N_SAMPLES_CO2;

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1                            /*!< I2C nack value */

#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL_CO2               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA_CO2               /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUMBER(CONFIG_I2C_MASTER_PORT_NUM_CO2) /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ CONFIG_I2C_MASTER_FREQUENCY_CO2        /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */

i2c_port_t i2c_num = I2C_MASTER_NUM;


static const char *TAG = "sgp30";

static uint8_t INIT_AIR_QUALITY[2] =        { 0x20, 0x03 };
static uint8_t MEASURE_AIR_QUALITY[2] =     { 0x20, 0x08 };
static uint8_t GET_BASELINE[2] =            { 0x20, 0x15 };
static uint8_t SET_BASELINE[2] =            { 0x20, 0x1E };
static uint8_t SET_HUMIDITY[2] =            { 0x20, 0x61 };
//static uint8_t MEASURE_TEST[2] =            { 0x20, 0x32 };
static uint8_t GET_FEATURE_SET_VERSION[2] = { 0x20, 0x2F };
static uint8_t MEASURE_RAW_SIGNALS[2] =     { 0x20, 0x50 };
static uint8_t GET_SERIAL_ID[2] =           { 0x36, 0x82 };
static uint8_t SOFT_RESET[2] =              { 0x00, 0x06 };


static uint8_t SGP_DEVICE_ADDR = SGP30_ADDR;  /**< SGP30 device address variable */



/*** I2C Driver Function Pointers ***/
typedef int8_t (*sgp30_read_fptr_t)(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);

typedef int8_t (*sgp30_write_fptr_t)(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);

/** SGP30 Main Data Struct */
typedef struct sgp30_dev {
    /**< The 48-bit serial number, this value is set when you call sgp30_init */
    uint16_t serial_number[3];

    /**< The last measurement of the IAQ-calculated Total Volatile Organic
            Compounds in ppb. This value is set when you call IAQmeasure() **/
    uint16_t TVOC;
     
    /**< The last measurement of the IAQ-calculated equivalent CO2 in ppm. This
            value is set when you call IAQmeasure() */
    uint16_t eCO2;

    /**< The last measurement of the IAQ-calculated equivalent CO2 in ppm. This
            value is set when you call IAQmeasureRaw() */
    uint16_t raw_H2;

    /**< The last measurement of the IAQ-calculated equivalent CO2 in ppm. This 
            value is set when you call IAQmeasureRaw */
    uint16_t raw_ethanol;

    /**< Interface pointer, used to store I2C address of the device */
    void *intf_ptr;

    /**< I2C read driver function pointer */
    sgp30_read_fptr_t i2c_read;

    /**< I2C write driver function pointer */
    sgp30_write_fptr_t i2c_write;
} sgp30_dev_t;


struct bufferMuestrasEnvio {
    struct sgp30_reading muestras[WINDOW_SIZE_CO2];
    int ini;
    int cont;
};

struct bufferMuestrasEnvio listaMuestrasParaEnviar = {
        .ini = 0,
        .cont = 0
};

esp_timer_handle_t periodic_timer_envio_al_brocker;
esp_timer_handle_t periodic_timer_sensor_co2;

/**
 *  @brief  Setups the hardware and detects a valid SGP30. Initializes I2C
 *          then reads the serialnumber and checks that we are talking to an SGP30.
 */
void sgp30_init(sgp30_dev_t *sensor, sgp30_read_fptr_t user_i2c_read, sgp30_write_fptr_t user_i2c_write);

/**
 *   @brief  Commands the sensor to perform a soft reset using the "General
 *           Call" mode. Take note that this is not sensor specific and all devices that
 *           support the General Call mode on the on the same I2C bus will perform this.
 */
void sgp30_softreset(sgp30_dev_t *sensor);

/**
 *   @brief  Commands the sensor to begin the IAQ algorithm. Must be called
 *           after startup.
 */
void sgp30_IAQ_init(sgp30_dev_t *sensor);

/**
 *  @brief  Commands the sensor to take a single eCO2/VOC measurement. Places
 *          results in sensor.TVOC and sensor.eCO2
 */

void sgp30_IAQ_measure(sgp30_dev_t *sensor);

/**
 *  @brief  Commands the sensor to take a single H2/ethanol raw measurement.
 *          Places results in sensor.raw_H2 and sensor.raw_ethanol
 */
void sgp30_IAQ_measure_raw(sgp30_dev_t *sensor);

/*!
 *   @brief  Request baseline calibration values for both CO2 and TVOC IAQ
 *           calculations. Places results in parameter memory locaitons.
 *   @param  eco2_base
 *           A pointer to a uint16_t which we will save the calibration
 *           value to
 *   @param  tvoc_base
 *           A pointer to a uint16_t which we will save the calibration value to
 */
void sgp30_get_IAQ_baseline(sgp30_dev_t *sensor, uint16_t *eco2_base, uint16_t *tvoc_base);

/**
 *  @brief  Assign baseline calibration values for both CO2 and TVOC IAQ
 *          calculations.
 *  @param  eco2_base
 *          A uint16_t which we will save the calibration value from
 *  @param  tvoc_base
 *          A uint16_t which we will save the calibration value from
 */
void sgp30_set_IAQ_baseline(sgp30_dev_t *sensor, uint16_t eco2_base, uint16_t tvoc_base);

/**
 *  @brief  Set the absolute humidity value [mg/m^3] for compensation to
 *          increase precision of TVOC and eCO2.
 *  @param  absolute_humidity
 *          A uint32_t [mg/m^3] which we will be used for compensation.
 *          If the absolute humidity is set to zero, humidity compensation
 *          will be disabled.
 */
void sgp30_set_humidity(sgp30_dev_t *sensor, uint32_t absolute_humidity);