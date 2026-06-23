@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"
REM 导入或更新 CloudNetDataApi IceGrid 应用模板。
REM 需要先启动 start_registry.bat。
bin\icegridadmin.exe --Ice.Config=config\admin.cfg -e "application update app\CloudNetDataApi.icegrid.xml"
endlocal
