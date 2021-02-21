@rem Helper script for creating release package from VC2017 build.

@echo off

@rem set_deploy_env.bat is machine-specific file that is expected to set the following variables:
@rem QT_BIN_DIR, value <Qt install path>\<Qt version>\<compiler folder, e.g. msvc2017_64>\bin\
@rem MSVC_REDIST_PATH_2017, value e.g. <VS 2017 install dir>\VC\Redist\MSVC\14.16.27012\x64\Microsoft.VC141.CRT\
@rem VCINSTALLDIR_2017, value <VS 2017 install dir>\VC
call set_deploy_env.bat

set EXE_PATH=TODO_set_exe_path_here.exe
set PDB_PATH=TODO_set_pdb_path_here.exe
set VCINSTALLDIR=%VCINSTALLDIR_2017%
call winqtdeploy.bat %QT_BIN_DIR% %EXE_PATH% %PDB_PATH% "%MSVC_REDIST_PATH_2017%"
