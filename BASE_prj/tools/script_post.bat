@echo off
if exist output\main_merge.mva (del output\main_merge.mva)
cd ..
grep '^#define CFG_DOUBLE_KEY_EN' system_config\app_config.h | grep '//'&& set config3 = -k 0 || set config3 = -k 1
tools\merge_script\Andes_MVAGenerate.exe %config3 % 
del zs.bin
pause