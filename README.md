# AQUACONTROL 2

You need Visual Studio Code and PlatformIO installed to compile this app.

## How to setup this app

### 1. Make a new file `src/secrets.h`

### 2. Open `src/secrets.h` and set your credentials
Copy and paste the source below to `src/secrets.h`:

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

- only the upload button is username/password protected. 
- set username and password to an empty string to disable password protection

## 2. Make sure you have a FAT32 formatted SD card in the card slot

Timers are saved on the SD card to a file called `/default.aqu` .<br>
Without SD card the app will not work.

## 3. Upload app and start editing

After uploading, the IP address of the webinterface is shown on the display.<br>Browse to this IP, then click on a channel bar and start editing timers.