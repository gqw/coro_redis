set curdir=%~dp0
cd %curdir%

mkdir build_vs2022
cd build_vs2022

if not exist %VCPKG_ROOT%\vcpkg.exe (
	call %VCPKG_ROOT%\bootstrap-vcpkg.bat
)
%VCPKG_ROOT%\vcpkg.exe install hiredis:x64-windows-v143 
%VCPKG_ROOT%\vcpkg.exe install spdlog:x64-windows-v143
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET="x64-windows-v143" -G "Visual Studio 17 2022"
cd %curdir%
