# 编译skia

## 编译环境搭建

### 安装 python

只能使用 `2.7.x` 版本，注意要把 `python.exe` 所在的路径加入到系统环境变量中

附下载地址：<https://www.python.org/ftp/python/2.7.18/python-2.7.18.amd64.msi>

 

### 下载编译工具和源代码

1. 推荐先配置代理

```
git config --global http.proxy=http://127.0.0.1:1080
git config --global https.proxy=http://127.0.0.1:1080
```

1. 同步编译工具和 skia 源代码

```
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
git clone https://skia.googlesource.com/skia.git
```

如果需要指定 skia 版本（例如使用 vs2015 编译时），可以在 skia 目录下 checkout 指定版本：

```
git checkout dfcb14a34920244924eeb6ff874c10d7387246f5 -b build_for_vs2015
```

 

### 配置环境变量

在系统环境变量 path 中加入如下路径：

```
C:\Users\elfive\Desktop\skia\bin
C:\Users\elfive\Desktop\skia\depot_tools
```

或者在同一个终端中使用 set 命令将额外的路径导出：

```
set PATH=%PATH%;C:\Users\elfive\Desktop\skia\bin;C:\Users\elfive\Desktop\skia\depot_tools
```

 

### 下载依赖项

使用同步命令同步依赖（在 skia 目录下运行）

```
python tools/git-sync-deps
```

#### 同步过程中常见问题及解决办法:

1. 404等下载错误或者找不到 git 仓库的错误

   这个是因为 google 代码的 git 库被移除了，需要修改 skia 下 `DEPS` 文件中的库路径（注意：@后面的版本不能修改）

   对于找不到的库，可以去 github 搜索相关的 repo，该 repo 要能够找到对应的 commit 或者对应的 tag。 这里推荐两个 github 用户，他们有对 google 相关的依赖库进行备份，大部分可以在他们的 repo 列表中找到：

   ```
   https://github.com/onlyyou2023
   https://github.com/GoogleDepends
   ```

   修改 `DEPS` 文件后，再次运行同步命令，直到所有依赖都正确下载且能显示出版本

    

2. `xxx is not a git directory` 错误

   删除有问题的目录，目录的具体路径可以先查看日志确定出问题的仓库，然后根据该仓库名在`skia/DEPS`文件中找到，删除后再重新运行同步指令即可。

    

### 生成 vs 解决方案文件

#### 生成静态库解决方案文件：

```
gn gen vs2015/static_release_x64 --args='is_component_build=false is_official_build=true target_cpu=\"x64\" skia_enable_discrete_gpu=false skia_enable_pdf=false skia_enable_effects=false skia_use_libjpeg_turbo=false skia_use_libpng=false skia_use_libwebp=false skia_use_egl=false skia_use_angle=false skia_use_expat=false skia_use_dng_sdk=false is_debug=false skia_enable_gpu=false extra_cflags=[\"/MD\"]' --ide=vs --sln=skia
gn gen vs2015/static_release_x86 --args='is_component_build=false is_official_build=true target_cpu=\"x86\" skia_enable_pdf=false skia_enable_pdf=false skia_enable_effects=false skia_use_libjpeg_turbo=false skia_use_libpng=false skia_use_libwebp=false skia_use_egl=false skia_use_angle=false skia_use_expat=false skia_use_dng_sdk=false is_debug=false skia_enable_gpu=false extra_cflags=[\"/MD\"]' --ide=vs --sln=skia
gn gen vs2015/static_debug_x86 --args='is_component_build=false is_official_build=false target_cpu=\"x86\" skia_enable_pdf=false skia_enable_pdf=false skia_enable_effects=false skia_use_libjpeg_turbo=false skia_use_libpng=false skia_use_libwebp=false skia_use_egl=false skia_use_angle=false skia_use_expat=false skia_use_dng_sdk=false is_debug=true skia_enable_gpu=false extra_cflags=[\"/MDd\"]' --ide=vs --sln=skia
gn gen vs2015/static_debug_x64 --args='is_component_build=false is_official_build=false target_cpu=\"x64\" skia_enable_pdf=false skia_enable_pdf=false skia_enable_effects=false skia_use_libjpeg_turbo=false skia_use_libpng=false skia_use_libwebp=false skia_use_egl=false skia_use_angle=false skia_use_expat=false skia_use_dng_sdk=false is_debug=true skia_enable_gpu=false extra_cflags=[\"/MDd\"]' --ide=vs --sln=skia
```

 

#### 生成动态库解决方案文件：

```
gn gen vs2015/dynamic_release_x64 --args='is_component_build=true is_official_build=true target_cpu=\"x64\" skia_enable_pdf=false skia_enable_pdf=false skia_enable_effects=false skia_use_libjpeg_turbo=false skia_use_libpng=false skia_use_libwebp=false skia_use_egl=false skia_use_angle=false skia_use_expat=false skia_use_dng_sdk=false is_debug=false skia_enable_gpu=false extra_cflags=[\"/MD\"]' --ide=vs --sln=skia
gn gen vs2015/dynamic_release_x86 --args='is_component_build=true is_official_build=true target_cpu=\"x86\" skia_enable_pdf=false skia_enable_pdf=false skia_enable_effects=false skia_use_libjpeg_turbo=false skia_use_libpng=false skia_use_libwebp=false skia_use_egl=false skia_use_angle=false skia_use_expat=false skia_use_dng_sdk=false is_debug=false skia_enable_gpu=false extra_cflags=[\"/MD\"]' --ide=vs --sln=skia
gn gen vs2015/dynamic_debug_x86 --args='is_component_build=true is_official_build=false target_cpu=\"x86\" skia_enable_pdf=false skia_enable_pdf=false skia_enable_effects=false skia_use_libjpeg_turbo=false skia_use_libpng=false skia_use_libwebp=false skia_use_egl=false skia_use_angle=false skia_use_expat=false skia_use_dng_sdk=false is_debug=true skia_enable_gpu=false extra_cflags=[\"/MDd\"]' --ide=vs --sln=skia
gn gen vs2015/dynamic_debug_x64 --args='is_component_build=true is_official_build=false target_cpu=\"x64\" skia_enable_pdf=false skia_enable_pdf=false skia_enable_effects=false skia_use_libjpeg_turbo=false skia_use_libpng=false skia_use_libwebp=false skia_use_egl=false skia_use_angle=false skia_use_expat=false skia_use_dng_sdk=false is_debug=true skia_enable_gpu=false extra_cflags=[\"/MDd\"]' --ide=vs --sln=skia
```

 

## 编译

### 64位编译

打开64位对应的解决方案文件，编译 skia 项目即可在解决方案目录下找到生成的二进制文件

 

### 32位编译

1. 修改解决方案目录下 `toolchain.ninja` 文件，将所有类似

   ```
   cmd /c C:/Program Files (x86)/Microsoft Visual Studio 14.0/win_sdk/bin/SetEnv.cmd /x86 && C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/bin/amd64_x86/
   ```

   的地方替换为**（注意 && 后面需要手动添加一个空格）**：

   ```
   cmd /c vcvarsall.bat x86 && 
   ```

2. 将路径添加到系统环境变量中去，例如：

   ```
   C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC
   C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\amd64_x86\
   ```

3. 打开32位对应的解决方案文件，编译 skia 项目即可在解决方案目录下找到生成的二进制文件

 

###  额外的

如果想在编译的时候显示过多的信息，例如：include文件包含关系，则需要修改对应的 toolchain.ninja 文件，将其中的所有 `/showIncludes` 删除即可。

 

## 使用

复制`skia/include`文件夹到对应的位置，并将其添加到附加包含目录中 引入`skia.lib`即可