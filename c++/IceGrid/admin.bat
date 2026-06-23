@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"
bin\icegridadmin.exe --Ice.Config=config\admin.cfg
endlocal
