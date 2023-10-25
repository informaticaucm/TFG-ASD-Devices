@REM ::::::::::::::::::::::::::::::::::::::::::::
@REM :: Automatically check & get admin rights V2
@REM ::::::::::::::::::::::::::::::::::::::::::::
@REM @echo off
@REM CLS
@REM ECHO.
@REM ECHO =============================
@REM ECHO Running Admin shell
@REM ECHO =============================

@REM :init
@REM setlocal DisableDelayedExpansion
@REM set "batchPath=%~0"
@REM for %%k in (%0) do set batchName=%%~nk
@REM set "vbsGetPrivileges=%temp%\OEgetPriv_%batchName%.vbs"
@REM setlocal EnableDelayedExpansion

@REM :checkPrivileges
@REM NET FILE 1>NUL 2>NUL
@REM if '%errorlevel%' == '0' ( goto gotPrivileges ) else ( goto getPrivileges )

@REM :getPrivileges
@REM if '%1'=='ELEV' (echo ELEV & shift /1 & goto gotPrivileges)
@REM ECHO.
@REM ECHO **************************************
@REM ECHO Invoking UAC for Privilege Escalation
@REM ECHO **************************************

@REM ECHO Set UAC = CreateObject^("Shell.Application"^) > "%vbsGetPrivileges%"
@REM ECHO args = "ELEV " >> "%vbsGetPrivileges%"
@REM ECHO For Each strArg in WScript.Arguments >> "%vbsGetPrivileges%"
@REM ECHO args = args ^& strArg ^& " "  >> "%vbsGetPrivileges%"
@REM ECHO Next >> "%vbsGetPrivileges%"
@REM ECHO UAC.ShellExecute "!batchPath!", args, "", "runas", 1 >> "%vbsGetPrivileges%"
@REM "%SystemRoot%\System32\WScript.exe" "%vbsGetPrivileges%" %*
@REM exit /B

@REM :gotPrivileges
@REM setlocal & pushd .
@REM cd /d %~dp0
@REM if '%1'=='ELEV' (del "%vbsGetPrivileges%" 1>nul 2>nul  &  shift /1)

@REM ::::::::::::::::::::::::::::
@REM ::START
@REM ::::::::::::::::::::::::::::
@REM net stop HNS
@REM net stop SharedAccess

CMD /C npx kill-port 53
CMD /C npx kill-port 443

cd HTTPS
start cmd /k node https_server.js
cd ../DNS
start cmd /k node dns_server.js
cd ../THINGSBOARD
start cmd /k docker compose up
cd ..

@REM net start HNS
@REM net start SharedAccess