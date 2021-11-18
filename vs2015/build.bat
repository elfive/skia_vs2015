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
	echo ��ȷ�� Visual Studio 2015 �Ƿ���ȷ��װ��
)

pause
exit

:build
set version=%1
set config=%2
set solution_filename=%cwd%%version%\skia.sln
set target_bin=%cwd%%version%\skia.lib
echo ���±����������ļ���%solution_filename% "GN|%config%"
%devnev% %solution_filename% /build "GN|%config%" /project "skia"
if exist %target_bin% (
	echo ���±����������ļ��ɹ�
) else (
	echo �޷��ҵ���%target_bin%
	echo �����������ļ�ʧ�ܣ�
	pause
	exit
)

goto:eof
