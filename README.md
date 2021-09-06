# LiTian-LittleFS
LiTian LittleFS.

# GITHUB 地址
https://github.com/littlefs-project
https://github.com/littlefs-project/littlefs
https://github.com/littlefs-project/littlefs-fuse
https://github.com/ARMmbed/littlefs

# 官方介绍
https://os.mbed.com/blog/entry/littlefs-high-integrity-embedded-fs/
http://littlefs.geky.net/demo.html
https://os.mbed.com/teams/mbed-os-examples/code/mbed-os-example-filesystem/

# 第三方仓库
https://github.com/armink/SFUD
https://github.com/armink/EasyLogger
https://github.com/RT-Thread-packages/fal
https://github.com/amouiche/lfs-tools

https://github.com/songwenshuai/LiTian-LittleFS.git

# 文章
https://mp.weixin.qq.com/s/y7hIQr1058twOELl-mnSBA
https://mp.weixin.qq.com/s/amg8zSMv-PL7EAlIkOsQ5g

``` c
1 littlefs主要用在微控制器和SPI flash上,是一种嵌入式文件系统.主要有3个特点：
1) 掉电恢复
在写入时即使复位或者掉电也可以恢复到上一个正确的状态.
2) 擦写均衡
有效延长flash的使用寿命
3) 有限的RAM/ROM
节省ROM和RAM空间
2 已有的文件系统
1）非掉电恢复,基于block的文件系统,常见的有FAT和EXT2.这两个文件系统在写入文件时是原地更新的,不具备非掉电恢复的特性.
2) 日志式的文件系统,比如JFFS,YAFFS等,具备掉电恢复的特性.但是这几个系统消耗了太多的RAM,且性能较低.
3) EXT4和COW类型的btrfs具有良好的恢复性和读写性能,但是需要的资源过多,不适合小型的嵌入式系统.

littlefs综合了日志式文件系统和COW文件系统的优点
从sub-block的角度来看,littlefs是基于日志的文件系统,提供了metadata的原子更新
从super-block的角度,littlefs是基于block的COW树.
```