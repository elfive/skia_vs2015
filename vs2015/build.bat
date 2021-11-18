@echo off

set cwd=%~dp0
set devnev="C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\IDE\devenv.com"
if exist %devnev% (
	if "%1"=="static_debug_x64" (
		call:build static_debug_x64 x64
	) else if "%1"=="static_debug_x86" (
		call:build static_debug_x86 Win32
	) else if "%1"=="static_release_x64" (
		call:build static_release_x64 x64
	) else if "%1"=="static_release_x86" (
		call:build static_release_x86 Win32
	) else (
	rem build all
		call:build static_debug_x64 x64
		call:build static_debug_x86 Win32
		call:build static_release_x64 x64
		call:build static_release_x86 Win32
	)
) else (
	echo 请确认 Visual Studio 2015 是否正确安装！
)

pause
exit

:build
set version=%1
set config=%2
set solution_filename=%cwd%%version%\skia.sln
set target_bin=%cwd%%version%\skia.lib
echo 重新编译解决方案文件：%solution_filename% "GN|%config%"
%devnev% %solution_filename% /build "GN|%config%" /project "skia"
if exist %target_bin% (
	echo 重新编译解决方案文件成功
) else (
	echo 无法找到：%target_bin%
	echo 编译解决方案文件失败！
	pause
	exit
)

goto:eof
