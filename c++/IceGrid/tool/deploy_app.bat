@echo off
REM 将控制台切换为 UTF-8，确保中文提示正常显示。
chcp 65001 >nul
REM 将环境变量限制在当前脚本，避免影响调用它的控制台。
setlocal
REM 设置当前窗口的中文用途标题。
title IceGrid 3.8.2 - 部署或更新应用
REM 脚本位于 tool，工作目录切换到上一级 IceGrid 部署根目录。
cd /d "%~dp0.."

REM 请求帮助时跳转到帮助输出，不连接 Registry 或部署应用。
if /i "%~1"=="--help" goto show_help
REM 兼容 Windows 常用的 /? 帮助参数。
if /i "%~1"=="/?" goto show_help
REM 兼容简写的 -h 帮助参数。
if /i "%~1"=="-h" goto show_help
REM 默认失败时暂停，方便人工双击查看错误。
set "PAUSE_ON_ERROR=1"
REM 自动化调用传入 --nopause 时不等待按键。
if /i "%~1"=="--nopause" set "PAUSE_ON_ERROR=0"
REM 除帮助和 --nopause 外不接受其他参数。
if not "%~1"=="" if /i not "%~1"=="--nopause" goto invalid_arguments
REM 设置 Registry 中固定的应用名称。
set "APP_NAME=CloudNetDataApiGrid"
REM 设置相对部署根目录的应用描述文件。
set "APP_FILE=app\MtApplication.xml"
REM 为 application list 创建本次运行独立的临时输出文件。
set "LIST_OUTPUT=%TEMP%\icegrid_application_list_%RANDOM%.txt"
REM 初始化供统一失败出口显示的错误说明。
set "FAIL_MESSAGE="

REM 部署前检查 IceGrid 管理工具是否完整。
if not exist "bin\icegridadmin.exe" (
	REM 记录缺失管理工具的明确错误。
	set "FAIL_MESSAGE=缺少 bin\icegridadmin.exe。"
	REM 跳转统一失败出口。
	goto fail
)
REM 部署前检查 MtApplication.xml 是否存在。
if not exist "%APP_FILE%" (
	REM 记录缺失应用描述文件的明确错误。
	set "FAIL_MESSAGE=缺少应用描述文件：%APP_FILE%"
	REM 跳转统一失败出口。
	goto fail
)

REM 先查询应用列表；该命令同时作为 Registry 连接健康检查。
bin\icegridadmin.exe --Ice.Config=config\admin.cfg -e "application list" >"%LIST_OUTPUT%" 2>&1
REM 查询失败时禁止继续执行 application add/update。
if errorlevel 1 goto list_failed

REM 在应用列表中精确查找目标应用名称。
findstr /i /c:"%APP_NAME%" "%LIST_OUTPUT%" >nul
REM 应用不存在时进入首次添加分支。
if errorlevel 1 goto add_application
REM 应用已存在时进入更新分支。
goto update_application

REM 首次部署应用的处理入口。
:add_application
REM application list 已成功，删除不再需要的临时输出。
del /q "%LIST_OUTPUT%" >nul 2>&1
REM 向控制台说明即将执行 add。
echo [INFO] 首次部署应用 %APP_NAME%，执行 application add...
REM 将 MtApplication.xml 添加到 Registry。
bin\icegridadmin.exe --Ice.Config=config\admin.cfg -e "application add %APP_FILE%"
REM add 成功时打印结果并返回 0。
if not errorlevel 1 (
	REM 显示首次添加成功。
	echo [SUCCESS] 应用 %APP_NAME% 已添加。
	REM 用成功错误码结束脚本。
	exit /b 0
)
REM 保存 add 失败的业务说明。
set "FAIL_MESSAGE=application add 执行失败。"
REM 跳转统一失败出口。
goto fail

REM 更新已有应用的处理入口。
:update_application
REM application list 已成功，删除不再需要的临时输出。
del /q "%LIST_OUTPUT%" >nul 2>&1
REM 向控制台说明即将执行 update。
echo [INFO] 应用 %APP_NAME% 已存在，执行 application update...
REM 用 MtApplication.xml 更新 Registry 中的现有应用。
bin\icegridadmin.exe --Ice.Config=config\admin.cfg -e "application update %APP_FILE%"
REM update 成功时打印结果并返回 0。
if not errorlevel 1 (
	REM 显示应用更新成功。
	echo [SUCCESS] 应用 %APP_NAME% 已更新。
	REM 用成功错误码结束脚本。
	exit /b 0
)
REM 保存 update 失败的业务说明。
set "FAIL_MESSAGE=application update 执行失败。"
REM 跳转统一失败出口。
goto fail

REM Registry 连接或 application list 查询失败的处理入口。
:list_failed
REM 第一时间保存 icegridadmin 返回码，避免被后续命令覆盖。
set "FAIL_CODE=%errorlevel%"
REM 原样显示管理工具返回的诊断信息。
type "%LIST_OUTPUT%"
REM 清理本次查询产生的临时文件。
del /q "%LIST_OUTPUT%" >nul 2>&1
REM 记录 Registry 不可用的明确说明。
set "FAIL_MESSAGE=无法连接 Registry 或读取 application list。"
REM 使用已保存的原始返回码进入失败出口。
goto fail_with_code

REM 普通失败入口，捕获上一条命令的返回码。
:fail
REM 保存失败命令的返回码。
set "FAIL_CODE=%errorlevel%"
REM 部分 goto 场景没有系统错误码时统一返回 1。
if "%FAIL_CODE%"=="0" set "FAIL_CODE=1"

REM 已有明确 FAIL_CODE 时使用的统一失败出口。
:fail_with_code
REM 输出空行分隔正常日志与错误摘要。
echo.
REM 显示具体失败原因。
echo [ERROR] %FAIL_MESSAGE%
REM 显示脚本最终返回码，便于自动化判断。
echo [INFO] 错误码：%FAIL_CODE%
REM 显示下一步需要检查的服务、XML 和 Registry 日志。
echo [INFO] 请检查 Registry、Node 服务、%APP_FILE% 和 %CD%\log\registry_stderr.txt。
REM 人工运行时暂停，--nopause 调用时跳过。
if "%PAUSE_ON_ERROR%"=="1" pause
REM 把失败码返回给调用方。
exit /b %FAIL_CODE%

REM 参数错误入口。
:invalid_arguments
REM 打印允许的调用方式。
echo [ERROR] 用法：deploy_app.bat [--nopause]
REM 默认暂停以便双击运行时查看错误。
pause
REM 使用参数错误返回码结束脚本。
exit /b 2

REM 帮助信息输出入口。
:show_help
REM 打印脚本功能。
echo 用途：部署 app\MtApplication.xml；应用不存在时 add，已存在时 update。
REM 打印调用格式。
echo 用法：deploy_app.bat [--nopause]
REM 打印人工调用示例。
echo 示例：deploy_app.bat
REM 打印自动化调用示例。
echo 示例：deploy_app.bat --nopause
REM 说明执行前置条件。
echo 前置条件：Registry 和目标 Node 已启动，config\admin.cfg 可连接 Registry。
REM 说明应用名称。
echo 应用名称：CloudNetDataApiGrid。
REM 使用成功返回码结束帮助输出。
exit /b 0
