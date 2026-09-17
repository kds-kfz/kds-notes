@echo off
REM 将控制台切换为 UTF-8，确保中文提示正常显示。
chcp 65001 >nul
REM 将环境变量限制在当前脚本。
setlocal
REM 设置当前窗口的中文用途标题。
title IceGrid 3.8.2 - 启动默认节点服务
REM 脚本位于 tool，工作目录切换到上一级 IceGrid 部署根目录。
cd /d "%~dp0.."

REM 请求帮助时跳转到帮助输出，不查询或启动 Windows 服务。
if /i "%~1"=="--help" goto show_help
REM 兼容 Windows 常用的 /? 帮助参数。
if /i "%~1"=="/?" goto show_help
REM 兼容简写的 -h 帮助参数。
if /i "%~1"=="-h" goto show_help
REM 默认失败时暂停，方便人工查看错误。
set "PAUSE_ON_ERROR=1"
REM 自动化调用传入 --nopause 时不等待按键。
if /i "%~1"=="--nopause" set "PAUSE_ON_ERROR=0"
REM 除帮助和 --nopause 外不接受其他参数。
if not "%~1"=="" if /i not "%~1"=="--nopause" goto invalid_arguments
REM 设置默认 Node1 Windows 服务名。
set "SERVICE_NAME=icegridnode.MtIceGrid.CloudNetNode1"
REM 设置健康检查必须发现的 Node 名。
set "NODE_NAME=CloudNetNode1"
REM 初始化统一失败说明。
set "FAIL_MESSAGE="

REM 检查 Node1 Windows 服务是否已经安装。
sc query "%SERVICE_NAME%" >nul 2>&1
REM 服务不存在时提示先执行总安装脚本。
if errorlevel 1 (
	REM 保存服务未安装说明。
	set "FAIL_MESSAGE=Node 服务尚未安装，请先运行 tool\install_icegrid_wtgrid.bat。"
	REM 跳转统一失败出口。
	goto fail
)

REM 检查服务是否已经处于 Running。
call :is_running
REM 已运行时跳过启动命令，直接检查 Node 注册状态。
if not errorlevel 1 goto health_check

REM 启动停止状态的 Windows 服务需要管理员权限。
fltmc >nul 2>&1
REM 当前控制台不是管理员时终止操作。
if errorlevel 1 (
	REM 保存权限错误说明。
	set "FAIL_MESSAGE=启动 Windows 服务需要管理员权限。"
	REM 跳转统一失败出口。
	goto fail
)

REM 显示即将启动的服务名。
echo [INFO] 启动 Node 服务 %SERVICE_NAME%...
REM 请求 Windows 服务控制管理器启动 Node1。
sc start "%SERVICE_NAME%"
REM sc start 失败后再次查询，兼容服务已被其他操作启动的竞态。
if errorlevel 1 (
	REM 再次检查最终运行状态。
	call :is_running
	REM 服务仍未运行时记录启动失败。
	if errorlevel 1 (
		REM 保存启动命令失败说明。
		set "FAIL_MESSAGE=Node 服务启动命令失败。"
		REM 跳转统一失败出口。
		goto fail
	)
)

REM 最多等待 30 秒让服务进入 Running。
call :wait_running
REM 超时未运行时记录错误。
if errorlevel 1 (
	REM 保存服务状态超时说明。
	set "FAIL_MESSAGE=Node 服务未在 30 秒内进入 Running 状态。"
	REM 跳转统一失败出口。
	goto fail
)

REM Node 服务已运行后的 Registry 注册健康检查入口。
:health_check
REM 最多等待 30 秒确认 Node1 出现在 node list。
call :wait_node
REM 健康检查失败时进入统一错误出口。
if errorlevel 1 goto fail
REM 显示 Node 后台运行和注册成功。
echo [SUCCESS] Node 服务已在后台运行，%NODE_NAME% 已注册。
REM 用成功错误码结束脚本。
exit /b 0

REM 判断 Node1 服务是否处于 Running 的子程序。
:is_running
REM Windows STATE 代码 4 表示 Running。
sc query "%SERVICE_NAME%" 2>nul | findstr /r /c:"STATE *: *4" >nul
REM 把 findstr 结果返回给调用方。
exit /b %errorlevel%

REM 等待 Windows 服务进入 Running 的子程序。
:wait_running
REM 每秒检查一次，最多检查 30 次。
for /l %%I in (1,1,30) do (
	REM 查询当前服务状态。
	call :is_running
	REM 一旦进入 Running 立即成功返回。
	if not errorlevel 1 exit /b 0
	REM 未运行时等待 1 秒再检查。
	timeout /t 1 /nobreak >nul
)
REM 30 秒后仍未运行则返回失败。
exit /b 1

REM 等待 Node1 注册到 Registry 的子程序。
:wait_node
REM 创建本次健康检查独立的临时输出路径。
set "CHECK_OUTPUT=%TEMP%\icegrid_node_check_%RANDOM%.txt"
REM 每秒执行一次 node list，最多检查 30 次。
for /l %%I in (1,1,30) do (
	REM 从 Registry 读取当前 Node 列表。
	bin\icegridadmin.exe --Ice.Config=config\admin.cfg -e "node list" >"%CHECK_OUTPUT%" 2>&1
	REM Registry 查询成功后继续检查目标 Node 名。
	if not errorlevel 1 (
		REM 在列表中查找 CloudNetNode1。
		findstr /i /c:"%NODE_NAME%" "%CHECK_OUTPUT%" >nul
		REM 找到 Node 时清理临时文件并返回成功。
		if not errorlevel 1 (
			REM 删除健康检查临时输出。
			del /q "%CHECK_OUTPUT%" >nul 2>&1
			REM 返回 Node 注册成功。
			exit /b 0
		)
	)
	REM Node 尚未注册时等待 1 秒。
	timeout /t 1 /nobreak >nul
)
REM 超时后显示 icegridadmin 最后一次诊断输出。
type "%CHECK_OUTPUT%"
REM 删除健康检查临时输出。
del /q "%CHECK_OUTPUT%" >nul 2>&1
REM 保存 Node 注册超时说明。
set "FAIL_MESSAGE=Node 已启动，但 %NODE_NAME% 未在 30 秒内注册。"
REM 返回健康检查失败。
exit /b 1

REM 所有错误路径共用的失败出口。
:fail
REM 保存上一条失败命令的错误码。
set "FAIL_CODE=%errorlevel%"
REM 没有系统错误码时统一返回 1。
if "%FAIL_CODE%"=="0" set "FAIL_CODE=1"
REM 输出空行分隔正常日志和错误摘要。
echo.
REM 显示具体失败原因。
echo [ERROR] %FAIL_MESSAGE%
REM 显示 Node 错误日志位置。
echo [INFO] 错误日志：%CD%\log\node_stderr.txt
REM 人工运行时暂停，--nopause 调用时跳过。
if "%PAUSE_ON_ERROR%"=="1" pause
REM 把失败码返回给调用方。
exit /b %FAIL_CODE%

REM 参数错误入口。
:invalid_arguments
REM 打印允许的调用方式。
echo [ERROR] 用法：start_node.bat [--nopause]
REM 默认暂停以便双击运行时查看错误。
pause
REM 使用参数错误返回码结束脚本。
exit /b 2

REM 帮助信息输出入口。
:show_help
REM 打印脚本功能。
echo 用途：启动已安装的 CloudNetNode1 Windows 服务并确认节点注册成功。
REM 打印调用格式。
echo 用法：start_node.bat [--nopause]
REM 打印人工调用示例。
echo 示例：start_node.bat
REM 打印自动化调用示例。
echo 示例：start_node.bat --nopause
REM 说明启动顺序。
echo 前置条件：Registry 已启动且健康，Node 服务已由总安装脚本安装。
REM 说明管理员权限规则。
echo 权限：服务已运行时只做健康检查；需要启动停止状态服务时必须使用管理员权限。
REM 使用成功返回码结束帮助输出。
exit /b 0
