set curdir=%~dp0
cd %curdir%

mkdir build_vs2019
cd build_vs2019

if not exist %VCPKG_ROOT%\vcpkg.exe (
	call %VCPKG_ROOT%\bootstrap-vcpkg.bat
)
%VCPKG_ROOT%\vcpkg.exe install hiredis:x64-windows-v142
%VCPKG_ROOT%\vcpkg.exe install spdlog:x64-windows-v142
%VCPKG_ROOT%\vcpkg.exe install libevent:x64-windows-v142
%VCPKG_ROOT%\vcpkg.exe install libuv:x64-windows-v142
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET="x64-windows-v142" -G "Visual Studio 16 2019"
cd %curdir%
