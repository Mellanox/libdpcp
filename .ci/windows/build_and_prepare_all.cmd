if "%WORKSPACE%" == "" (
    set WORKSPACE_REL=%~dp0\..\..\
    :: Normalize relative path to abs path without ..
    for %%i in ("%WORKSPACE_REL%") do SET "WORKSPACE=%%~fi"
)

:: map M:, N:
call %WORKSPACE%\.ci\windows\map_network_drives.cmd

:: Map EWDK .iso as DVD ROM.
:: Below command keeps output of command 'PowerShell . %~dp0\map_ewdk_iso.ps1' to variable EWDK_DRIVE
:: No spaces in round brackets (SET EWDK_DRIVE=%%A) are important because trailing spaces may be added to the variable value!
FOR /F "tokens=*" %%A in ('PowerShell . %WORKSPACE%\.ci\windows\map_ewdk_iso.ps1') do (SET EWDK_DRIVE=%%A)

:: Build Rivermax binaries
call %EWDK_DRIVE:~0,1%:\BuildEnv\SetupBuildEnv.cmd
set SolutionDir=%WORKSPACE%
msbuild dpcp.sln /t:Build /p:Configuration=Release;Platform=x64
if %errorlevel% neq 0 exit /b %errorlevel%
