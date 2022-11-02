if "%WORKSPACE%" == "" (
    set WORKSPACE_REL=%~dp0\..\..\
    :: Normalize relative path to abs path without ..
    for %%i in ("%WORKSPACE_REL%") do SET "WORKSPACE=%%~fi"
)

:: Build DPCP
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"

set SolutionDir=%WORKSPACE%
msbuild dpcp.sln /t:Build /p:Configuration=Release;Platform=x64
if %errorlevel% neq 0 exit /b %errorlevel%

set GTEST_TAP=2
set /p FILTER=< %WORKSPACE%\.ci\windows\gtest-filter.txt
%WORKSPACE%\bin\user\objWin19H1Release\x64\dpcp-gtest.exe --gtest_filter=%FILTER%
if %errorlevel% neq 0 exit /b %errorlevel%
