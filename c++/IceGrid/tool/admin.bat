@echo off
REM 将控制台切换为 UTF-8，确保中文提示正常显示。
chcp 65001 >nul
REM 将环境变量限制在当前脚本，避免影响外部控制台。
setlocal
REM 设置当前窗口的中文用途标题。
title IceGrid 3.8.2 - 管理控制台
REM 脚本位于 tool，工作目录切换到上一级 IceGrid 部署根目录。
cd /d "%~dp0.."

REM 请求帮助时跳转到帮助输出，不连接 Registry。
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

REM 使用 admin.cfg 中的 Locator 和登录信息进入交互式管理控制台。
bin\icegridadmin.exe --Ice.Config=config\admin.cfg
REM 管理控制台正常退出时直接返回成功。
if not errorlevel 1 exit /b 0

REM 保存 icegridadmin 的原始错误码。
set "FAIL_CODE=%errorlevel%"
REM 输出空行分隔管理工具输出与错误摘要。
echo.
REM 显示连接失败和错误码。
echo [ERROR] IceGrid 管理控制台连接失败，错误码：%FAIL_CODE%
REM 提示检查固定的 Registry Windows 服务。
echo [INFO] 请确认 Registry 服务 icegridregistry.MtIceGrid 正在运行。
REM 显示 Registry 错误日志位置。
echo [INFO] Registry 日志：%CD%\log\registry_stderr.txt
REM 人工运行时暂停，--nopause 调用时跳过。
if "%PAUSE_ON_ERROR%"=="1" pause
REM 把管理工具错误码返回给调用方。
exit /b %FAIL_CODE%

REM 参数错误入口。
:invalid_arguments
REM 打印允许的调用方式。
echo [ERROR] 用法：admin.bat [--nopause]
REM 默认暂停以便双击运行时查看错误。
pause
REM 使用参数错误返回码结束脚本。
exit /b 2

REM 帮助信息输出入口。
:show_help
REM 打印脚本功能。
echo 用途：连接 MtIceGrid Registry 并进入交互式 icegridadmin 管理控制台。
REM 打印调用格式。
echo 用法：admin.bat [--nopause]
REM 打印常用示例。
echo 示例：admin.bat
REM 打印进入控制台后的常用命令。
echo 常用命令：node list、application list、adapter list、server list、exit。
REM 说明配置来源。
echo 配置文件：config\admin.cfg。
REM 使用成功返回码结束帮助输出。
exit /b 0
