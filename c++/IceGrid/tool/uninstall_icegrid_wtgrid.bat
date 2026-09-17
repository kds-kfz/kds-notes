@echo off
REM 将控制台切换为 UTF-8，确保中文提示正常显示。
chcp 65001 >nul
REM 将环境变量限制在当前脚本。
setlocal
REM 设置当前窗口的中文用途标题。
title IceGrid 3.8.2 - 卸载注册中心和节点服务
REM 脚本位于 tool，工作目录切换到上一级 IceGrid 部署根目录。
cd /d "%~dp0.."

REM 请求帮助时跳转到帮助输出，不检查权限或卸载 Windows 服务。
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
REM 设置默认 Registry Windows 服务名。
set "REGISTRY_SERVICE=icegridregistry.MtIceGrid"
REM 设置默认 Node1 Windows 服务名。
set "NODE_SERVICE=icegridnode.MtIceGrid.CloudNetNode1"
REM 设置用于识别 Registry 实例名的基础配置。
set "REGISTRY_CONFIG=config\registry.cfg"
REM 设置用于识别 Node1 名称的基础配置。
set "NODE_CONFIG=config\node.cfg"
REM 初始化统一失败说明。
set "FAIL_MESSAGE="

REM 卸载 Windows 服务必须使用管理员权限。
fltmc >nul 2>&1
REM 当前控制台不是管理员时终止操作。
if errorlevel 1 (
	REM 保存权限错误说明。
	set "FAIL_MESSAGE=卸载 Windows 服务需要管理员权限。"
	REM 跳转统一失败出口。
	goto fail
)

REM 检查 Ice 3.8.2 服务安装器是否存在。
if not exist "bin\iceserviceinstall.exe" (
	REM 保存安装器缺失说明。
	set "FAIL_MESSAGE=缺少 bin\iceserviceinstall.exe。"
	REM 跳转统一失败出口。
	goto fail
)

REM 查询默认 Node1 服务是否存在。
sc query "%NODE_SERVICE%" >nul 2>&1
REM 服务存在时先停止并卸载 Node。
if not errorlevel 1 (
	REM 显示即将卸载的 Node 服务。
	echo [INFO] 停止并卸载 Node 服务 %NODE_SERVICE%...
	REM 使用基础配置中的实例名和 Node 名卸载 Node1。
	bin\iceserviceinstall.exe --nopause --uninstall icegridnode "%NODE_CONFIG%"
	REM 安装器失败时保留 Registry 服务，不继续卸载。
	if errorlevel 1 (
		REM 保存 Node 卸载失败说明。
		set "FAIL_MESSAGE=Node 服务卸载失败。"
		REM 跳转统一失败出口。
		goto fail
	)
) else (
	REM Node 服务不存在时安全跳过。
	echo [INFO] Node 服务未安装，跳过。
)

REM 查询 Registry 服务是否存在。
sc query "%REGISTRY_SERVICE%" >nul 2>&1
REM 服务存在时在 Node 之后停止并卸载 Registry。
if not errorlevel 1 (
	REM 显示即将卸载的 Registry 服务。
	echo [INFO] 停止并卸载 Registry 服务 %REGISTRY_SERVICE%...
	REM 使用基础配置中的实例名卸载 Registry。
	bin\iceserviceinstall.exe --nopause --uninstall icegridregistry "%REGISTRY_CONFIG%"
	REM 安装器失败时进入统一错误出口。
	if errorlevel 1 (
		REM 保存 Registry 卸载失败说明。
		set "FAIL_MESSAGE=Registry 服务卸载失败。"
		REM 跳转统一失败出口。
		goto fail
	)
) else (
	REM Registry 服务不存在时安全跳过。
	echo [INFO] Registry 服务未安装，跳过。
)

REM 输出空行分隔过程日志和成功摘要。
echo.
REM 显示默认 Windows 服务卸载成功。
echo [SUCCESS] IceGrid Windows 服务已卸载。
REM 明确说明数据库和日志不会被删除。
echo [INFO] db 和 log 目录已保留。
REM 用成功错误码结束脚本。
exit /b 0

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
REM 人工运行时暂停，--nopause 调用时跳过。
if "%PAUSE_ON_ERROR%"=="1" pause
REM 把失败码返回给调用方。
exit /b %FAIL_CODE%

REM 参数错误入口。
:invalid_arguments
REM 打印允许的调用方式。
echo [ERROR] 用法：uninstall_icegrid_wtgrid.bat [--nopause]
REM 默认暂停以便双击运行时查看错误。
pause
REM 使用参数错误返回码结束脚本。
exit /b 2

REM 帮助信息输出入口。
:show_help
REM 打印脚本功能。
echo 用途：按 Node 到 Registry 顺序停止并卸载默认 IceGrid Windows 服务。
REM 打印调用格式。
echo 用法：uninstall_icegrid_wtgrid.bat [--nopause]
REM 打印人工调用示例。
echo 示例：uninstall_icegrid_wtgrid.bat
REM 打印自动化调用示例。
echo 示例：uninstall_icegrid_wtgrid.bat --nopause
REM 说明管理员权限要求。
echo 前置条件：必须以管理员身份运行。
REM 说明数据保留策略。
echo 数据策略：只卸载服务，保留 db 和 log；不会删除 Registry 或 Node 数据。
REM 使用成功返回码结束帮助输出。
exit /b 0
