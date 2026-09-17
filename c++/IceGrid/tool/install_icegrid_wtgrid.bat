@echo off
REM 将控制台切换为 UTF-8，确保中文提示正常显示。
chcp 65001 >nul
REM 将环境变量限制在当前脚本。
setlocal
REM 设置当前窗口的中文用途标题。
title IceGrid 3.8.2 - 安装并启动注册中心和节点服务
REM 脚本位于 tool，工作目录切换到上一级 IceGrid 部署根目录。
cd /d "%~dp0.."

REM 请求帮助时跳转到帮助输出，不检查权限或修改 Windows 服务。
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
REM 设置安装过程中生成的 Registry 绝对路径配置。
set "REGISTRY_CONFIG=config\service\registry.service.cfg"
REM 设置安装过程中生成的 Node1 绝对路径配置。
set "NODE_CONFIG=config\service\node.service.cfg"
REM 标记 Registry 是否由本次运行新建，失败回滚时使用。
set "CREATED_REGISTRY=0"
REM 标记 Node 是否由本次运行新建，失败回滚时使用。
set "CREATED_NODE=0"
REM 初始化统一失败说明。
set "FAIL_MESSAGE="

REM 检查当前控制台是否具有管理员权限。
call :check_admin
REM 非管理员运行时终止安装。
if errorlevel 1 (
	REM 保存权限错误说明。
	set "FAIL_MESSAGE=请右键选择“以管理员身份运行”后重试。"
	REM 跳转统一失败出口。
	goto fail
)

REM 检查 Windows 服务安装器。
call :check_file "bin\iceserviceinstall.exe"
REM 缺少安装器时终止。
if errorlevel 1 goto fail
REM 检查 Registry 主程序。
call :check_file "bin\icegridregistry.exe"
REM 缺少 Registry 时终止。
if errorlevel 1 goto fail
REM 检查 Node 主程序。
call :check_file "bin\icegridnode.exe"
REM 缺少 Node 时终止。
if errorlevel 1 goto fail
REM 检查管理客户端。
call :check_file "bin\icegridadmin.exe"
REM 缺少管理客户端时终止。
if errorlevel 1 goto fail
REM 检查 Registry 基础配置。
call :check_file "config\registry.cfg"
REM 缺少 Registry 配置时终止。
if errorlevel 1 goto fail
REM 检查 Node 基础配置。
call :check_file "config\node.cfg"
REM 缺少 Node 配置时终止。
if errorlevel 1 goto fail
REM 检查 Admin 基础配置。
call :check_file "config\admin.cfg"
REM 缺少 Admin 配置时终止。
if errorlevel 1 goto fail

REM 校验 Registry 必须是 Ice 3.8.2。
call :check_ice_version "bin\icegridregistry.exe"
REM Registry 版本不符时终止。
if errorlevel 1 goto fail
REM 校验 Node 必须是 Ice 3.8.2。
call :check_ice_version "bin\icegridnode.exe"
REM Node 版本不符时终止。
if errorlevel 1 goto fail
REM 校验管理客户端必须是 Ice 3.8.2。
call :check_ice_version "bin\icegridadmin.exe"
REM 管理客户端版本不符时终止。
if errorlevel 1 goto fail

REM 检查部署路径是否包含批处理不支持的特殊字符。
call :check_root_path
REM 路径不兼容时终止。
if errorlevel 1 goto fail

REM 创建数据库、日志和生成配置目录。
call :prepare_directories
REM 任一目录创建失败时终止。
if errorlevel 1 goto fail
REM 从基础配置生成服务使用的绝对路径配置。
call :generate_service_configs
REM 配置生成失败时终止。
if errorlevel 1 goto fail
REM 为 LocalService 授予数据库和日志目录修改权限。
call :grant_runtime_permissions
REM 任一目录授权失败时终止。
if errorlevel 1 goto fail

REM 显示 Registry 服务检查阶段。
echo [INFO] 检查 Registry 服务...
REM 查询 Registry 服务是否已经安装。
call :service_exists "%REGISTRY_SERVICE%"
REM 服务不存在时执行首次安装。
if errorlevel 1 (
	REM 显示即将安装的 Registry 服务名。
	echo [INFO] 安装 Registry 服务 %REGISTRY_SERVICE%...
	REM 安装 LocalService 自动启动的 Registry Windows 服务。
	bin\iceserviceinstall.exe --nopause --AutoStart 1 --DisplayName "MtIceGrid Registry" icegridregistry "%REGISTRY_CONFIG%"
	REM 安装器失败时进入统一错误出口。
	if errorlevel 1 (
		REM 保存 Registry 安装失败说明。
		set "FAIL_MESSAGE=Registry 服务安装失败。"
		REM 跳转统一失败出口。
		goto fail
	)
	REM 标记 Registry 为本次新建，后续失败时允许回滚。
	set "CREATED_REGISTRY=1"
) else (
	REM 已有服务必须指向当前部署目录、自动启动且使用 LocalService。
	call :validate_service "%REGISTRY_SERVICE%" "%CD%\%REGISTRY_CONFIG%" ""
	REM 现有服务配置不匹配时终止且不覆盖。
	if errorlevel 1 goto fail
	REM 显示现有 Registry 服务可安全复用。
	echo [INFO] Registry 服务已正确安装。
)

REM 启动或复用已运行的 Registry 服务。
call :start_service "%REGISTRY_SERVICE%" "Registry"
REM Registry 启动失败时进入统一错误出口。
if errorlevel 1 goto fail
REM 等待 Registry 管理接口健康。
call :wait_registry
REM Registry 健康检查失败时进入统一错误出口。
if errorlevel 1 goto fail

REM 显示 Node 服务检查阶段。
echo [INFO] 检查 Node 服务...
REM 查询默认 Node1 服务是否已经安装。
call :service_exists "%NODE_SERVICE%"
REM 服务不存在时执行首次安装。
if errorlevel 1 (
	REM 显示即将安装的 Node 服务名。
	echo [INFO] 安装 Node 服务 %NODE_SERVICE%...
	REM 安装 LocalService 自动启动的 Node，并建立对本机 Registry 的服务依赖。
	bin\iceserviceinstall.exe --nopause --AutoStart 1 --DependOnRegistry 1 --DisplayName "MtIceGrid CloudNetNode1" icegridnode "%NODE_CONFIG%"
	REM 安装器失败时进入统一错误出口。
	if errorlevel 1 (
		REM 保存 Node 安装失败说明。
		set "FAIL_MESSAGE=Node 服务安装失败。"
		REM 跳转统一失败出口。
		goto fail
	)
	REM 标记 Node 为本次新建，后续失败时允许回滚。
	set "CREATED_NODE=1"
) else (
	REM 已有 Node 必须指向当前目录、自动启动、使用 LocalService 并依赖 Registry。
	call :validate_service "%NODE_SERVICE%" "%CD%\%NODE_CONFIG%" "%REGISTRY_SERVICE%"
	REM 现有服务配置不匹配时终止且不覆盖。
	if errorlevel 1 goto fail
	REM 显示现有 Node 服务可安全复用。
	echo [INFO] Node 服务已正确安装。
)

REM 启动或复用已运行的 Node 服务。
call :start_service "%NODE_SERVICE%" "Node"
REM Node 启动失败时进入统一错误出口。
if errorlevel 1 goto fail
REM 等待 CloudNetNode1 注册到 Registry。
call :wait_node
REM Node 健康检查失败时进入统一错误出口。
if errorlevel 1 goto fail

REM 输出空行分隔过程日志和成功摘要。
echo.
REM 显示服务安装和启动成功。
echo [SUCCESS] IceGrid 3.8.2 服务已安装并启动。
REM 显示 Registry 服务名。
echo [INFO] Registry: %REGISTRY_SERVICE%
REM 显示 Node 服务名。
echo [INFO] Node:     %NODE_SERVICE%
REM 显示数据库根目录。
echo [INFO] 数据目录: %CD%\db
REM 显示日志目录。
echo [INFO] 日志目录: %CD%\log
REM 用成功错误码结束脚本。
exit /b 0

REM 管理员权限检查子程序。
:check_admin
REM fltmc 只有管理员控制台可以成功执行。
fltmc >nul 2>&1
REM 返回 fltmc 的权限检查结果。
exit /b %errorlevel%

REM 必需文件检查子程序，参数 1 是相对部署根目录的路径。
:check_file
REM 文件存在时直接返回成功。
if exist "%~1" exit /b 0
REM 文件缺失时保存具体路径。
set "FAIL_MESSAGE=缺少必需文件：%~1"
REM 返回文件缺失错误。
exit /b 1

REM Ice 版本检查子程序，参数 1 是待检查 exe。
:check_ice_version
REM 清空上一次版本检查结果。
set "ICE_VERSION="
REM 执行 --version 并保存最后一行输出。
for /f "delims=" %%V in ('%~1 --version 2^>^&1') do set "ICE_VERSION=%%V"
REM 版本精确等于 3.8.2 时返回成功。
if "%ICE_VERSION%"=="3.8.2" exit /b 0
REM 保存期望版本和实际版本。
set "FAIL_MESSAGE=%~1 版本错误，期望 3.8.2，实际为 %ICE_VERSION%。"
REM 返回版本不匹配错误。
exit /b 1

REM 部署根目录字符检查子程序。
:check_root_path
REM 保存当前 IceGrid 部署根目录。
set "ROOT_PATH=%CD%"
REM 路径包含 & 时进入不支持分支。
if not "%ROOT_PATH:&=%"=="%ROOT_PATH%" goto unsupported_root
REM 路径包含竖线时进入不支持分支。
if not "%ROOT_PATH:|=%"=="%ROOT_PATH%" goto unsupported_root
REM 路径包含小于号时进入不支持分支。
if not "%ROOT_PATH:<=%"=="%ROOT_PATH%" goto unsupported_root
REM 路径包含大于号时进入不支持分支。
if not "%ROOT_PATH:>=%"=="%ROOT_PATH%" goto unsupported_root
REM 路径包含感叹号时进入不支持分支。
if not "%ROOT_PATH:!=%"=="%ROOT_PATH%" goto unsupported_root
REM 路径检查全部通过。
exit /b 0

REM 不支持的部署路径处理入口。
:unsupported_root
REM 保存路径字符限制说明。
set "FAIL_MESSAGE=部署目录不能包含 &、|、<、> 或 ! 等批处理特殊字符：%ROOT_PATH%"
REM 返回路径不支持错误。
exit /b 1

REM 创建运行目录的子程序。
:prepare_directories
REM 确保 Registry LMDB 目录存在。
call :ensure_directory "db\registry"
REM Registry 目录创建失败时立即返回。
if errorlevel 1 exit /b 1
REM 确保默认 Node1 数据目录存在。
call :ensure_directory "db\node1"
REM Node1 目录创建失败时立即返回。
if errorlevel 1 exit /b 1
REM 确保统一日志目录存在。
call :ensure_directory "log"
REM 日志目录创建失败时立即返回。
if errorlevel 1 exit /b 1
REM 确保生成服务配置目录存在。
call :ensure_directory "config\service"
REM 服务配置目录创建失败时立即返回。
if errorlevel 1 exit /b 1
REM 所有目录准备完成。
exit /b 0

REM 单个目录创建子程序，参数 1 是目标目录。
:ensure_directory
REM 目录已经存在时直接返回成功。
if exist "%~1\" exit /b 0
REM 创建缺失目录并隐藏普通输出。
mkdir "%~1" >nul 2>&1
REM 创建后再次确认目录存在。
if exist "%~1\" exit /b 0
REM 保存无法创建的具体目录。
set "FAIL_MESSAGE=无法创建目录：%~1"
REM 返回目录创建失败。
exit /b 1

REM 生成 Windows 服务专用绝对路径配置的子程序。
:generate_service_configs
REM 把 Windows 反斜杠路径转换为 Ice 配置兼容的正斜杠路径。
set "ROOT_CONFIG=%CD:\=/%"
REM 复制 Registry 基础配置作为服务配置。
copy /y "config\registry.cfg" "%REGISTRY_CONFIG%" >nul
REM Registry 配置复制失败时记录错误。
if errorlevel 1 (
	REM 保存 Registry 配置生成失败说明。
	set "FAIL_MESSAGE=无法生成 Registry 服务配置。"
	REM 返回配置生成失败。
	exit /b 1
)
REM 在生成配置末尾添加分隔空行。
>>"%REGISTRY_CONFIG%" echo.
REM 标记该配置由脚本生成，禁止手工维护。
>>"%REGISTRY_CONFIG%" echo # 由 tool/install_icegrid_wtgrid.bat 自动生成，请勿手工修改。
REM 用部署根目录绝对路径覆盖 Registry LMDB 目录。
>>"%REGISTRY_CONFIG%" echo IceGrid.Registry.LMDB.Path=%ROOT_CONFIG%/db/registry
REM 用部署根目录绝对路径覆盖 Registry 标准输出。
>>"%REGISTRY_CONFIG%" echo Ice.StdOut=%ROOT_CONFIG%/log/registry_stdout.txt
REM 用部署根目录绝对路径覆盖 Registry 标准错误。
>>"%REGISTRY_CONFIG%" echo Ice.StdErr=%ROOT_CONFIG%/log/registry_stderr.txt

REM 复制 Node 基础配置作为服务配置。
copy /y "config\node.cfg" "%NODE_CONFIG%" >nul
REM Node 配置复制失败时记录错误。
if errorlevel 1 (
	REM 保存 Node 配置生成失败说明。
	set "FAIL_MESSAGE=无法生成 Node 服务配置。"
	REM 返回配置生成失败。
	exit /b 1
)
REM 在生成配置末尾添加分隔空行。
>>"%NODE_CONFIG%" echo.
REM 标记该配置由脚本生成，禁止手工维护。
>>"%NODE_CONFIG%" echo # 由 tool/install_icegrid_wtgrid.bat 自动生成，请勿手工修改。
REM 用部署根目录绝对路径覆盖 Node1 数据目录。
>>"%NODE_CONFIG%" echo IceGrid.Node.Data=%ROOT_CONFIG%/db/node1
REM 用部署根目录绝对路径覆盖托管业务进程输出目录。
>>"%NODE_CONFIG%" echo IceGrid.Node.Output=%ROOT_CONFIG%/log
REM 用部署根目录绝对路径覆盖 Node 标准输出。
>>"%NODE_CONFIG%" echo Ice.StdOut=%ROOT_CONFIG%/log/node_stdout.txt
REM 用部署根目录绝对路径覆盖 Node 标准错误。
>>"%NODE_CONFIG%" echo Ice.StdErr=%ROOT_CONFIG%/log/node_stderr.txt
REM 两份服务配置生成完成。
exit /b 0

REM 为 LocalService 授予所有运行目录修改权限的子程序。
:grant_runtime_permissions
REM 授权 Registry LMDB 目录及其子项。
call :grant_directory_permission "%CD%\db\registry"
REM Registry 目录授权失败时立即返回。
if errorlevel 1 exit /b 1
REM 授权 Node1 数据目录及其子项。
call :grant_directory_permission "%CD%\db\node1"
REM Node1 目录授权失败时立即返回。
if errorlevel 1 exit /b 1
REM 授权统一日志目录及其子项。
call :grant_directory_permission "%CD%\log"
REM 日志目录授权失败时立即返回。
if errorlevel 1 exit /b 1
REM 所有运行目录授权完成。
exit /b 0

REM 单个目录 LocalService ACL 授权子程序。
:grant_directory_permission
REM SID S-1-5-19 是跨语言 Windows 系统一致的 LocalService 账户。
icacls "%~1" /grant "*S-1-5-19:(OI)(CI)M" /T /C >nul 2>&1
REM 授权成功时直接返回。
if not errorlevel 1 exit /b 0
REM 保存无法授权的具体目录。
set "FAIL_MESSAGE=无法为 LocalService 授予目录写权限：%~1"
REM 返回 ACL 授权失败。
exit /b 1

REM Windows 服务存在性检查子程序。
:service_exists
REM 查询参数 1 指定的服务。
sc query "%~1" >nul 2>&1
REM 返回服务控制管理器查询结果。
exit /b %errorlevel%

REM 验证已有服务是否可安全复用的子程序。
:validate_service
REM 检查服务 ImagePath 是否包含当前生成配置的绝对路径。
reg query "HKLM\SYSTEM\CurrentControlSet\Services\%~1" /v ImagePath 2>nul | find /i "%~2" >nul
REM 服务指向其他部署目录时拒绝静默覆盖。
if errorlevel 1 (
	REM 保存部署目录不匹配说明。
	set "FAIL_MESSAGE=服务 %~1 已存在，但指向其他部署目录。请先运行 tool\uninstall_icegrid_wtgrid.bat。"
	REM 返回服务配置不匹配。
	exit /b 1
)
REM 检查服务 Start 值 0x2，即 Windows 自动启动。
reg query "HKLM\SYSTEM\CurrentControlSet\Services\%~1" /v Start 2>nul | find /i "0x2" >nul
REM 已有服务不是自动启动时拒绝复用。
if errorlevel 1 (
	REM 保存启动策略不匹配说明。
	set "FAIL_MESSAGE=服务 %~1 已存在，但不是自动启动。请先卸载后重新安装。"
	REM 返回服务配置不匹配。
	exit /b 1
)
REM 检查服务运行账户是否为 LocalService。
reg query "HKLM\SYSTEM\CurrentControlSet\Services\%~1" /v ObjectName 2>nul | find /i "LocalService" >nul
REM 已有服务账户不匹配时拒绝复用。
if errorlevel 1 (
	REM 保存服务账户不匹配说明。
	set "FAIL_MESSAGE=服务 %~1 已存在，但运行账户不是 LocalService。请先卸载后重新安装。"
	REM 返回服务配置不匹配。
	exit /b 1
)
REM 参数 3 为空表示 Registry 无需检查服务依赖。
if "%~3"=="" exit /b 0
REM Node 必须依赖同机 Registry 服务。
reg query "HKLM\SYSTEM\CurrentControlSet\Services\%~1" /v DependOnService 2>nul | find /i "%~3" >nul
REM 找到依赖时返回服务验证成功。
if not errorlevel 1 exit /b 0
REM 保存 Node 服务依赖缺失说明。
set "FAIL_MESSAGE=服务 %~1 缺少对 %~3 的依赖。请先卸载后重新安装。"
REM 返回服务配置不匹配。
exit /b 1

REM 启动指定 Windows 服务的通用子程序。
:start_service
REM 先检查服务是否已经运行。
call :is_running "%~1"
REM 已运行时不重复执行 sc start。
if not errorlevel 1 (
	REM 显示服务已运行。
	echo [INFO] %~2 服务已在运行。
	REM 返回启动成功。
	exit /b 0
)
REM 创建本次 sc start 独立的临时输出文件。
set "SERVICE_OUTPUT=%TEMP%\icegrid_service_%RANDOM%.txt"
REM 显示即将启动的服务角色。
echo [INFO] 启动 %~2 服务...
REM 请求 Windows 服务控制管理器启动服务。
sc start "%~1" >"%SERVICE_OUTPUT%" 2>&1
REM sc start 失败后再次查询，兼容并发启动竞态。
if errorlevel 1 (
	REM 再次检查服务最终状态。
	call :is_running "%~1"
	REM 服务已由其他操作启动时清理并返回成功。
	if not errorlevel 1 (
		REM 删除 sc start 临时输出。
		del /q "%SERVICE_OUTPUT%" >nul 2>&1
		REM 返回启动成功。
		exit /b 0
	)
	REM 显示服务控制管理器的原始错误。
	type "%SERVICE_OUTPUT%"
	REM 删除 sc start 临时输出。
	del /q "%SERVICE_OUTPUT%" >nul 2>&1
	REM 保存启动命令失败说明。
	set "FAIL_MESSAGE=%~2 服务启动命令失败。"
	REM 返回启动失败。
	exit /b 1
)
REM sc start 成功后删除临时输出。
del /q "%SERVICE_OUTPUT%" >nul 2>&1
REM 等待服务真正进入 Running。
call :wait_running "%~1"
REM 服务按时运行时返回成功。
if not errorlevel 1 exit /b 0
REM 保存服务状态超时说明。
set "FAIL_MESSAGE=%~2 服务未在 30 秒内进入 Running 状态。"
REM 返回启动超时。
exit /b 1

REM 判断指定服务是否处于 Running 的子程序。
:is_running
REM Windows STATE 代码 4 表示 Running。
sc query "%~1" 2>nul | findstr /r /c:"STATE *: *4" >nul
REM 返回 findstr 检查结果。
exit /b %errorlevel%

REM 等待指定服务进入 Running 的子程序。
:wait_running
REM 每秒检查一次，最多检查 30 次。
for /l %%I in (1,1,30) do (
	REM 查询参数 1 指定的服务状态。
	call :is_running "%~1"
	REM 一旦进入 Running 立即成功返回。
	if not errorlevel 1 exit /b 0
	REM 未运行时等待 1 秒再检查。
	timeout /t 1 /nobreak >nul
)
REM 30 秒后仍未运行则返回失败。
exit /b 1

REM 等待 Registry 管理接口可用的子程序。
:wait_registry
REM 创建本次健康检查独立的临时输出路径。
set "CHECK_OUTPUT=%TEMP%\icegrid_registry_check_%RANDOM%.txt"
REM 每秒执行一次 node list，最多检查 30 次。
for /l %%I in (1,1,30) do (
	REM node list 成功表示 Registry 管理接口已可用。
	bin\icegridadmin.exe --Ice.Config=config\admin.cfg -e "node list" >"%CHECK_OUTPUT%" 2>&1
	REM 命令成功时清理临时文件并返回。
	if not errorlevel 1 (
		REM 删除健康检查临时输出。
		del /q "%CHECK_OUTPUT%" >nul 2>&1
		REM 显示 Registry 健康检查通过。
		echo [INFO] Registry 健康检查通过。
		REM 返回健康检查成功。
		exit /b 0
	)
	REM Registry 尚未就绪时等待 1 秒。
	timeout /t 1 /nobreak >nul
)
REM 超时后显示 icegridadmin 最后一次诊断输出。
type "%CHECK_OUTPUT%"
REM 删除健康检查临时输出。
del /q "%CHECK_OUTPUT%" >nul 2>&1
REM 保存 Registry 管理接口超时说明。
set "FAIL_MESSAGE=Registry 已启动，但管理接口在 30 秒内不可用。"
REM 返回健康检查失败。
exit /b 1

REM 等待默认 Node1 注册到 Registry 的子程序。
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
		findstr /i /c:"CloudNetNode1" "%CHECK_OUTPUT%" >nul
		REM 找到 Node 时清理临时文件并返回成功。
		if not errorlevel 1 (
			REM 删除健康检查临时输出。
			del /q "%CHECK_OUTPUT%" >nul 2>&1
			REM 显示 Node 健康检查通过。
			echo [INFO] Node 健康检查通过。
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
set "FAIL_MESSAGE=Node 服务已启动，但 CloudNetNode1 未在 30 秒内注册。"
REM 返回健康检查失败。
exit /b 1

REM 失败时只回滚本次新建服务的子程序。
:rollback
REM 本次新建了 Node 时才允许卸载 Node。
if "%CREATED_NODE%"=="1" (
	REM 显示 Node 回滚操作。
	echo [INFO] 回滚本次新建的 Node 服务...
	REM 卸载本次新建的 Node 服务，保留 db 和 log。
	bin\iceserviceinstall.exe --nopause --uninstall icegridnode "%NODE_CONFIG%"
)
REM 本次新建了 Registry 时才允许卸载 Registry。
if "%CREATED_REGISTRY%"=="1" (
	REM 显示 Registry 回滚操作。
	echo [INFO] 回滚本次新建的 Registry 服务...
	REM 卸载本次新建的 Registry 服务，保留 db 和 log。
	bin\iceserviceinstall.exe --nopause --uninstall icegridregistry "%REGISTRY_CONFIG%"
)
REM 回滚子程序结束。
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
REM 显示 Registry 错误日志位置。
echo [INFO] Registry 日志：%CD%\log\registry_stderr.txt
REM 显示 Node 错误日志位置。
echo [INFO] Node 日志：%CD%\log\node_stderr.txt
REM 只回滚本次运行创建的服务。
call :rollback
REM 人工运行时暂停，--nopause 调用时跳过。
if "%PAUSE_ON_ERROR%"=="1" pause
REM 把失败码返回给调用方。
exit /b %FAIL_CODE%

REM 参数错误入口。
:invalid_arguments
REM 打印允许的调用方式。
echo [ERROR] 用法：install_icegrid_wtgrid.bat [--nopause]
REM 默认暂停以便双击运行时查看错误。
pause
REM 使用参数错误返回码结束脚本。
exit /b 2

REM 帮助信息输出入口。
:show_help
REM 打印脚本功能。
echo 用途：安装并启动本机 IceGrid 3.8.2 Registry 和 CloudNetNode1 Windows 服务。
REM 打印调用格式。
echo 用法：install_icegrid_wtgrid.bat [--nopause]
REM 打印人工调用示例。
echo 示例：install_icegrid_wtgrid.bat
REM 打印自动化调用示例。
echo 示例：install_icegrid_wtgrid.bat --nopause
REM 说明管理员权限要求。
echo 前置条件：必须以管理员身份运行，bin 和 config 基础文件必须完整。
REM 说明安装结果。
echo 服务：icegridregistry.MtIceGrid、icegridnode.MtIceGrid.CloudNetNode1。
REM 说明运行数据位置。
echo 数据与日志：db\registry、db\node1、log；已有数据不会被清空。
REM 使用成功返回码结束帮助输出。
exit /b 0
