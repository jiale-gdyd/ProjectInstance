# ProjectInstance
工程应用实例


<a href="https://996.icu"><img src="https://img.shields.io/badge/link-996.icu-red.svg" alt="996.icu" /></a>
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)
[![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg)](https://github.com/996icu/996.ICU/blob/master/LICENSE)
## 一、构建应用，(默认master分支)可以根据configs/*_defconfig文件对功能进行裁剪

### 构建基于瑞星微rv1109/rv1126的应用
+ 构建基于rv1109/rv1126 soc的简单EMS应用
   ```shell
   ./build.sh rv11xx_media_ems          最终生成Rv11xxEMSApp可执行文件，可通过修改build.rv11xx.version中对应名字的版本号
   ```
### 构建基于爱心元智AX620A的应用
+ 构建基于爱心元智AX620A soc的gstreamer+ai项目
   ```shell
   ./build.sh ax620a_gst_api            最终生成GstAiApp可执行文件，可通过修改build.ax620a.version中对应名字的版本号
   ```

### 工程清理与帮助
+ 项目工程清理
   ```shell
   ./build.sh clean                     会将编译的信息全部清理干净
   ```
+ 项目构建帮助
   ```shell
   ./build.sh [help]                    显示项目构建帮助信息[help]表示可选，也可以直接执行./build.sh
   ```
