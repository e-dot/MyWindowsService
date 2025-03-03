@REM Windows Service Setup : register service with SC.EXE commands
@ECHO OFF

ECHO [%TIME%] %~n0 : Begin...
SET SERVICE_PATH=%~dp0
SET SERVICE_PASSWORD=%~1
SET SERVICE_LOGIN=%~2
SET SERVICE_NAME=%~3
SET SERVICE_LABEL=%~4
SET SERVICE_LOG_FOLDER=%~5
SET SERVICE_AUTOSTART=%~6
IF "%SERVICE_PASSWORD%" == "help" GOTO USAGE
IF "%SERVICE_PASSWORD%" == "--help" GOTO USAGE

IF NOT "%SERVICE_PASSWORD%" == "" GOTO POST_ASK_PASSWORD
:ASK_PASSWORD
SET "SERVICE_PASSWORD="
SET /P SERVICE_PASSWORD=Service login's password ? 
IF "%SERVICE_PASSWORD%" == "help" GOTO USAGE
IF "%SERVICE_PASSWORD%" == "" GOTO ASK_PASSWORD
:POST_ASK_PASSWORD

IF NOT "%SERVICE_LOGIN%" == "" GOTO POST_ASK_LOGIN
:ASK_LOGIN
SET "SERVICE_LOGIN=%USERDOMAIN%\%USERNAME%"
SET /P SERVICE_LOGIN=Service login, including domain [%SERVICE_LOGIN%]? 
IF "%SERVICE_LOGIN%" == "help" GOTO USAGE
IF "%SERVICE_LOGIN%" == "NULL" GOTO POST_ASK_LOGIN
IF "%SERVICE_LOGIN%" == "" GOTO ASK_LOGIN
:POST_ASK_LOGIN

IF NOT "%SERVICE_NAME%" == "" GOTO POST_ASK_NAME
:ASK_NAME
SET "SERVICE_NAME=MWS"
SET /P SERVICE_NAME=Service name [%SERVICE_NAME%]? 
IF "%SERVICE_NAME%" == "help" GOTO USAGE
IF "%SERVICE_NAME%" == "" GOTO ASK_NAME
:POST_ASK_NAME

IF NOT "%SERVICE_LABEL%" == "" GOTO POST_ASK_LABEL
:ASK_LABEL
SET "SERVICE_LABEL=%SERVICE_NAME%"
SET /P SERVICE_LABEL=Service label [%SERVICE_LABEL%]? 
IF "%SERVICE_LABEL%" == "help" GOTO USAGE
IF "%SERVICE_LABEL%" == "" GOTO ASK_LABEL
:POST_ASK_LABEL

IF NOT "%SERVICE_LOG_FOLDER%" == "" GOTO POST_ASK_LOG_FOLDER
:ASK_LOG_FOLDER
SET "SERVICE_LOG_FOLDER=%SERVICE_PATH%"
SET /P SERVICE_LOG_FOLDER=Service log folder [%SERVICE_LOG_FOLDER%]? 
IF "%SERVICE_LOG_FOLDER%" == "help" GOTO USAGE
IF "%SERVICE_LOG_FOLDER%" == "" GOTO ASK_LOG_FOLDER
:POST_ASK_LOG_FOLDER

SET SERVICE_EXE=%SERVICE_PATH%%SERVICE_NAME%.exe
IF NOT EXIST "%SERVICE_EXE%" GOTO EXE_NOT_FOUND
IF NOT EXIST "%SERVICE_PATH%_start.bat" GOTO START_BAT_NOT_FOUND
IF NOT EXIST "%SERVICE_PATH%_stop.bat" GOTO STOP_BAT_NOT_FOUND

SET SERVICE_CONFIG=%SERVICE_PATH%%SERVICE_NAME%.config
ECHO [%TIME%] %~n0 : Configuration file creation "%SERVICE_CONFIG%"...
TYPE NUL >"%SERVICE_CONFIG%" || GOTO ERROR
ECHO SERVICE_PATH=%SERVICE_PATH% >>"%SERVICE_CONFIG%" || GOTO ERROR
ECHO SERVICE_NAME=%SERVICE_NAME% >>"%SERVICE_CONFIG%" || GOTO ERROR
ECHO SERVICE_LABEL=%SERVICE_LABEL% >>"%SERVICE_CONFIG%" || GOTO ERROR
ECHO SERVICE_LOGIN=%SERVICE_LOGIN% >>"%SERVICE_CONFIG%" || GOTO ERROR
ECHO SERVICE_LOG_FOLDER=%SERVICE_LOG_FOLDER% >>"%SERVICE_CONFIG%" || GOTO ERROR

ECHO [%TIME%] %~n0 : Installing service "%SERVICE_NAME%" ("%SERVICE_LABEL%") to run as %SERVICE_LOGIN% with password ***secret***...
REM Stop service (if running - ignore errors)
SC STOP "%SERVICE_NAME%" 1>NUL 2>NUL
REM Delete service (if installed - ignore errors)
SC DELETE "%SERVICE_NAME%" 1>NUL 2>NUL
REM Create service
SET SERVICE_OPTIONS= type= own obj= "%SERVICE_LOGIN%" password= "%SERVICE_PASSWORD%"
IF "%SERVICE_LOGIN%" == "NULL" SET "SERVICE_OPTIONS="
SC CREATE "%SERVICE_NAME%" binPath= "%SERVICE_EXE%"%SERVICE_OPTIONS% DisplayName= "%SERVICE_LABEL%" start= auto || GOTO ERROR

REM Configure error handling for service : automatically restart twice (then stops)
ECHO [%TIME%] %~n0 : Configuring service "%SERVICE_NAME%" to restart twice on failure...
SC FAILURE "%SERVICE_NAME%" reset= 86400 actions= restart/10000/restart/10000// || GOTO ERROR

REM Pause while service is being created
timeout 5 >nul

IF NOT "%SERVICE_AUTOSTART%" == "AUTOSTART" GOTO POST_AUTOSTART
REM Start service
ECHO [%TIME%] %~n0 : Starting service "%SERVICE_NAME%"...
SC START "%SERVICE_NAME%" || GOTO ERROR
:POST_AUTOSTART

:END
ECHO [%TIME%] %~n0 : End.
EXIT /B 0

:USAGE
ECHO [%TIME%] Usage : %~nx0 SERVICE_PASSWORD [SERVICE_LOGIN=%USERNAME%] [SERVICE_NAME=.\*.exe] [SERVICE_LABEL=SERVICE_NAME] >&2
EXIT /B 0

:ERROR
ECHO [%TIME%] %~n0 : ERROR! >&2
EXIT /B 1

:EXE_NOT_FOUND
ECHO [%TIME%] %~n0 : ERROR : executable file not found: "%SERVICE_EXE%" >&2
EXIT /B 2

:START_BAT_NOT_FOUND
ECHO [%TIME%] %~n0 : ERROR : start command not found: "%SERVICE_PATH%_start.bat" >&2
EXIT /B 3

:STOP_BAT_NOT_FOUND
ECHO [%TIME%] %~n0 : ERROR : stop command not found: "%SERVICE_PATH%_stop.bat" >&2
EXIT /B 4
