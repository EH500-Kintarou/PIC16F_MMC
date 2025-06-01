[English Readme is here](https://github.com/EH500-Kintarou/PIC16F_MMC/blob/master/README-en.md)

# Enhanced PIC16FシリーズでのMMCアクセスサンプル

PIC16F18857など、一部のEnhanced midrange PIC16Fシリーズは、FATファイルシステムを持つMMC互換カード（要するにSDカード）の読み書きをできるだけの非常に大きなプログラムメモリとデータメモリを持っています。

このプロジェクトは、ChaN氏のFatFsを使用してMMCの読み書きを実施するためのサンプルプロジェクトです。

[FatFs - Generic FAT Filesystem Module](https://elm-chan.org/fsw/ff/)

![](https://img.shields.io/badge/MPLAB_X-6.20-silver?style=plastic)
![](https://img.shields.io/badge/XC8-3.00-blue?style=plastic)
![](https://img.shields.io/badge/FatFs-R0.15a-purple?style=plastic)
![](https://img.shields.io/badge/PIC16F-18857-red?style=plastic)

# 環境

- OS: Windows 11
- IDE: MPLAB X 6.20
- Compiler: XC8 3.00
- MCU: PIC16F18857
- Library: FatFs R0.15a

# 回路図

![Circuit Diagram](https://raw.githubusercontent.com/EH500-Kintarou/PIC16F_MMC/master/Images/circuit.png)
