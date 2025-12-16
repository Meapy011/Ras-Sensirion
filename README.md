# Ras-Sensirion: C code for Sensirion Sensor Read out on Pi Terminal

<img src="imgs/raspi.png">

## Resources:

- [Sensirion Site](https://sensirion.com/)
- [Sensirion Github](https://github.com/Sensirion)
- [Sen44 Docs](https://github.com/Sensirion/raspberry-pi-uart-sen44)
- [SCD30 Docs](https://github.com/Sensirion/raspberry-pi-i2c-scd30)
- [SFA30 Docs](https://github.com/Sensirion/raspberry-pi-uart-sfa3x)

## Setup your Pi: 

### Checking for Updates and making sure Make is installed

	sudo apt update -y
	sudo apt install build-essential -y

## Compiling Makefile

   1. Open a [terminal](https://www.raspberrypi.org/documentation/usage/terminal/)
   2. Navigate to the directory. E.g. `cd ~/Ras-Sensirion
   3. Run the `make command to compile the driver

       Output:
       ```
       gcc -Wall -Iinclude -O2 -c main.c -o main.o
       gcc -Wall -Iinclude -O2 -o multi-sensirion main.o src/sensirion_i2c_hal.o src/sensirion_i2c.o src/sensirion_common.o src/sen44_i2c.o 
       src/scd30_i2c.o src/sfa3x_i2c.o

       ```

## Test your connected sensor
    - Run `./multi-sensirion` in the same directory you used to compile the Program

