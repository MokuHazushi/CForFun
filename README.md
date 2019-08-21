# CForFun
This GitHub aims to be a testing platform for Linux Kernel development and study. It compiles several project to explore specific aspects of the low-level system through simplified re-implementation of specific features or through various module development.

## Project #1 : Simple Master Boot Record (MBR) Analyser
Corresponding to the first 512 bytes of a storing device (e.g HDD and such), the Master Boot Record (MBR) is the place loaded by the BIOS at the booting phase. It is segmented as the following :
- The Boot loader (for example GRUB), an executable code in charge of loading the kernel
- The partition table describing the 4 partitions of the device
- The magic number to validate the MBR

This project takes a storing device file as argument (typically /dev/sda or VM image files), read the first 512 bytes and tries to analyse the partition table. One usage example is shown below :
```
./bin/partition_record_reader sample/archvm.bin
```
![Example screenshot](doc/simpleMBRAnalyzer_example.png)
