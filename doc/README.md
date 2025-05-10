# How to

Will add here compiled and short desctiptions for project modules and setup.

## ESP32

Most options for the board itself.

- Kconfig [doc](https://docs.espressif.com/projects/esp-idf-kconfig/en/latest/kconfiglib/language.html)

### Little FS - SPI flash partition

Create custom partition to save config data from sensors and small html page.

- Update `menuconfig` - `ESPTOOLPY_FLASHSIZE`
  - (8Mb) For ESP32C6 test board
  - (4Mb) For ESP32C6 WaveShare LCD board
- Update `menuconfig` - `PARTITION_TABLE_TYPE`

```csv
# Common partitions above
littlefs,   data,   littlefs,  ,            2000K,
```

- Update `CMakeLists.txt` root project


```txt
# Note: you must have a partition named the first argument (here it's "littlefs")
# in your partition table csv file.
littlefs_create_partition_image(littlefs ../flash_data FLASH_IN_PROJECT)
```

Path in project: `{PROJ_ROOT}/flash_data` will be used to upload files into SPI flash from build!

Initial files:
- `index.html` - dummy index page to show that LittleFS is runing and WebServer is too.
- `upload_script.html` - to upload files with the webserver to SD card


# Wifi

- Work in AP mode if no known network available.
  - Start `Web Server`
- Connect to known network and send data to Home Assistant
  - Don't use AP
  - Start `Web Server` with `File Upload` feature


# Display

[LVGL](../managed_components/idf_components.md)

[ICONS](https://www.flaticon.com/search?word=gauge)


## Simple OLED

- [ALi](https://www.aliexpress.com/item/1005006035385704.html)
- [Setup example](https://github.com/trianglesis/Monochromatic_168x64/blob/dd3de94a450a443e910fca3291d5ad1612539b91/README.md)

```txt
128*64 LED display module for Arduino, supports many control chip.
Fully compatible with for Arduino, 51 Series, MSP430 Series, STM32 / 2, CSR IC, etc.
Ultra-low power consumption: full screen lit 0.08W
Super high brightness and contrast are adjustable
With embedded driver/controller
Interface type is IIC
Pin Definition: GND, VCC ,SCL, SDA
Pins: 4 pins
Voltage: 3V ~ 5V DC
Working Temperature: -30 ° ~ 70 °
Drive Duty: 1/64 Duty
High resolution: 128 * 64
Panel Dimensions: 26.70* 19.26* 1.85mm/1.03*0.76*0.07 inch(approx)
Active Area: 21.74* 11.2mm /0.86*0.44 inch(approx)
Driver IC: SSD1306
```

Detect address: `0x3c`

```text
i2c-tools> i2cdetect
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00: 00 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
30: -- -- -- -- -- -- -- -- -- -- -- -- 3c -- -- --
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- 62 -- -- -- -- -- -- -- -- -- -- -- -- --
70: -- -- -- -- -- -- -- 77 -- -- -- -- -- -- -- --
```

- Show info

SquareLine Studio project is easily installed:

![SQLine](pics/SSD1306/SquareLineUI_Example.png)

Here is an attempt to draw an `invisible` line just to mark the border between different pixel colour matrixes.

![UI on screen](pics/SSD1306/Screen_UI.jpg)

This is a pixel border between two colour zones:

![Pixel Border](pics/SSD1306/PixelBprder.jpg)

It seems like this border is sitiated between 15th and 16th pixel.

## LVGL

Check float enabled, or UI will not show you float numbers.

`LV_USE_FLOAT`

# SD Card

- Save logs
- Save sensor states betweeen powercycles
  - Last status
  - Last calibration
- Save other statuses
  - Wifi statistics
  - Battery

# Web Server

- At local Wifi
  - Allow upload and download files to and from SD card
    - Update HTML files with this feature
- At AP mode
  - Show Sensors statistics for last 5,30,60 minutes and longer
  - Graphs and JS

# File server

CURL example:

```shell
curl -X POST --data-binary @sd_card/index.html  http://192.168.1.225:80/upload/index.html
curl -X POST --data-binary @index.html  http://192.168.1.225:80/upload/index.html
curl -X POST http://192.168.1.225:80/delete/index3.txt

# New added replace ARG
curl -X POST --data-binary @index.html  http://192.168.1.225:80/upload/index.txt?replace=1
File uploaded successfully
```

Can be used also with web: `http://192.168.1.197/download/`
- You should see dir and files

Check `FATFS_LONG_FILENAMES` to upload files with >3 chars in extension!
- [link](https://stackoverflow.com/a/72530185)

# Sensors

- `SCD40` for CO2
- `BME680` for temperature, humidity and pressure
  - Use to calibrate CO2 sensor too

# I2C

- Using new `master_i2c.h` driver from IDF 5.4.1

Test proper connections:

```log
I (231) main_task: Returned from app_main()
i2c-tools> i2cdetect
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00: 00 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- 62 -- -- -- -- -- -- -- -- -- -- -- -- -- 
70: -- -- -- -- -- -- -- 77 -- -- -- -- -- -- -- --
i2c-tools> 
```

Always check if PINs are added correctly!

i2cconfig  --port=0 --freq=100000 --sda=9 --scl=18


## Components details

[Read](components.md)

## IDF Components

[Read](../managed_components/idf_components.md)

# ADC

For measuring battery charge.

Battery 18650 cell in the case with charge\discharge\overcharge usb-c module is connected to the board via voltage bust module. 
- Charging module if giving 4.2 volts.
- Bosster is giving 5.03 volts.
- Voltage divider is dropping voltage to 0.333V

## Voltage diviner

[Voltage Divider](https://raw.org/tool/voltage-divider-calculator/)

- Using 100K and 10K resistors from example.
- UPD: Switched `47K` and `100K`, now using `100K` as drop to ground `(neutral, negative)` and `47K` as measuring protectionf for ADC `pin 0`. 

Now it shows `3247mV` which is closer to real.

## PIN ACD

ESP32 C6 have a `PIN 0` with `ADC1_CH0` use it.

Real voltage formula from [doc](https://randomnerdtutorials.com/power-esp32-esp8266-solar-panels-battery-level-monitoring/)

```text
Vout = (Vin*R2)/(R1+R2)

Vout = (4.2 * 100000) / (10000 + 100000) = 3.8V
              420000  /  110000          = 3.8 - bit extra for GPIO

Vout = (4.2 * 100000) / (20000 + 100000) = 3.5V
              420000  /  120000          = 3.5 - bit extra for GPIO

New 47k for even lower voltage
Vout = (4.2 * 100000) / (47000 + 100000) = 2.8V
              420000  /  147000          = 2.8 - this is battery 100% charged

OR reversed (used old 10K, now replaced with 47K)

Vout = (4.2 * 10000) / (10000 + 100000)   = 380mV
              42000  /  110000            = 380mV - this is battery 100% charged
```

## Get percentage

Linear interpolation:

```text
We know:
- MAX voltage       x1  - Get this value from full charged battery under the real load.
- MIN voltage       x2  - Get this value as soon as battery dies or powered off by charging board protectiion.
- MAX percent:      y1  - Desired 100%
- MIN percent:      y2  - Minimal possible charge 0.1% or 1%
- Current voltage   x   - From ADC calubrated output

Don't know
- Current percent:  y   - we need to calculate it using linear interpolation
```

By the formula:

```cpp
float max_perc = 100.0;
float min_perc = 0.1;
float max_volt = 2590.0;  // Fully charged, under esp32 load
float min_volt = 2043.0;  // Fully discharged, 
float batteryLevel = max_perc + (((battery_readings.voltage_m - max_volt) * (min_perc -  max_perc)) / (min_volt - max_volt));

// Some code ...
ESP_LOGI(TAG, "RAW: %d; Cali: V:%d; Converted V %d; Battery percentage: %d", battery_readings.adc_raw, battery_readings.voltage_m, battery_readings.voltage_m, battery_readings.percentage);
```

Result:

```log
I (1001528) adc-battery: ADC1 Channel[0] Raw Data: 2529
I (1001528) adc-battery: ADC1 Channel[0] Cali Voltage: 2542 mV
I (1001528) adc-battery: RAW: 2529; Cali: V:2542; Converted V 2542; Battery percentage: 90
```

Measurements are a bit off, sometimes dropping to from 93% to 50% and than back. 
Maybe it can become better if we use continuoius mode or measure oneshot for three times in a row getting the max value.


# Battery power

Cannot start:

`I BOD: Brownout detector was triggered`

[Doc](https://randomnerdtutorials.com/power-esp32-esp8266-solar-panels-battery-level-monitoring/)
[Reddit comment](https://www.reddit.com/r/esp32/comments/sa73xs/comment/htsybyz/)

Must change the power scheme.
This converter is probably cheap and unstable, can only work with esp32 5V if no Wifi used.

![cheap](pics/converter/shitverter.png)

Temporary solution: add wifi Off option, and skip wifi AP\STA, Webserver and captive portal setup.
The board is working fine now.

# Debug and etc

- [JTAG](https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32c6/api-guides/jtag-debugging/index.html)

With and without LP core

```txt
openocd -f board/esp32c6-builtin.cfg
openocd -f board/esp32c6-lpcore-builtin.cfg

# Separate PS
d:\.espressif\tools\openocd-esp32\v0.12.0-esp32-20241016\openocd-esp32\bin\openocd.exe -f board/esp32c6-builtin.cfg
d:\.espressif\tools\openocd-esp32\v0.12.0-esp32-20241016\openocd-esp32\bin\openocd.exe -f board/esp32c6-lpcore-builtin.cfg
```

```text
ESP_CONSOLE_UART
ESP_CONSOLE_SECONDARY
```

# Old

The old version, make as educational project is here:
- [README.md](https://github.com/trianglesis/Air_Quality_station/blob/0f882de520a3a1b63564a4ebc3921752ade938d1/doc/README.md)