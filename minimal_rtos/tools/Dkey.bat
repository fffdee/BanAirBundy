@echo off
cd ..
del *.sag
del *.ld
grep '^#define CFG_DOUBLE_KEY_EN' system_config\app_config.h
if %errorlevel%==0 (goto found) else (goto not_found)
:found
grep '^#define CFG_DOUBLE_KEY_EN' system_config\app_config.h | grep '//'
if %errorlevel%==1 (nds_ldsag.exe -t tools\nds32_template.txt tools\nds32-ae210p_Dkey.sag -o nds32-ae210p.ld) else (nds_ldsag.exe -t tools\nds32_template.txt tools\nds32-ae210p.sag -o nds32-ae210p.ld)
exit
:not_found
nds_ldsag.exe -t tools\nds32_template.txt tools\nds32-ae210p.sag -o nds32-ae210p.ld
exit