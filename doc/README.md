# How to

WIll add here compiled and short desctiptions for project modules and setup.

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


# Old

The old version, make as educational project is here:
- [README.md](https://github.com/trianglesis/Air_Quality_station/blob/0f882de520a3a1b63564a4ebc3921752ade938d1/doc/README.md)