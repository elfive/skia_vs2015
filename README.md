# compile skia for windows visual studio 2015

## setup build environment

### setup proxy server

you can skip this, if your do not want use a proxy server.

```
git config --global http.proxy=http://127.0.0.1:1080
git config --global https.proxy=http://127.0.0.1:1080
```

### clone this repo

```
git clone -b build_for_vs2015 https://github.com/elfive/skia_vs2015.git
```

and we assume your skia code is locate at `C:\Users\yourname\Desktop\skia_vs2015`

### install python

only version `2.7.x` works here, and don't forget to add python.exe to your system PATH variable.

python 2.7.18 for windows 64bit download：<https://www.python.org/ftp/python/2.7.18/python-2.7.18.amd64.msi>

 

### setup environment variables

then you should add those paths to your system PATH environment variable

```
C:\Users\yourname\Desktop\skia_vs2015\bin
C:\Users\yourname\Desktop\skia_vs2015\depot_tools
```

or just export at the same terminal(batch file or cmd.exe) if you don't want change system configure:

```
set PATH=%PATH%;C:\Users\yourname\Desktop\skia_vs2015\bin;C:\Users\yourname\Desktop\skia_vs2015\depot_tools
```

 

### download dependencies

using skia `git-sync-deps` sync command in your skia directory

```
python tools/git-sync-deps
```

it will download all the build tools and skia dependencies needed for build skia. so make sure all the content are correctly downloaded.

you can verify it by just rerun the `git-sync-deps` command, it will show all dependencies and it's version or commit id.

#### some sync issue and it's solution:

1. 404 or can not find git repo issue:

   for vs2015 build we can only use old version of skia and old dependencies, and maybe some old dependencies removed by google, so we need to modify `skia_vs2015/DEPS`, to some alternative repo(always remember version number or commit id after @ should not be changed.)

   for those deleted repo can no longer download, you could find mirror repo at github.com. we recomand two users, if you need to do so, most outdated repo can be found at their repo list：

   ```
   https://github.com/onlyyou2023
   https://github.com/GoogleDepends
   ```

   after you modified `DEPS`, rerun `git-sync-deps` command, until all the contents are downloaded ccorrectly.

    

2. `xxx is not a git directory` error

   first delete the directory(if you don't know where it is, you can find it in `skia_vs2015/DEPS` file.), you should rerun `git-sync-deps` command after doing so.

    

### generate visual studio 2015 solution file

we use `bin/gn gen` command to generate vs sln file, if you don't know how to use it, you should head to [this web page](https://gn.googlesource.com/gn/) or [this web page](https://skia.org/docs/user/build/) to find answers for yourselves.

#### command to generate static libraries(.lib) build solution:

```
gn gen vs2015/static_release_x64 --args='is_component_build=false is_official_build=true target_cpu=\"x64\" skia_enable_discrete_gpu=false skia_enable_pdf=false skia_enable_effects=false skia_use_libjpeg_turbo=false skia_use_libpng=false skia_use_libwebp=false skia_use_egl=false skia_use_angle=false skia_use_expat=false skia_use_dng_sdk=false is_debug=false skia_enable_gpu=false extra_cflags=[\"/MD\"]' --ide=vs --sln=skia
gn gen vs2015/static_release_x86 --args='is_component_build=false is_official_build=true target_cpu=\"x86\" skia_enable_pdf=false skia_enable_pdf=false skia_enable_effects=false skia_use_libjpeg_turbo=false skia_use_libpng=false skia_use_libwebp=false skia_use_egl=false skia_use_angle=false skia_use_expat=false skia_use_dng_sdk=false is_debug=false skia_enable_gpu=false extra_cflags=[\"/MD\"]' --ide=vs --sln=skia
gn gen vs2015/static_debug_x86 --args='is_component_build=false is_official_build=false target_cpu=\"x86\" skia_enable_pdf=false skia_enable_pdf=false skia_enable_effects=false skia_use_libjpeg_turbo=false skia_use_libpng=false skia_use_libwebp=false skia_use_egl=false skia_use_angle=false skia_use_expat=false skia_use_dng_sdk=false is_debug=true skia_enable_gpu=false extra_cflags=[\"/MDd\"]' --ide=vs --sln=skia
gn gen vs2015/static_debug_x64 --args='is_component_build=false is_official_build=false target_cpu=\"x64\" skia_enable_pdf=false skia_enable_pdf=false skia_enable_effects=false skia_use_libjpeg_turbo=false skia_use_libpng=false skia_use_libwebp=false skia_use_egl=false skia_use_angle=false skia_use_expat=false skia_use_dng_sdk=false is_debug=true skia_enable_gpu=false extra_cflags=[\"/MDd\"]' --ide=vs --sln=skia
```

files can be founded whthin `skia_vs2015/vs2015` directory

 

#### command to generate dynamic libraries(.dll) build solution：

we generate  

```
gn gen vs2015/dynamic_release_x64 --args='is_component_build=true is_official_build=true target_cpu=\"x64\" skia_enable_pdf=false skia_enable_pdf=false skia_enable_effects=false skia_use_libjpeg_turbo=false skia_use_libpng=false skia_use_libwebp=false skia_use_egl=false skia_use_angle=false skia_use_expat=false skia_use_dng_sdk=false is_debug=false skia_enable_gpu=false extra_cflags=[\"/MD\"]' --ide=vs --sln=skia
gn gen vs2015/dynamic_release_x86 --args='is_component_build=true is_official_build=true target_cpu=\"x86\" skia_enable_pdf=false skia_enable_pdf=false skia_enable_effects=false skia_use_libjpeg_turbo=false skia_use_libpng=false skia_use_libwebp=false skia_use_egl=false skia_use_angle=false skia_use_expat=false skia_use_dng_sdk=false is_debug=false skia_enable_gpu=false extra_cflags=[\"/MD\"]' --ide=vs --sln=skia
gn gen vs2015/dynamic_debug_x86 --args='is_component_build=true is_official_build=false target_cpu=\"x86\" skia_enable_pdf=false skia_enable_pdf=false skia_enable_effects=false skia_use_libjpeg_turbo=false skia_use_libpng=false skia_use_libwebp=false skia_use_egl=false skia_use_angle=false skia_use_expat=false skia_use_dng_sdk=false is_debug=true skia_enable_gpu=false extra_cflags=[\"/MDd\"]' --ide=vs --sln=skia
gn gen vs2015/dynamic_debug_x64 --args='is_component_build=true is_official_build=false target_cpu=\"x64\" skia_enable_pdf=false skia_enable_pdf=false skia_enable_effects=false skia_use_libjpeg_turbo=false skia_use_libpng=false skia_use_libwebp=false skia_use_egl=false skia_use_angle=false skia_use_expat=false skia_use_dng_sdk=false is_debug=true skia_enable_gpu=false extra_cflags=[\"/MDd\"]' --ide=vs --sln=skia
```

files can be founded whthin `skia_vs2015/vs2015` directory

 

## compile

### compile 64bit version

just open the `skia.sln` file and then compile project `skia`, after that you can find the `skia.lib` binary at the same directory of `skia.sln`

 

### compile 32bit version

1. modify the `toolchain.ninja` file locate at the same directory of `skia.sln`, replace all the content(like below)

   ```
   cmd /c C:/Program Files (x86)/Microsoft Visual Studio 14.0/win_sdk/bin/SetEnv.cmd /x86 && C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/bin/amd64_x86/
   ```

   to this(notice that there is a space' ' behind &&):

   ```
   cmd /c vcvarsall.bat x86 && 
   ```

2. then add this paths to your system variables(or just export it should do the same work):

   the path may different if you installed your visual studio to a different location.

   ```
   C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC
   C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\amd64_x86\
   ```

3. open the `skia.sln` file and then compile project `skia`, after that you can find the `skia.lib` binary at the same directory of `skia.sln`

 

##  one more thing

if you do not wanna show the include info whild compile skia, you just need modify the `toolchain.ninja` file, delete all the `/showIncludes` options.

 

## use

copy `skia_vs2015/include` folder to whatever you like, setup the include paths for your project, then link `skia.lib`, and you should be ok.
