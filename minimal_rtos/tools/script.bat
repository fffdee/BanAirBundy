cd ..
if exist mpx (rd /s /q mpx)
md mpx
for %%i in (remind_res//*.mp3) do (tools\remind_script\ffmpeg.exe -i remind_res//%%i -ar 44100 -ab 32000 -ac 1 mpx//%%i.mp2 >> info.txt 2>&1)
del info.txt
cd mpx
ren *.mp2 *.
cd ..
tools\remind_script\MergeAudio2BinNew.exe -a 0x0 -i ..\..\mpx
rd /s /q mpx
fc tools\remind_script\sound_remind_item.h audio\remind_sound_item.h
if %errorlevel%==1 (copy tools\remind_script\sound_remind_item.h audio\remind_sound_item.h)