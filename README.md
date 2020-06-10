# esp-comms: esp communication support lib
provides some useful tools for ESP32 projects. 

Can run in its own task (`init_comms(true, ...`) or in the current task (`init_comms(false, ...`). For the latter, `refresh_comms()` needs to be called regularly.

## web console
All `log_x()` calls are shown in real-time in the browser. Supports ANSI color codes. There is a ring-buffer of a few hundred lines, to see what happened before opening the web-site.

## .json settings
application settings in a .json file. On spiffs or SD card. Provides a web-interface with .json syntax verification.

This also handles the wifi-ssid and password, which are loaded from the `wifi_ssid` and `wifi_pw` keys in the .json file. If they are not defined (which they are not, by default), esp-comms will fall back on the build flags in `platformio.ini`. 

If no connection can be established, esp-comms will switch to STA (hotspot) mode, which allows to configure the wifi settings by editing the .json file over the web-interface.

STA mode can be enforced by pushing the flash button while esp-comms tries to establish the connection.

## ota updates
Runs the [standard esp32 arduino way of doing OTA](https://docs.platformio.org/en/latest/platforms/espressif32.html#over-the-air-ota-update). Must be enabled in the settings.json file with `"enable_ota": true`.
