@echo off

rem Ping��ł�������(mDNS�̕s���h�~�̂���)
for /f "tokens=1,2 delims==;	 " %%i in ('wmic process call create "ping lmap2024.local -t"') do (
  if "%%i" equ "ProcessId" set ProcessId=%%j
)

rem SPIFFS����������
echo SPIFFS���X�V���Ă��܂��B
cd %~dp0
%HOMEPATH%\.platformio\penv\Scripts\platformio.exe run -t uploadfs
if %errorlevel% neq 0 (
  echo SPIFFS�X�V�������ُ�I�����܂����B�I���R�[�h=%errorlevel%
  taskkill /pid %ProcessId%
  pause
  exit %errorlevel%
)
ping localhost -n 20
rem �X�P�b�`����������
echo �X�P�b�`���X�V���Ă��܂��B
%HOMEPATH%\.platformio\penv\Scripts\platformio.exe run -t upload
if %errorlevel% neq 0 (
  echo �X�P�b�`�X�V�������ُ�I�����܂����B�I���R�[�h=%errorlevel%
  taskkill /pid %ProcessId%
  pause
  exit %errorlevel%
)

taskkill /pid %ProcessId%
echo �������܂����B

pause
