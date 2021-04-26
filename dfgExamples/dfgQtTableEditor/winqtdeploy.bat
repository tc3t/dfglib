@REM Helper script for creating a binary distribution package of dfgQtTableEditor
@REM Usage:
@REM
@REM	Takes three parameters and creates release packages to .7z files.
@REM
@REM	Parameters
@REM		0: QT bin dir path
@REM		1: dfgQtTableEditor.exe path
@REM		2: MSVC runtime REDIST dir path
@REM

@echo off

set QT_BIN_DIR=%1
set EXE_PATH=%2
set PDB_PATH=%3
set MSVC_RUNTIME_REDIST_PATH=%4

@echo QT_BIN_DIR is "%QT_BIN_DIR%"
@echo EXE_PATH is "%EXE_PATH%"
@echo PDB_PATH is "%PDB_PATH%"
@echo MSVC_RUNTIME_REDIST_PATH is %MSVC_RUNTIME_REDIST_PATH%

@echo --------------------------------
@echo Checking input parameters
if not "%QT_BIN_DIR%" NEQ "" goto :usage
if not "%EXE_PATH%" NEQ "" goto :usage
if not "%PDB_PATH%" NEQ "" goto :usage

set OUTPUT_WORKDIR=winqtdeploy_sandbox
set PACKAGE_FOLDER_NAME=dfgQtTableEditor
set PACKAGE_OUTPUT_PATH=%OUTPUT_WORKDIR%\%PACKAGE_FOLDER_NAME%

@echo Output workdir is %OUTPUT_WORKDIR%
@echo Package output path is %PACKAGE_OUTPUT_PATH%

@echo --------------------------------
@echo Removing old workdir
rd /S %OUTPUT_WORKDIR%

@echo --------------------------------
@echo Running windeployqt with output path %PACKAGE_OUTPUT_PATH%

@rem Notes on windeployqt
@rem     --no-compiler-runtime is specified as it does not copy runtime dll's but vc_redist.x64.exe installer.
@rem     Others are mostly to avoid package bloat from unneeded stuff.
%QT_BIN_DIR%\windeployqt -dir %PACKAGE_OUTPUT_PATH% -no-translations -no-system-d3d-compiler -no-opengl-sw --no-compiler-runtime %EXE_PATH%

if "%errorlevel%" NEQ "0" (
	@echo windeployqt failed; terminating package creation.
	exit /b 1
)

@echo --------------------------------
@echo Copying exe-file to output dir
copy %EXE_PATH% %PACKAGE_OUTPUT_PATH%
@echo Copying pdb-file to work dir (note: not to package dir as don't want to include pdb in package)
copy %PDB_PATH% %OUTPUT_WORKDIR%

@echo --------------------------------
@echo Copying MSVC runtime DLL's
copy %MSVC_RUNTIME_REDIST_PATH%\msvcp140.dll %PACKAGE_OUTPUT_PATH%
copy %MSVC_RUNTIME_REDIST_PATH%\msvcp140_1.dll %PACKAGE_OUTPUT_PATH%
copy %MSVC_RUNTIME_REDIST_PATH%\msvcp140_2.dll %PACKAGE_OUTPUT_PATH%
copy %MSVC_RUNTIME_REDIST_PATH%\vcruntime140.dll %PACKAGE_OUTPUT_PATH%

@echo --------------------------------
@echo Removing some unneeded plugins
del %PACKAGE_OUTPUT_PATH%\imageformats\qgif.dll
del %PACKAGE_OUTPUT_PATH%\imageformats\qjpeg.dll
del %PACKAGE_OUTPUT_PATH%\imageformats\qtiff.dll
del %PACKAGE_OUTPUT_PATH%\imageformats\qwebp.dll

@echo --------------------------------
@echo Copying examples
mkdir %PACKAGE_OUTPUT_PATH%\examples
copy examples\bars_example_qcp_bar_chart_demo_mimic.csv %PACKAGE_OUTPUT_PATH%\examples\
copy examples\bars_example_qcp_bar_chart_demo_mimic.csv.conf %PACKAGE_OUTPUT_PATH%\examples\
copy examples\number_generator_example.csv %PACKAGE_OUTPUT_PATH%\examples\
copy examples\number_generator_example.csv.conf %PACKAGE_OUTPUT_PATH%\examples\

@echo --------------------------------
@echo Copying guide
copy ..\..\dfg\qt\res\chartGuide.html %PACKAGE_OUTPUT_PATH%

@echo --------------------------------
@echo Extracting version info from executable
set EXE_VERSION_FILE_PATH=%OUTPUT_WORKDIR%\exe_version.txt
powershell "(Get-Item -path %PACKAGE_OUTPUT_PATH%\dfgQtTableEditor.exe).VersionInfo.ProductVersion" > %EXE_VERSION_FILE_PATH%
set /P EXE_VERSION= < %EXE_VERSION_FILE_PATH%
del %EXE_VERSION_FILE_PATH%
@echo Exe version is %EXE_VERSION%

@echo --------------------------------
@echo Creating .7z packages
set PACKAGE_PATH_FULL=%OUTPUT_WORKDIR%\dfgQtTableEditor_%EXE_VERSION%_full.7z
set PACKAGE_PATH_UDPATE=%OUTPUT_WORKDIR%\dfgQtTableEditor_%EXE_VERSION%_update.7z
set PACKAGE_PATH_PDB=%OUTPUT_WORKDIR%\dfgQtTableEditor_%EXE_VERSION%_pdb.7z
cd %OUTPUT_WORKDIR%
"C:\Program Files\7-Zip\7z.exe" a -mx9 ..\%PACKAGE_PATH_FULL% %PACKAGE_FOLDER_NAME%
cd ..
"C:\Program Files\7-Zip\7z.exe" a -mx9 %PACKAGE_PATH_UDPATE% %EXE_PATH%
"C:\Program Files\7-Zip\7z.exe" a -mx9 %PACKAGE_PATH_PDB% %PDB_PATH%

@echo --------------------------------
@echo Creating hash files for the packages.
powershell "(Get-FileHash %PACKAGE_PATH_FULL%).Hash" > %PACKAGE_PATH_FULL%.sha256
powershell "(Get-FileHash %PACKAGE_PATH_UDPATE%).Hash" > %PACKAGE_PATH_UDPATE%.sha256
powershell "(Get-FileHash %PACKAGE_PATH_PDB%).Hash" > %PACKAGE_PATH_PDB%.sha256

@echo --------------------------------
@echo All done.

goto :exit

:usage
	@echo Not all expected parameters were given.

:exit
