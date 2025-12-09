#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#include "sensirion_i2c_hal.h"
#include "sfa3x_i2c.h"
#include "scd30_i2c.h"
#include "sen44_i2c.h"

int main(void) {
    int16_t error = 0;

    // Initialize I2C
    sensirion_i2c_hal_init();

    //SFA30
    sfa3x_init(SFA3X_I2C_ADDR_5D);
    error = sfa3x_device_reset();
    if (error) printf("SFA3X reset error: %i\n", error);
    sensirion_i2c_hal_sleep_usec(1000000);
    error = sfa3x_start_continuous_measurement();
    if (error) printf("SFA3X start measurement error: %i\n", error);

    //SCD30
    scd30_init(SCD30_I2C_ADDR_61);
    scd30_stop_periodic_measurement();
    scd30_soft_reset();
    sensirion_i2c_hal_sleep_usec(2000000);
    error = scd30_start_periodic_measurement(0);
    if (error) printf("SCD30 start measurement error: %i\n", error);

    //SEN44
    error = sen44_device_reset();
    if (error) printf("SEN44 reset error: %i\n", error);
    error = sen44_start_measurement();
    if (error) printf("SEN44 start measurement error: %i\n", error);

    printf("Starting multi-sensor measurement loop...\n");

    // Poll sensors indefinitely
    while (1) {
        sensirion_i2c_hal_sleep_usec(1000000); // 1 second delay

        uint64_t timestamp = sensirion_i2c_hal_get_time_usec();
        printf("\n--- %" PRIu64 " us ---\n", timestamp);

        // SFA30
        float hcho = 0, humidity = 0, temperature = 0;
        error = sfa3x_read_measured_values(&hcho, &humidity, &temperature);
        if (!error) {
            printf("SFA3X -> HCHO: %.2f ppm, Humidity: %.2f %%, Temp: %.2f °C\n",
                   hcho, humidity, temperature);
        } else {
            printf("SFA3X read error: %i\n", error);
        }

        // SCD30
        float co2 = 0, scd_temp = 0, scd_hum = 0;
        error = scd30_blocking_read_measurement_data(&co2, &scd_temp, &scd_hum);
        if (!error) {
            printf("SCD30 -> CO2: %.2f ppm, Temp: %.2f °C, Humidity: %.2f %%\n",
                   co2, scd_temp, scd_hum);
        } else {
            printf("SCD30 read error: %i\n", error);
        }

        // SEN44
        uint16_t pm1p0 = 0, pm2p5 = 0, pm4p0 = 0, pm10p0 = 0;
        float voc_index = 0, sen44_hum = 0, sen44_temp = 0;
        error = sen44_read_measured_mass_concentration_and_ambient_values(
            &pm1p0, &pm2p5, &pm4p0, &pm10p0, &voc_index, &sen44_hum, &sen44_temp);
        if (!error) {
            printf("SEN44 -> PM1.0: %u µg/m³, PM2.5: %u µg/m³, PM4.0: %u µg/m³, PM10: %u µg/m³\n",
                   pm1p0, pm2p5, pm4p0, pm10p0);
            printf("         VOC index: %.1f, Humidity: %.2f %%, Temp: %.2f °C\n",
                   voc_index, sen44_hum, sen44_temp);
        } else {
            printf("SEN44 read error: %i\n", error);
        }
    }

    // Stop measurements (unreachable in infinite loop)
    sfa3x_stop_measurement();
    scd30_stop_periodic_measurement();
    sen44_stop_measurement();

    return 0;
}
