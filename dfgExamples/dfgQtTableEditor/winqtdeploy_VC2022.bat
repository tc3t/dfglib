@rem Helper script for creating release package from VC2022 build.

@echo off

@rem set_deploy_env_VC0222.bat is machine-specific file that is expected to set the following variables:
@rem QT_BIN_DIR, value <Qt install path>\<Qt version>\<compiler folder, e.g. msvc2019_64>\bin\
@rem MSVC_REDIST_PATH_2022, value e.g. <VS 2022 install dir>\VC\Redist\MSVC\14.16.27012\x64\Microsoft.VC141.CRT\
@rem VCINSTALLDIR_2022, value <VS 2022 install dir>\VC
call set_deploy_env_VC2022.bat

set EXE_PATH=TODO_set_exe_path_here
set PDB_PATH=TODO_set_pdb_path_here
set VCINSTALLDIR=%VCINSTALLDIR_2022%
call winqtdeploy.bat %QT_BIN_DIR% %EXE_PATH% %PDB_PATH% "%MSVC_REDIST_PATH_2022%"
