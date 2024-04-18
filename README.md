[中文](README_CN.md)

## Introduce

Wireless debugging with ***only one ESP Chip*** !

Realized by USBIP and CMSIS-DAP protocol stack.

> 👉 5m range, 100kb size firmware(Hex) earse and download test:

<p align="center"><img src="https://user-images.githubusercontent.com/17078589/120925694-4bca0d80-c70c-11eb-91b7-ffa54770faea.gif"/></p>

----

For Keil users, we now also support [elaphureLink](https://github.com/windowsair/elaphureLink). No need for usbip to start your wireless debugging!

## Feature

1. SoC Compatibility
    - [x] ESP32
    - [x] ESP32C3
    
2. Debug Communication Mode
    - [x] SWD
    - [x] JTAG

3. USB Communication Mode
    - [x] USB-HID
    - [x] WCID & WinUSB (Default)


5. More..
    - [x] SWD protocol based on SPI acceleration (Up to 40MHz)
    - [x] Support for [elaphureLink](https://github.com/windowsair/elaphureLink), fast Keil debug without drivers
    - [x] ...



## Link your board

### WIFI


There is built-in ipv4 only mDNS server. You can access the device using `dap.local`.

![mDNS](https://user-images.githubusercontent.com/17078589/149659052-7b29533f-9660-4811-8125-f8f50490d762.png)


### Debugger

<details>
<summary>ESP32C3</summary>

| SWD            |        |
|----------------|--------|
| SWCLK          | GPIO6  |
| SWDIO          | GPIO7  |
| TVCC           | 3V3    |
| GND            | GND    |


--------------


| JTAG               |         |
|--------------------|---------|
| TCK                | GPIO6   |
| TMS                | GPIO7   |
| TDI                | GPIO9   |
| TDO                | GPIO8   |
| nTRST \(optional\) | GPIO4   |
| nRESET             | GPIO5   |
| TVCC               | 3V3     |
| GND                | GND     |


</details>



<details>
<summary>ESP32</summary>

| SWD            |        |
|----------------|--------|
| SWCLK          | GPIO14 |
| SWDIO          | GPIO13 |
| TVCC           | 3V3    |
| GND            | GND    |

--------------

| JTAG               |         |
|--------------------|---------|
| TCK                | GPIO14  |
| TMS                | GPIO13  |
| TDI                | GPIO18  |
| TDO                | GPIO19  |
| nTRST \(optional\) | GPIO25  |
| nRESET             | GPIO26  |
| TVCC               | 3V3     |
| GND                | GND     |

</details>


----


## Build And Flash

You can build locally or use Github Action to build online and then download firmware to flash.


### General build and Flash

<details>
<summary>ESP32C3</summary>

1. Get esp-idf

    For now, please use esp-idf v5.2.1 : https://github.com/espressif/esp-idf/releases/tag/v5.2.1

2. Build & Flash

    Build with ESP-IDF build system.
    More information can be found at the following link: [Build System](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html "Build System")

The following example shows a possible way to build:

```bash
# Set build target
idf.py set-target esp32c3
# Build
idf.py build
# Flash
idf.py -p /dev/ttyS5 flash
```

</details>

## Usage

1. Get USBIP project

- Windows: [usbip-win](https://github.com/cezanne/usbip-win) .
- Linux: Distributed as part of the Linux kernel, but we have not yet tested on Linux platform, and the following instructions are all under Windows platform.

2. Start ESP chip and connect it to the device to be debugged

3. Connect it with usbip:

```bash
# HID Mode only
# for pre-compiled version on SourceForge
# or usbip old version
.\usbip.exe -D -a <your-esp-device-ip-address>  1-1

# 👉 Recommend
# HID Mode Or WinUSB Mode
# for usbip-win 0.3.0 kmdf ude
.\usbip.exe attach_ude -r <your-esp-device-ip-address> -b 1-1

```

If all goes well, you should see your device connected.

![image](https://user-images.githubusercontent.com/17078589/107849548-f903d780-6e36-11eb-846f-3eaf0c0dc089.png)


Here, we use MDK for testing:

![target](https://user-images.githubusercontent.com/17078589/73830040-eb3c6f00-483e-11ea-85ee-c40b68a836b2.png)


------

## FAQ

### Keil is showing a "RDDI-DAP ERROR" or "SWD/JTAG Communication Failure" message.

1. Check your line connection. Don't forget the 3v3 connection cable.
2. Check that your network connection is stable.


### DAP is slow or often abnormal.

Note that this project is sensitive to the network environment. If you are using a hotspot on your computer, you can try using network analyzer such as wireshark to observe the status of your AP network. During the idle time, the network should stay silent, while in the working state, there should be no too much packet loss.

Some LAN broadcast packets can cause serious impact, including:
- DropBox LAN Sync
- Logitech Arx Control
- ...


It is also affected by the surrounding radio environment, your AP situation (some NICs have terrible AP performance), distance, etc.


----

## Document

### Speed Strategy

The maximum rate of esp8266 pure IO is about 2MHz.
When you select max clock, we will take the following actions:

- `clock < 2Mhz` : Similar to the clock speed you choose.
- `2MHz <= clock < 10MHz` : Use the fastest pure IO speed.
- `clock >= 10MHz` : SPI acceleration using 40MHz clock.

> Note that the most significant speed constraint of this project is still the TCP connection speed.


### For OpenOCD user

This project was originally designed to run on Keil, but now you can also perform firmware flash on OpenOCD.

Note that if you want to use a 40MHz SPI acceleration, you need to specify the speed after the target device is connected, otherwise it will fail with the beginning.

```bash
# Run before approaching the flash command
> adapter speed 10000

> halt
> flash write_image [erase] [unlock] filename [offset] [type]
```

> Keil's timing handling is somewhat different from OpenOCD's. For example, OpenOCD lacks the SWD line reset sequence before reading the `IDCODE` registers.

------

## Credit


Credits to the following project, people and organizations:

> - https://www.github.com/windowsair/wireless-esp8266-dap origin of this project
> - https://github.com/thevoidnn/esp8266-wifi-cmsis-dap for adapter firmware based on CMSIS-DAP v1.0
> - https://github.com/ARM-software/CMSIS_5 for CMSIS
> - https://github.com/cezanne/usbip-win for usbip windows

- [@windowsair](https://www.github.com/windowsair/wireless-esp8266-dap)
- [@HeavenSpree](https://www.github.com/HeavenSpree)
- [@Zy19930907](https://www.github.com/Zy19930907)
- [@caiguang1997](https://www.github.com/caiguang1997)
- [@ZhuYanzhen1](https://www.github.com/ZhuYanzhen1)


## License

[Apache2.0 LICENSE](LICENSE)
