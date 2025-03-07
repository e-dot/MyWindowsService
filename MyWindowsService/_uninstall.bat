@REM Windows Service Setup : register service with SC.EXE commands
@ECHO OFF

ECHO [%TIME%] %~n0 : Begin...
SET SERVICE_PATH=%~dp0
SET SERVICE_NAME=%~1
IF "%SERVICE_NAME%" == "help" GOTO USAGE
IF "%SERVICE_NAME%" == "--help" GOTO USAGE

IF NOT "%SERVICE_NAME%" == "" GOTO POST_ASK_NAME
:ASK_NAME
SET "SERVICE_NAME="
SET /P SERVICE_NAME=Service name ? 
IF "%SERVICE_NAME%" == "help" GOTO USAGE
IF "%SERVICE_NAME%" == "" GOTO ASK_NAME
:POST_ASK_NAME

ECHO [%TIME%] %~n0 : Uninstalling service "%SERVICE_NAME%"...
REM Stop service (if running - ignore errors)
SC STOP "%SERVICE_NAME%" 1>NUL 2>NUL
REM Pause while service is being stopped
timeout 5 >nul
REM Delete service
SC DELETE "%SERVICE_NAME%" || GOTO ERROR

REM Send event "Service unregistered" into the Windows Application Event Log (Note that since we delete registry entries below, full message mapping won't work, but message is still readable)
EVENTCREATE /T SUCCESS /L APPLICATION /ID 201 /D "Service %SERVICE_NAME% (%SERVICE_LABEL%) uninstalled successfully."

REM Delete registry entries for event message files (SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\[SERVICE_NAME]\\EventMessageFile = "%SERVICE_EXE%") - ignore errors
ECHO [%TIME%] %~n0 : Deleting registry entries for event messages...
REG.EXE DELETE "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\EventLog\Application\%SERVICE_NAME%" /f

:END
ECHO [%TIME%] %~n0 : End.
EXIT /B 0

:USAGE
ECHO [%TIME%] Usage : %~nx0 SERVICE_NAME >&2
EXIT /B 0

:ERROR
ECHO [%TIME%] %~n0 : ERROR! >&2
EXIT /B 1
