@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"
if not exist db\node1 mkdir db\node1
if not exist log mkdir log
bin\icegridnode.exe --Ice.Config=config\node.cfg
endlocal
