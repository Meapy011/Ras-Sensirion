#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <curl/curl.h>

#include "sensirion_i2c_hal.h"
#include "sfa3x_i2c.h"
#include "scd30_i2c.h"
#include "sen44_i2c.h"
#include "sen66_i2c.h"

/* --- InfluxDB configuration --- */
#define INFLUXDB_URL    "http://localhost:8086/api/v2/write?org=biome&bucket=sensors&precision=s"
#define INFLUXDB_TOKEN  "qRct_zczypJxm_UKQ0u_sp3z67XooIFpUHdWz1tP2qErOc9eTRaMi5uTBcrFftoSHDLABdotjaMVfVsrU1R_8g=="

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

static void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

/* --- InfluxDB HTTP write --- */
static void influxdb_write(const char* line) {
    CURL* curl = curl_easy_init();
    if (!curl) return;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: text/plain; charset=utf-8");
    headers = curl_slist_append(headers, "Authorization: Token " INFLUXDB_TOKEN);

    curl_easy_setopt(curl, CURLOPT_URL, INFLUXDB_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, line);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 2000L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "InfluxDB write failed: %s\n", curl_easy_strerror(res));
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    /* --- I2C init --- */
    sensirion_i2c_hal_init();

    /* --- SFA3X --- */
    sfa3x_init(SFA3X_I2C_ADDR_5D);
    sfa3x_device_reset();
    sensirion_i2c_hal_sleep_usec(1000000);
    sfa3x_start_continuous_measurement();

    /* --- SCD30 --- */
    scd30_init(SCD30_I2C_ADDR_61);
    scd30_stop_periodic_measurement();
    scd30_soft_reset();
    sensirion_i2c_hal_sleep_usec(2000000);
    scd30_start_periodic_measurement(0);

    /* --- SEN44 --- */
    sen44_device_reset();
    sen44_start_measurement();

    /* --- SEN66 --- */
    sen66_init(SEN66_I2C_ADDR_6B);
    sen66_device_reset();
    sensirion_i2c_hal_sleep_usec(1200000);
    sen66_start_continuous_measurement();

    printf("Starting multi-sensor measurement loop...\n");

    while (running) {
        sensirion_i2c_hal_sleep_usec(1000000);

        char timestamp[32];
        get_local_timestamp(timestamp, sizeof(timestamp));
        printf("\n--- %s ---\n", timestamp);

        /* --- SFA3X --- */
        float hcho = 0.0f, sfa_hum = 0.0f, sfa_temp = 0.0f;
        if (!sfa3x_read_measured_values(&hcho, &sfa_hum, &sfa_temp)) {
            printf("SFA3X -> HCHO: %.2f ppb, Humidity: %.2f %%, Temp: %.2f °C\n",
                   hcho, sfa_hum, sfa_temp);

            char line[256];
            snprintf(line, sizeof(line),
                     "sfa3x,device=SFA3X hcho=%.2f,humidity=%.2f,temp=%.2f",
                     hcho, sfa_hum, sfa_temp);
            influxdb_write(line);
        }

        /* --- SCD30 --- */
        float co2 = 0.0f, scd_temp = 0.0f, scd_hum = 0.0f;
        if (!scd30_blocking_read_measurement_data(&co2, &scd_temp, &scd_hum)) {
            printf("SCD30 -> CO2: %.2f ppm, Temp: %.2f °C, Humidity: %.2f %%\n",
                   co2, scd_temp, scd_hum);

            char line[256];
            snprintf(line, sizeof(line),
                     "scd30,device=SCD30 co2=%.2f,temp=%.2f,humidity=%.2f",
                     co2, scd_temp, scd_hum);
            influxdb_write(line);
        }

        /* --- SEN44 --- */
        uint16_t pm1p0_44=0, pm2p5_44=0, pm4p0_44=0, pm10p0_44=0;
        float voc_44=0.0f, hum_44=0.0f, temp_44=0.0f;

        if (!sen44_read_measured_mass_concentration_and_ambient_values(
                &pm1p0_44, &pm2p5_44, &pm4p0_44, &pm10p0_44,
                &voc_44, &hum_44, &temp_44)) {

            printf("SEN44 -> PM1.0: %u, PM2.5: %u, PM4.0: %u, PM10: %u µg/m³\n",
                   pm1p0_44, pm2p5_44, pm4p0_44, pm10p0_44);
            printf("         VOC: %.1f, Humidity: %.2f %%, Temp: %.2f °C\n",
                   voc_44, hum_44, temp_44);

            char line[512];
            snprintf(line, sizeof(line),
                     "sen44,device=SEN44 pm1p0=%u,pm2p5=%u,pm4p0=%u,pm10p0=%u,voc=%.2f,hum=%.2f,temp=%.2f",
                     pm1p0_44, pm2p5_44, pm4p0_44, pm10p0_44,
                     voc_44, hum_44, temp_44);
            influxdb_write(line);
        }

        /* --- SEN66 --- */
        float pm1p0_66=0.0f, pm2p5_66=0.0f, pm4p0_66=0.0f, pm10p0_66=0.0f;
        float hum_66=0.0f, temp_66=0.0f, voc_66=0.0f, nox_66=0.0f;
        uint16_t co2_66=0;

        if (!sen66_read_measured_values(
                &pm1p0_66, &pm2p5_66, &pm4p0_66, &pm10p0_66,
                &hum_66, &temp_66, &voc_66, &nox_66, &co2_66)) {

            printf("SEN66 -> PM1.0: %.2f, PM2.5: %.2f, PM4.0: %.2f, PM10: %.2f µg/m³\n",
                   pm1p0_66, pm2p5_66, pm4p0_66, pm10p0_66);
            printf("         Humidity: %.2f %%, Temp: %.2f °C, VOC: %.2f, NOx: %.2f, CO2: %u ppm\n",
                   hum_66, temp_66, voc_66, nox_66, co2_66);

            char line[512];
            snprintf(line, sizeof(line),
                     "sen66,device=SEN66 pm1p0=%.2f,pm2p5=%.2f,pm4p0=%.2f,pm10p0=%.2f,"
                     "hum=%.2f,temp=%.2f,voc=%.2f,nox=%.2f,co2=%u",
                     pm1p0_66, pm2p5_66, pm4p0_66, pm10p0_66,
                     hum_66, temp_66, voc_66, nox_66, co2_66);
            influxdb_write(line);
        }
    }

    printf("Stopping measurements...\n");
    sfa3x_stop_measurement();
    scd30_stop_periodic_measurement();
    sen44_stop_measurement();
    sen66_stop_measurement();

    return 0;
}
