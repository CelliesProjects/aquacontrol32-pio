# AQUACONTROL32-PIO

## What is this?

This is a led control app aimed at aquarium use. It is the successor to aquacontrol32 and is compatible with the file format used there so you can easily upgrade and keep your current light settings. Just upload your existing `default.aqu` to the new device and done!

You need a supported device and Visual Studio Code plus PlatformIO installed to compile and upload this app.

Supported devices for now are M5stack Grey/Fire and the ESP32-S3-BOX-Lite. The display driver is LovyanGFX so porting to a new device should be pretty easy and some more devices will be supported.

## How to setup this app

### 1. Software download and setup

- Download and install Visual Studio Code
- Download and install PlatformIO
- [Download the latest release of aquacontrol](https://github.com/CelliesProjects/aquacontrol32-pio/releases/latest) and unzip the file to a local folder.

### 2. Open this folder and create a new file `src/secrets.h`

Go to the folder where you extracted the files and create a new file `src/secrets.h`.

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

### 3. Make sure you have a FAT32 formatted SD card inserted

Timers are saved on the SD card as `default.aqu`.

Moonlight setup is saved on the SD card as `default.mnl`.

Without SD card the app will seem to work but saving timers or moonlight settings is not possible.

## 4. Upload app and start editing

Select the PIO icon on the left, then open `Project Tasks`. Click on your device to expand the menu and there select `Upload and Monitor`.

After uploading, the IP address of the webinterface is shown on the display.<br>Browse to this IP, then click on a channel bar and start editing timers.

The web interface is not yet completely ready but should be fully functional.

The important urls are:

- `/`<br>Current channel levels and temperature are shown.<br>Click on a level bar to go to the:


- `/editor`<br>Edit channel timers and save these timers to an SD card.


- `/moonsetup`<br>Setup full moon levels for the moon simulator.


- `/fileupload`<br>Upload files to the controller.<br>
  Uploaded files named `default.aqu` or `default.mnl` will be parsed and if valid light/moon settings are found, these will be applied.
