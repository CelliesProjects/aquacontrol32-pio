# AQUACONTROL32-PIO

This is a LED control app aimed at aquarium use. With this app you can program gradual sunrises and sunsets on 5 LED channels through a web interface. The moon phase cycle is simulated and can be adjusted from the web interface. All settings are saved to an sd card.

To use this project you need a esp32 module, a LED dimming board and you will need [VSCode](https://code.visualstudio.com/) with the [PlatformIO](https://platformio.org/) plugin installed.

Supported devices are all esp32 devices with an sd card slot plus M5Stack Grey/Fire and the ESP32-S3-BOX-Lite.

![M5Stack grey LED dimming board with 2A LED channels and sensor connector](https://github.com/user-attachments/assets/30b79d2d-9873-4528-86e6-4fe226557873)  
M5Stack grey LED dimming board with 2A LED channels and sensor connector

## Edit channels with the web interface

![editor](https://github.com/user-attachments/assets/65d36678-d3d9-4f20-9c3a-0ab2893e1b3b)

- Add timers by clicking on the grid
- To adjust timers drag and drop a timer handle to another location
- The first timer can not be deleted but the intensity can be set
- No touch handlers are defined so it will not work on a touch device

## How to setup this app

### Hardware needed

- Supported esp32 board. (SD card slot is required)
- Led dimming board capable of handling 5 LED pwm inputs. (and optionally a i2c temperature sensor)
- ds18b20 temperature sensor. (optional)

### Upgrading from aquacontrol32

This app is the successor to aquacontrol32 and is compatible with the file format used so you can easily upgrade and keep your current light settings. Just upload your existing `default.aqu` to the new device and done!

## Software setup

### 1 - Download software

- Visual Studio Code
- PlatformIO
- [The latest release of aquacontrol](https://github.com/CelliesProjects/aquacontrol32-pio/releases/latest) - unzip the file to a local folder.

Open this folder in PlatformIO `File->Open Folder`.

### 2 - Adjust user location and board setup

Open `platformio.ini` and adjust the `[user]`section to your location.

In the same file, you can adjust the gpio pins in the `[env:headless]` section by adjusting the following `build_flags`:

```bash
    -D SDCARD_SS=xx
    -D LEDPIN_0=xx
    -D LEDPIN_1=xx
    -D LEDPIN_2=xx
    -D LEDPIN_3=xx
    -D LEDPIN_4=xx
```

Note: The ds18b20 temperature sensoris not supported in the headless build.

### 3 - Setup your WiFi secrets in `src/secrets.h`

In the folder where you extracted the files, create a new file `src/secrets.h`.

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

### 4. Make sure you have a FAT32 formatted SD card inserted

- Timers are saved on the SD card as `default.aqu`.
- Moonlight setup is saved on the SD card as `default.mnl`.

Without SD card the app will seem to work but saving timers or moonlight settings is not possible.  
Uploaded timers will be gone on a reboot without a SD card.

### 5. Upload app and start editing

Select the PIO icon on the left, then open `Project Tasks`.  
Click on your device to expand the menu and there select `Upload and Monitor`.

After uploading, the IP address of the webinterface is shown on the display.  
Browse to this IP, then click on a channel bar and start editing timers.

## URLs on the device

- **`/`**  
  Current channel levels and temperature are shown.  
  Click on a level bar to go to the **`/editor`**.

- **`/editor`**  
  Edit channel timers and save these timers to an SD card.

- **`/moonsetup`**  
  Setup full moon levels for the moon simulator.

- **`/fileupload`**  
  Upload files to the controller.  
  Uploaded files named `default.aqu` or `default.mnl` will be parsed and if valid light or moon settings are found, these will be applied directly after upload.

- **`/api/uptime`**  
  Uptime in human readable format
