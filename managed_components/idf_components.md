# Install

```shell
idf.py add-dependency "lvgl/lvgl^9.2.2"
idf.py add-dependency "espressif/led_strip^3.0.1"
idf.py add-dependency "joltwallet/littlefs^1.19.2"
```


## LVGL

Keep `lv_conf.h` in root dir, to setup LVGL.

OR do not use it at all, set `menuconfig` option `LV_CONF_SKIP` for that.