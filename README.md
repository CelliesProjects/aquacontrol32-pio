# AQUACONTROL32-PIO

## What is this?

This is a led control app aimed at aquarium use. With this app you can program gradual sunrises and sunsets on 5 led channels through a web interface. The moon phase cycle is simulated and can be adjusted from the web interface. All settings are saved to an sd card.

Besides a led dimming board, you need a supported device and Visual Studio Code plus PlatformIO installed to compile and upload this app.

Supported devices for now are M5Stack Grey/Fire and the ESP32-S3-BOX-Lite.<br>The display driver is LovyanGFX so porting to a new device should be pretty easy and some more devices will be supported.

## Editor screenshot

![editor](https://github.com/user-attachments/assets/65d36678-d3d9-4f20-9c3a-0ab2893e1b3b)

## How to setup this app

### Hardware needed

- A supported esp32 board. (an sd card slot is required)
- A led dimming board capable of handling 5 led pwm inputs. (and optionally a i2c temperature sensor)
- A ds18b20 temperature sensor. (optional)

### Upgrading from aquacontrol32

This app is the successor to aquacontrol32 and is compatible with the file format used so you can easily upgrade and keep your current light settings. Just upload your existing `default.aqu` to the new device and done!

## 1. Software setup

- Visual Studio Code
- PlatformIO
- [The latest release of aquacontrol](releases/latest) - unzip the file to a local folder.

## 2. Open this folder in platformIO

Open `platformio.ini` and adjust the `[user]`section to your location.

In the same file, adjust the gpio pins in the `[env:boardxxx]` section by adjusting the following `build_flags`:

```bash
    -D SDCARD_SS=xx
    -D LEDPIN_0=xx
    -D LEDPIN_1=xx
    -D LEDPIN_2=xx
    -D LEDPIN_3=xx
    -D LEDPIN_4=xx
    -D ONE_WIRE_PIN=xx
```

## 3. Create a new file `src/secrets.h`

Still in the folder where you extracted the files, create a new file `src/secrets.h`.

Copy-paste the source below to `src/secrets.h`:

```c++
#ifndef _SECRETS_H_
#define _SECRETS_H_

const char *SSID = "wifi network";
const char *PSK = "wifi password";

const char *WEBIF_USER = "admin";
const char *WEBIF_PASSWORD = "admin";

#endif
```

Adjust the values as needed.

## 4. Make sure you have a FAT32 formatted SD card inserted

Timers are saved on the SD card as `default.aqu`.

Moonlight setup is saved on the SD card as `default.mnl`.

Without SD card the app will seem to work but saving timers or moonlight settings is not possible.

## 5. Upload app and start editing

Select the PIO icon on the left, then open `Project Tasks`. Click on your device to expand the menu and there select `Upload and Monitor`.

After uploading, the IP address of the webinterface is shown on the display.<br>Browse to this IP, then click on a channel bar and start editing timers.

The web interface is not yet completely ready but should be fully functional.

The important urls are:

- `/`<br>Current channel levels and temperature are shown.<br>Click on a level bar to go to the:
- `/editor`<br>Edit channel timers and save these timers to an SD card.
- `/moonsetup`<br>Setup full moon levels for the moon simulator.
- `/fileupload`<br>Upload files to the controller.<br>
  Uploaded files named `default.aqu` or `default.mnl` will be parsed and if valid light/moon settings are found, these will be applied.
