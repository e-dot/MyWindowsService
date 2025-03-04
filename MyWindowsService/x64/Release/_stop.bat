@REM Windows Service Simulation : stop running MWS_start.bat batch

@REM Call Powershell script to kill processes cmd.exe with parameter with same name (MWS_start.bat)
SET START_BATCH_NAME=%~dpn0
SET START_BATCH_NAME=%START_BATCH_NAME:_stop=_start%
POWERSHELL.EXE -InputFormat none -File %~dpn0.ps1 -BatchName "%START_BATCH_NAME%.bat"
