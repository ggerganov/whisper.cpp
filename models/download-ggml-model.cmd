@echo off

pushd %~dp0
set models_path=%CD%
for %%d in (%~dp0..) do set root_path=%%~fd
popd

set argc=0
for %%x in (%*) do set /A argc+=1

set models=tiny.en tiny base.en base small.en small medium.en medium large-v1 large

if %argc% neq 1 (
  echo.
  echo Usage: download-ggml-model.cmd model
  CALL :list_models
  goto :eof
)

set model=%1

for %%b in (%models%) do (
  if "%%b"=="%model%" (
    CALL :download_model
    goto :eof
  )
)

echo Invalid model: %model%
CALL :list_models
goto :eof

:download_model
echo Downloading ggml model %model%...

cd %models_path%

if exist "ggml-%model%.bin" (
  echo Model %model% already exists. Skipping download.
  goto :eof
)

PowerShell -NoProfile -ExecutionPolicy Bypass -Command "Invoke-WebRequest -Uri https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-%model%.bin -OutFile ggml-%model%.bin"

if %ERRORLEVEL% neq 0 (
  echo Failed to download ggml model %model%
  echo Please try again later or download the original Whisper model files and convert them yourself.
  goto :eof
)

echo Done! Model %model% saved in %root_path%\models\ggml-%model%.bin
echo You can now use it like this:
echo main.exe -m %root_path%\models\ggml-%model%.bin -f %root_path%\samples\jfk.wav

goto :eof

:list_models
  echo.
  echo Available models:
  (for %%a in (%models%) do ( 
    echo %%a 
  ))
  echo.
  exit /b
