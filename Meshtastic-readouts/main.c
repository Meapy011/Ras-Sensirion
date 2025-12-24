#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include "sensirion_i2c_hal.h"
#include "sfa3x_i2c.h"
#include "scd30_i2c.h"
#include "sen44_i2c.h"
#include "sen66_i2c.h"
#include "sen5x_i2c.h"

/* Spike thresholds */
#define HCHO_SPIKE     5.0f   // ppm
#define CO2_SPIKE      100.0f // ppm
#define PM_SPIKE       50.0f  // µg/m³
#define TEMP_SPIKE     5.0f   // °C
#define HUM_SPIKE      10.0f  // %
#define VOC_SPIKE      10.0f  // arbitrary units
#define NOX_SPIKE      10.0f  // arbitrary units

static volatile sig_atomic_t running = 1;

/* Timestamp "YYYY-MM-DD HH:MM:SS" */
static void get_local_timestamp(char* buf, size_t len) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm_info;
    if (localtime_r(&tv.tv_sec, &tm_info) == NULL) {
        snprintf(buf, len, "1970-01-01 00:00:00");
        return;
    }
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", &tm_info);
}

/* Signal handler */
static void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

/* Meshtastic send helper (escapes quotes) */
static void send_meshtastic(const char* msg) {
    char cmd[512];
    char escaped[512];
    size_t j = 0;
    for (size_t i = 0; msg[i] != '\0' && j < sizeof(escaped) - 1; i++) {
        if (msg[i] == '"') {
            if (j + 2 < sizeof(escaped) - 1) {
                escaped[j++] = '\\';
                escaped[j++] = '"';
            }
        } else {
            escaped[j++] = msg[i];
        }
    }
    escaped[j] = '\0';

    snprintf(cmd, sizeof(cmd),
             "meshtastic --port /dev/ttyACM0 --sendtext \"%s\"",
             escaped);
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Meshtastic send failed: %d\n", ret);
    }
}

/* Spike detection helper */
static bool spike_detected(float current, float previous, float threshold) {
    if (isnan(current) || isnan(previous)) return false;
    return fabsf(current - previous) > threshold;
}

/* Convert Celsius to Fahrenheit */
static float c_to_f(float c) {
    return c * 9.0f / 5.0f + 32.0f;
}

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    /* --- I2C init --- */
    sensirion_i2c_hal_init();

    /* --- Sensor startup --- */
    sfa3x_init(SFA3X_I2C_ADDR_5D);
    sfa3x_device_reset();
    sensirion_i2c_hal_sleep_usec(1000000);
    sfa3x_start_continuous_measurement();

    scd30_init(SCD30_I2C_ADDR_61);
    scd30_stop_periodic_measurement();
    scd30_soft_reset();
    sensirion_i2c_hal_sleep_usec(2000000);
    scd30_start_periodic_measurement(0);

    sen44_device_reset();
    sen44_start_measurement();

    sen66_init(SEN66_I2C_ADDR_6B);
    sen66_device_reset();
    sensirion_i2c_hal_sleep_usec(1200000);
    sen66_start_continuous_measurement();

    sen5x_device_reset();
    sensirion_i2c_hal_sleep_usec(1000000);
    sen5x_set_temperature_offset_simple(0.0f);
    sen5x_start_measurement();

    /* --- Previous values for spike detection --- */
    float prev_hcho=0, prev_sfa_temp=0, prev_sfa_hum=0;
    float prev_co2=0, prev_scd_temp=0, prev_scd_hum=0;
    float prev_pm2p5_44=0, prev_voc_44=0;
    float prev_pm2p5_66=0, prev_voc_66=0, prev_nox_66=0;
    float prev_pm2p5_5x=0, prev_voc_5x=0, prev_nox_5x=0;

    printf("Starting multi-sensor measurement loop...\n");

    while (running) {
        char timestamp[32];
        get_local_timestamp(timestamp, sizeof(timestamp));
        printf("\n--- %s ---\n", timestamp);

        /* --- Read sensor data --- */
        float hcho=0, sfa_temp=0, sfa_hum=0;
        float co2=0, scd_temp=0, scd_hum=0;
        uint16_t pm1p0_44=0, pm2p5_44=0, pm4p0_44=0, pm10p0_44=0;
        float voc_44=0, hum_44=0, temp_44=0;
        float pm1p0_66=0, pm2p5_66=0, pm4p0_66=0, pm10p0_66=0;
        float hum_66=0, temp_66=0, voc_66=0, nox_66=0;
        uint16_t co2_66=0;
        float pm1p0_5x=0, pm2p5_5x=0, pm4p0_5x=0, pm10p0_5x=0;
        float hum_5x=0, temp_5x=0, voc_5x=0, nox_5x=0;

        sfa3x_read_measured_values(&hcho, &sfa_hum, &sfa_temp);
        scd30_blocking_read_measurement_data(&co2, &scd_temp, &scd_hum);
        sen44_read_measured_mass_concentration_and_ambient_values(
            &pm1p0_44,&pm2p5_44,&pm4p0_44,&pm10p0_44,&voc_44,&hum_44,&temp_44);
        sen66_read_measured_values(
            &pm1p0_66,&pm2p5_66,&pm4p0_66,&pm10p0_66,
            &hum_66,&temp_66,&voc_66,&nox_66,&co2_66);
        sen5x_read_measured_values(
            &pm1p0_5x,&pm2p5_5x,&pm4p0_5x,&pm10p0_5x,
            &hum_5x,&temp_5x,&voc_5x,&nox_5x);

        char msg[256];

        /* --- SFA3X --- */
        bool spike = spike_detected(hcho, prev_hcho, HCHO_SPIKE) ||
                     spike_detected(sfa_temp, prev_sfa_temp, TEMP_SPIKE) ||
                     spike_detected(sfa_hum, prev_sfa_hum, HUM_SPIKE);
        prev_hcho = hcho; prev_sfa_temp = sfa_temp; prev_sfa_hum = sfa_hum;

        if ((spike || hcho != 0.0f || sfa_temp != 0.0f || sfa_hum != 0.0f) &&
            !isnan(hcho) && !isnan(sfa_temp) && !isnan(sfa_hum)) {
            snprintf(msg, sizeof(msg),
                     "SFA3X -> HCHO: %.2f ppm, Temp: %.2f C (%.2f F), Hum: %.2f%%",
                     hcho, sfa_temp, c_to_f(sfa_temp), sfa_hum);
            send_meshtastic(msg);
            printf("%s\n", msg);
        }

        /* --- SCD30 --- */
        spike = spike_detected(co2, prev_co2, CO2_SPIKE) ||
                spike_detected(scd_temp, prev_scd_temp, TEMP_SPIKE) ||
                spike_detected(scd_hum, prev_scd_hum, HUM_SPIKE);
        prev_co2 = co2; prev_scd_temp = scd_temp; prev_scd_hum = scd_hum;

        if ((spike || co2 != 0.0f || scd_temp != 0.0f || scd_hum != 0.0f) &&
            !isnan(co2) && !isnan(scd_temp) && !isnan(scd_hum)) {
            snprintf(msg, sizeof(msg),
                     "SCD30 -> CO2: %.2f ppm, Temp: %.2f C (%.2f F), Hum: %.2f%%",
                     co2, scd_temp, c_to_f(scd_temp), scd_hum);
            send_meshtastic(msg);
            printf("%s\n", msg);
        }

        /* --- SEN44 --- */
        spike = spike_detected(pm2p5_44, prev_pm2p5_44, PM_SPIKE) ||
                spike_detected(voc_44, prev_voc_44, VOC_SPIKE);
        prev_pm2p5_44 = pm2p5_44; prev_voc_44 = voc_44;

        if ((spike || pm2p5_44 != 0 || voc_44 != 0.0f || temp_44 != 0.0f || hum_44 != 0.0f)) {
            snprintf(msg, sizeof(msg),
                     "SEN44 -> PM2.5: %u, VOC: %.2f, Temp: %.2f C (%.2f F), Hum: %.2f",
                     pm2p5_44, voc_44, temp_44, c_to_f(temp_44), hum_44);
            send_meshtastic(msg);
            printf("%s\n", msg);
        }

        /* --- SEN66 --- */
        spike = spike_detected(pm2p5_66, prev_pm2p5_66, PM_SPIKE) ||
                spike_detected(voc_66, prev_voc_66, VOC_SPIKE) ||
                spike_detected(nox_66, prev_nox_66, NOX_SPIKE);
        prev_pm2p5_66 = pm2p5_66; prev_voc_66 = voc_66; prev_nox_66 = nox_66;

        if ((spike || pm2p5_66 != 0.0f || voc_66 != 0.0f || temp_66 != 0.0f ||
             hum_66 != 0.0f || nox_66 != 0.0f)) {
            snprintf(msg, sizeof(msg),
                     "SEN66 -> PM2.5: %.2f, VOC: %.2f, Temp: %.2f C (%.2f F), Hum: %.2f, NOx: %.2f",
                     pm2p5_66, voc_66, temp_66, c_to_f(temp_66), hum_66, nox_66);
            send_meshtastic(msg);
            printf("%s\n", msg);
        }

        /* --- SEN55 --- */
        spike = spike_detected(pm2p5_5x, prev_pm2p5_5x, PM_SPIKE) ||
                spike_detected(voc_5x, prev_voc_5x, VOC_SPIKE) ||
                spike_detected(nox_5x, prev_nox_5x, NOX_SPIKE);
        prev_pm2p5_5x = pm2p5_5x; prev_voc_5x = voc_5x; prev_nox_5x = nox_5x;

        if ((spike || pm2p5_5x != 0.0f || voc_5x != 0.0f || temp_5x != 0.0f ||
             hum_5x != 0.0f || nox_5x != 0.0f)) {
            snprintf(msg, sizeof(msg),
                     "SEN55 -> PM2.5: %.2f, VOC: %.2f, Temp: %.2f C (%.2f F), Hum: %.2f, NOx: %.2f",
                     pm2p5_5x, voc_5x, temp_5x, c_to_f(temp_5x), hum_5x, nox_5x);
            send_meshtastic(msg);
            printf("%s\n", msg);
        }

        /* --- Sleep 2 minutes --- */
        if (running) {
            printf("Sleeping 2 minutes before next reading...\n");
            for (int i = 0; i < 120 && running; i++) sleep(1);
        }
    }

    printf("Stopping measurements...\n");
    sfa3x_stop_measurement();
    scd30_stop_periodic_measurement();
    sen44_stop_measurement();
    sen66_stop_measurement();
    sen5x_stop_measurement();

    return 0;
}
