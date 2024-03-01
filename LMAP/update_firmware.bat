@echo off

rem Pingを打ち続ける(mDNSの不調防止のため)
for /f "tokens=1,2 delims==;	 " %%i in ('wmic process call create "ping lmap2024.local -t"') do (
  if "%%i" equ "ProcessId" set ProcessId=%%j
)

rem SPIFFSを書き込む
echo SPIFFSを更新しています。
cd %~dp0
%HOMEPATH%\.platformio\penv\Scripts\platformio.exe run -t uploadfs
if %errorlevel% neq 0 (
  echo SPIFFS更新処理が異常終了しました。終了コード=%errorlevel%
  taskkill /pid %ProcessId%
  pause
  exit %errorlevel%
)
ping localhost -n 20
rem スケッチを書き込む
echo スケッチを更新しています。
%HOMEPATH%\.platformio\penv\Scripts\platformio.exe run -t upload
if %errorlevel% neq 0 (
  echo スケッチ更新処理が異常終了しました。終了コード=%errorlevel%
  taskkill /pid %ProcessId%
  pause
  exit %errorlevel%
)

taskkill /pid %ProcessId%
echo 完了しました。

pause
