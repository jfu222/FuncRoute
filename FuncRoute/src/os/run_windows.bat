
set version=2.0.1.0

if not "%1" == "" set version=%1

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64
@rem nmake.exe -f Makefile.vs2013 DEBUG=0 PLATFORM=x64 PROJECT_VERSION=%version%
nmake.exe -f Makefile.vs2013 DEBUG=1 PLATFORM=x64 PROJECT_VERSION=%version%

pause