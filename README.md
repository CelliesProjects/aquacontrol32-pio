# AQUACONTROL32-PIO

This is a led control app aimed at aquarium use. With this app you can program gradual sunrises and sunsets on 5 led channels through a web interface. The moon phase cycle is simulated and can be adjusted from the web interface. All settings are saved to an sd card.

To use this project you need a led dimming board, a supported device and Visual Studio Code plus PlatformIO installed to compile and upload this app.  

![image-aquaboard2](https://github.com/user-attachments/assets/30b79d2d-9873-4528-86e6-4fe226557873)

Supported devices are M5Stack Grey/Fire and the ESP32-S3-BOX-Lite.

## Editor screenshot

Edit the channels through the built-in web interface.  
![editor](https://github.com/user-attachments/assets/65d36678-d3d9-4f20-9c3a-0ab2893e1b3b)  

## How to setup this app

### Hardware needed

- Supported esp32 board. (LovyanGFX support and sd card slot is required)  
- Led dimming board capable of handling 5 led pwm inputs. (and optionally a i2c temperature sensor)  
- ds18b20 temperature sensor. (optional)  

### Upgrading from aquacontrol32

This app is the successor to aquacontrol32 and is compatible with the file format used so you can easily upgrade and keep your current light settings. Just upload your existing `default.aqu` to the new device and done!

## 1. Software setup

- Visual Studio Code
- PlatformIO
- [The latest release of aquacontrol](https://github.com/CelliesProjects/aquacontrol32-pio/releases/latest) - unzip the file to a local folder.

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

- Timers are saved on the SD card as `default.aqu`.
- Moonlight setup is saved on the SD card as `default.mnl`.

Without SD card the app will seem to work but saving timers or moonlight settings is not possible.

## 5. Upload app and start editing

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
