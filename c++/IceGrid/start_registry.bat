@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"
if not exist db\registry mkdir db\registry
if not exist log mkdir log
bin\icegridregistry.exe --Ice.Config=config\registry.cfg
endlocal
