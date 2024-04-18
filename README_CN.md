## 简介

只需要**一枚ESP芯片**即可开始无线调试！通过USBIP协议栈和CMSIS-DAP协议栈实现。

> 👉在5米范围内，擦除并烧写100kb大小的固件(Hex固件) ：

<p align="center"><img src="https://user-images.githubusercontent.com/17078589/120925694-4bca0d80-c70c-11eb-91b7-ffa54770faea.gif"/></p>

----

对于Keil用户，我们现在支持[elaphureLink](https://github.com/windowsair/elaphureLink)。无需usbip即可开始您的无线调试之旅！

## 特性

1. 支持的ESP芯片
    - [x] ESP32
    - [x] ESP32C3

2. 支持的调试接口：
    - [x] SWD
    - [x] JTAG

3. 支持的USB通信协议：
    - [x] USB-HID
    - [x] WCID & WinUSB (默认)
4. 支持的调试跟踪器：
    - [x] TCP转发的串口

5. 其它
    - [x] 通过SPI接口加速的SWD协议（最高可达40MHz）
    - [x] 支持[elaphureLink](https://github.com/windowsair/elaphureLink)，无需驱动的快速Keil调试
    - [x] ...

## 连接你的开发板

### WIFI连接



固件中已经内置了一个mDNS服务。你可以通过`dap.local`的地址访问到设备。

![mDNS](https://user-images.githubusercontent.com/17078589/149659052-7b29533f-9660-4811-8125-f8f50490d762.png)


### 调试接口连接

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

## 编译固件并烧写

你可以在本地构建或使用Github Action在线构建固件，然后下载固件进行烧写。


### 在本地构建并烧写

<details>
<summary>ESP32C3</summary>

1. 获取esp-idf

    目前，请考虑使用esp-idf v5.2.1： https://github.com/espressif/esp-idf/releases/tag/v5.2.1

2. 编译和烧写

    使用ESP-IDF编译系统进行构建。
    更多的信息，请见：[Build System](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html "Build System")


下面例子展示了在Windows上完成这些任务的一种可行方法：

```bash
idf.py set-target esp32c3
# 编译
idf.py build
# 烧写
idf.py -p /dev/ttyS5 flash
```


> 位于项目根目录的`idf.py`脚本仅适用于较老的ESP8266设备，请不要在ESP32设备上使用。

</details>


> 我们还提供了预编译固件用于快速评估。详见 [Releases](https://github.com/windowsair/wireless-esp8266-dap/releases)



## 使用

1. 获取USBIP项目

- Windows: [usbip-win](https://github.com/cezanne/usbip-win)。
- Linux：USBIP作为Linux内核的一部分发布，但我们还没有在Linux平台上测试，下面的说明都是在Windows平台下的。

2. 启动ESP8266并且把ESP8266连接到同一个WIFI下。

3. 通过USBIP连接ESP8266：

```bash
# 仅HID模式，用于SourceForge上的预编译版本或者旧的USBIP版本。
.\usbip.exe -D -a <your-esp8266-ip-address>  1-1

# 👉 推荐。HID模式或者WinUSB模式。用于usbip-win 0.3.0 kmdf ude版本。
.\usbip.exe attach_ude -r <your-esp8266-ip-address> -b 1-1
```

如果一切顺利，你应该看到你的设备被连接，如下图所示。

![image](https://user-images.githubusercontent.com/17078589/107849548-f903d780-6e36-11eb-846f-3eaf0c0dc089.png)

下面我们用keil MDK来测试：

![target](https://user-images.githubusercontent.com/17078589/73830040-eb3c6f00-483e-11ea-85ee-c40b68a836b2.png)

------

## 经常会问的问题

### Keil提示“RDDI-DAP ERROR”或“SWD/JTAG Communication Failure”

1. 检查线路连接。别忘了连接3V3引脚。
2. 检查网络连接是否稳定。


## DAP很慢或者不稳定

注意，本项目受限于周围的网络环境。如果你在电脑上使用热点进行连接，你可以尝试使用wireshark等工具对网络连接进行分析。当调试闲置时，线路上应保持静默，而正常工作时一般不会发生太多的丢包。

一些局域网广播数据包可能会造成严重影响，这些包可能由这些应用发出：
- DropBox LAN Sync
- Logitech Arx Control
- ...


周围的射频环境同样会造成影响，此外距离、网卡性能等也可能是需要考虑的。


## 文档

### 速度策略

单独使用ESP8266通用IO时的最大翻转速率只有大概2MHz。当你选择最大时钟时，我们需要采取以下操作：

- `clock < 2Mhz` ：与你选择的时钟速度类似。
- `2MHz <= clock < 10MHz` ：使用最快的纯IO速度。
- `clock >= 10MHz` ：使用40MHz时钟的SPI加速。

> 请注意，这个项目最重要的速度制约因素仍然是TCP连接速度。

### 对于OpenOCD用户

这个项目最初是为在Keil上运行而设计的，但现在你也可以在OpenOCD上通过它来烧录程序。
注意，如果你想使用40MHz的SPI加速器，你需要在连接目标设备后指定速度，否则会在开始时失败。

```bash
# 在使用flash指令前需要先运行：
> adapter speed 10000

> halt
> flash write_image [erase] [unlock] filename [offset] [type]
```

> Keil的操作时序与OpenOCD的有些不同。例如，OpenOCD在读取 "IDCODE "寄存器之前缺少SWD线复位序列。

# 致谢

归功于以下项目、人员和组织。

> - https://www.github.com/windowsair/wireless-esp8266-dap origin of this project
> - https://github.com/thevoidnn/esp8266-wifi-cmsis-dap for adapter firmware based on CMSIS-DAP v1.0
> - https://github.com/ARM-software/CMSIS_5 for CMSIS
> - https://github.com/cezanne/usbip-win for usbip windows

- [@windowsair](https://www.github.com/windowsair/wireless-esp8266-dap)
- [@HeavenSpree](https://www.github.com/HeavenSpree)
- [@Zy19930907](https://www.github.com/Zy19930907)
- [@caiguang1997](https://www.github.com/caiguang1997)
- [@ZhuYanzhen1](https://www.github.com/ZhuYanzhen1)


## 许可证
[Apache 2.0 许可证](LICENSE)
