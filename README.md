# JLink Flush GUI 程序



采用Qt 6.0.3编写。

由于`JFlush.exe`下载程序操作起来比较麻烦，因此自己写了一个GUI来调用`JLink.exe`来实现一些基本功能。

软件包： `JLink_Flush_GUI\JLink_Flush_GUI\JLink_Flash_GUI.exe` 。



## 2021-09-03

新增：

* 增加多个固件功能。
* Bin文件地址可配置。
* 固件可选。
* 日志显示优化。
* Linux下进度显示。（Linux烧录Jlink不会有进度条窗口弹出，因此自己增加了一个进度条显示）

Bug：

* 启动时，未选择固件空间未禁用。

界面：

![ui](./image/README/ui_0902.png)



## First

### 运行

---

1. 配置`JLink.exe`。

   ![pic_ui](./image/README/ui.png)

   ![jlink_settings](./image/README/jlink_path_settings.png)

   ![download](./image/README/download.png)

2. 下载HEX固件。

   ![](D:\Code\gitee\Code\JLink_Flash_GUI\JLink_Flash_GUI\image\README\20210523165955733.png)
