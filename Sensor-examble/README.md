# Ras-Sensirion: C code for Sensirion Sensor Read out on Pi Terminal

<img src="../imgs/raspi.png">

## Setup

   1. Open a [terminal](https://www.raspberrypi.org/documentation/usage/terminal/)
   3. Run the `Build.sh` file to compile the driver.

## Test your connected sensor

   - Run `./test-sensors` in the same directory you used to compile the Program

     Output:

	--- 2025-12-22 15:22:12 ---
	SFA3X -> HCHO: 10.20 ppm, Humidity: 56.22 %, Temp: 23.00 °C (73.40 °F)
	SCD30 -> CO2: 548.39 ppm, Temp: 24.53 °C (76.15 °F), Humidity: 53.61 %
	SEN55 -> PM1.0: 0.20, PM2.5: 0.30, PM4.0: 0.30, PM10: 0.30 µg/m³, Hum: 60.79, Temp: 22.60 °C (72.69 °F), VOC: 86.00, NOx: 1.00

	--- 2025-12-22 15:22:14 ---
	SFA3X -> HCHO: 10.00 ppm, Humidity: 56.19 %, Temp: 23.00 °C (73.40 °F)
	SCD30 -> CO2: 547.82 ppm, Temp: 24.54 °C (76.17 °F), Humidity: 53.58 %
	SEN55 -> PM1.0: 0.20, PM2.5: 0.20, PM4.0: 0.20, PM10: 0.20 µg/m³, Hum: 60.80, Temp: 22.60 °C (72.68 °F), VOC: 86.00, NOx: 1.00

	--- 2025-12-22 15:22:16 ---
	SFA3X -> HCHO: 10.20 ppm, Humidity: 56.15 %, Temp: 22.99 °C (73.38 °F)
	SCD30 -> CO2: 548.23 ppm, Temp: 24.51 °C (76.12 °F), Humidity: 53.59 %
	SEN55 -> PM1.0: 0.20, PM2.5: 0.20, PM4.0: 0.20, PM10: 0.20 µg/m³, Hum: 60.84, Temp: 22.58 °C (72.65 °F), VOC: 87.00, NOx: 1.00

	--- 2025-12-22 15:22:18 ---
	SFA3X -> HCHO: 10.40 ppm, Humidity: 56.13 %, Temp: 23.00 °C (73.40 °F)
	SCD30 -> CO2: 547.34 ppm, Temp: 24.54 °C (76.17 °F), Humidity: 53.59 %
	SEN55 -> PM1.0: 0.20, PM2.5: 0.20, PM4.0: 0.20, PM10: 0.20 µg/m³, Hum: 60.86, Temp: 22.58 °C (72.64 °F), VOC: 88.00, NOx: 1.00


## Resources:

- [Sensirion Site](https://sensirion.com/)
- [Sensirion Github](https://github.com/Sensirion)
- [SCD30 Docs](https://github.com/Sensirion/raspberry-pi-i2c-scd30)
- [SFA30 Docs](https://github.com/Sensirion/raspberry-pi-uart-sfa3x)
- [SEN44 Docs](https://github.com/Sensirion/raspberry-pi-uart-sen44)
- [SEN55 Docs](https://github.com/Sensirion/raspberry-pi-i2c-sen5x)
- [SEN66 Docs](https://github.com/Sensirion/raspberry-pi-i2c-sen66)
