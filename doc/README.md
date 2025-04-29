# How to

Will add here compiled and short desctiptions for project modules and setup.

## ESP32

Most options for the board itself.

- Kconfig [doc](https://docs.espressif.com/projects/esp-idf-kconfig/en/latest/kconfiglib/language.html)

### Little FS - SPI flash partition

Create custom partition to save config data from sensors and small html page.

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

- Show info

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

# Sensors

- `SCD40` for CO2
- `BME680` for temperature, humidity and pressure
  - Use to calibrate CO2 sensor too

# I2C

- Using new `master_i2c.h` driver from IDF 5.4.1


## Components details

[Read](components.md)

## IDF Components

[Read](../managed_components/idf_components.md)

# Old

The old version, make as educational project is here:
- [README.md](https://github.com/trianglesis/Air_Quality_station/blob/0f882de520a3a1b63564a4ebc3921752ade938d1/doc/README.md)