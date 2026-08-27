# ============================================================================
# SuperTuxKart + mklib 一键构建脚本 (Windows / PowerShell)
#
# 用法:
#   .\build-windows.ps1                    # 默认构建 (Release)
#   .\build-windows.ps1 -BuildType Debug   # Debug 模式
#   .\build-windows.ps1 -Run               # 构建后直接运行
#   $env:MKLIB_ROOT = "C:\path\to\mklib"; .\build-windows.ps1  # 自定义 mklib 路径
#
# 前置条件:
#   - Windows 10 / 11
#   - Visual Studio 2019 或 2022 (含 C++ 桌面开发工作负载)
#   - CMake (通常 VS 自带)
#   - Git
#   - Subversion (用于下载 stk-assets)
#
# ============================================================================
#
# !!! 重要警告 !!!
#
# 当前 mklib 的 Windows 多键盘功能 **尚未完成**。
# 具体来说: SDL_SYSWMEVENT -> WM_INPUT (Raw Input) 的事件循环接线未实现。
# 这意味着在 Windows 上，mklib 无法区分不同键盘的输入。
#
# 构建可以成功，但多键盘分屏功能 **不会工作**。
#
# 要完成 Windows 接线，需要在以下文件中实现:
#
#   1. stk-code/src/input/MKLibInputManager.cpp (或对应文件)
#      - 在 SDL_SYSWMEVENT 处理分支中，解析 Windows 的 WM_INPUT 消息
#      示例接线代码 (伪代码):
#        #ifdef _WIN32
#        #include <windows.h>
#        #include <rawinput.h>
#        #pragma comment(lib, "user32.lib")
#
#        case SDL_SYSWMEVENT: {
#            SDL_SysWMmsg* msg = event.syswm;
#            if (msg->msg.win.msg == WM_INPUT) {
#                RAWINPUT raw;
#                UINT size = sizeof(raw);
#                GetRawInputData(
#                    (HRAWINPUT)msg->msg.win.lParam,
#                    RID_INPUT,
#                    &raw, &size,
#                    sizeof(RAWINPUTHEADER)
#                );
#                if (raw.header.dwType == RIM_TYPEKEYBOARD) {
#                    // 将 raw.data.keyboard 转发给 mklib
#                    // mklib_handle_raw_keyboard(&raw.data.keyboard, raw.header.hDevice);
#                }
#            }
#            break;
#        }
#        #endif
#
#   2. stk-code/src/main_loop.cpp
#      - 确保在 SDL_Init 后调用 SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE)
#      - 确保主事件循环处理 SDL_SYSWMEVENT 事件
#
#   3. mklib/src/mklib_platform_windows.c (如果存在)
#      - 实现 mklib 的 Windows Raw Input 注册和回调
#      - 使用 RegisterRawInputDevices 注册多个键盘
#
# 详见 README-MKLIB.md 中 "Windows 后续开发工作" 章节。
#
# ============================================================================

param(
    [string]$BuildType = "Release",
    [switch]$Run = $false
)

$ErrorActionPreference = "Stop"

# --- 辅助函数 ---
function Write-Info  { param([string]$Msg) Write-Host "[INFO] $Msg" -ForegroundColor Green }
function Write-Warn  { param([string]$Msg) Write-Host "[WARN] $Msg" -ForegroundColor Yellow }
function Write-Err { param([string]$Msg) Write-Host "[ERROR] $Msg" -ForegroundColor Red; exit 1 }

# --- 路径检测 ---
$StkCodeDir = $PSScriptRoot
$BuildDir   = Join-Path $StkCodeDir "build-windows"

# 自动检测 mklib 路径
if (-not $env:MKLIB_ROOT) {
    $MaybeMklib = Resolve-Path (Join-Path $StkCodeDir "..\..\mklib") -ErrorAction SilentlyContinue
    if ($MaybeMklib -and (Test-Path (Join-Path $MaybeMklib.Path "include"))) {
        $env:MKLIB_ROOT = $MaybeMklib.Path
    }
}
if (-not $env:MKLIB_ROOT) {
    Write-Err "未找到 mklib。请设置环境变量:`n  `$env:MKLIB_ROOT = 'C:\path\to\mklib'`n  或将 mklib 放在 stk-code 的 ..\..\mklib 位置"
}
if (-not (Test-Path $env:MKLIB_ROOT)) {
    Write-Err "mklib 路径不存在: $($env:MKLIB_ROOT)"
}
Write-Info "mklib 路径: $($env:MKLIB_ROOT)"

# --- Windows 多键盘功能警告 ---
Write-Host ""
Write-Host "============================================================" -ForegroundColor Red
Write-Host "  警告: Windows 多键盘功能尚未接线!" -ForegroundColor Red
Write-Host "  构建可以成功，但多键盘分屏不会工作。" -ForegroundColor Red
Write-Host "  详见脚本顶部的接线说明注释。" -ForegroundColor Red
Write-Host "============================================================" -ForegroundColor Red
Write-Host ""

# --- 检查工具链 ---
Write-Info "检查工具链..."

# CMake
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    # 尝试 VS 自带的 CMake
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $vsPath = & $vsWhere -latest -property installationPath 2>$null
        if ($vsPath) {
            $cmakePath = Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
            if (Test-Path $cmakePath) {
                $env:PATH = "$(Split-Path $cmakePath);$env:PATH"
            }
        }
    }
    $cmake = Get-Command cmake -ErrorAction SilentlyContinue
}
if (-not $cmake) {
    Write-Err "未找到 CMake。请安装 CMake 或 Visual Studio (含 C++ 桌面开发工作负载)。"
}
Write-Info "CMake: $($cmake.Source)"

# Visual Studio
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWhere) {
    $vsPath = & $vsWhere -latest -property installationPath 2>$null
    if ($vsPath) {
        Write-Info "Visual Studio: $vsPath"
    } else {
        Write-Err "未找到 Visual Studio 安装。请安装 Visual Studio 2019/2022 (含 C++ 桌面开发工作负载)。"
    }
} else {
    Write-Warn "未找到 vswhere，将使用默认生成器。"
}

# --- stk-assets 检查 ---
$StkAssetsDir = Join-Path $StkCodeDir "..\stk-assets"
if (-not (Test-Path $StkAssetsDir)) {
    Write-Warn "未找到 stk-assets 目录: $StkAssetsDir"
    Write-Warn "如果你还没有下载资产，请运行:"
    Write-Warn "  svn checkout https://svn.code.sf.net/p/supertuxkart/code/stk-assets $StkAssetsDir"
    Write-Warn "或者从 GitHub Release 下载: https://github.com/supertuxkart/stk-assets/releases"
    $continue = Read-Host "继续构建（无资产将无法完整运行游戏）? [y/N]"
    if ($continue -ne 'y') { exit 0 }
}

# --- 下载 Windows 依赖库 ---
$DepsDir = Join-Path $StkCodeDir "dependencies-win64"
if (-not (Test-Path $DepsDir)) {
    Write-Info "下载 Windows 依赖库..."
    $depUrl = "https://github.com/supertuxkart/dependencies-windows/archive/master.zip"
    $zipFile = Join-Path $env:TEMP "stk-deps.zip"
    Write-Info "下载: $depUrl"
    Invoke-WebRequest -Uri $depUrl -OutFile $zipFile -UseBasicParsing
    Write-Info "解压依赖库..."
    $extractDir = Join-Path $env:TEMP "stk-deps-extracted"
    Expand-Archive -Path $zipFile -DestinationPath $extractDir -Force
    $extractedDir = Join-Path $extractDir "dependencies-windows-master"
    if (Test-Path $extractedDir) {
        $win64Dir = Get-ChildItem $extractedDir -Directory | Where-Object { $_.Name -match "64" } | Select-Object -First 1
        if ($win64Dir) {
            Copy-Item $win64Dir.FullName $DepsDir -Recurse -Force
        } else {
            Copy-Item $extractedDir $DepsDir -Recurse -Force
        }
    }
    Remove-Item $zipFile -ErrorAction SilentlyContinue
    Remove-Item $extractDir -Recurse -ErrorAction SilentlyContinue
}

if (Test-Path $DepsDir) {
    Write-Info "Windows 依赖库: $DepsDir"
} else {
    Write-Warn "未能自动下载依赖库，将尝试使用系统库。"
    $DepsDir = ""
}

# --- CMake 配置 ---
Write-Info "CMake 配置 (Build Type: $BuildType)..."

$cmakeArgs = @(
    $StkCodeDir,
    "-B", $BuildDir,
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DUSE_WIIUSE=OFF",
    "-DUSE_MKLIB=ON",
    "-DMKLIB_ROOT=$($env:MKLIB_ROOT)",
    "-DCHECK_ASSETS=OFF"
)

if ($DepsDir -and (Test-Path $DepsDir)) {
    $cmakeArgs += "-DDEPENDENCIES=$DepsDir"
}

# 尝试检测 VS 版本
$configured = $false
$generators = @("Visual Studio 17 2022", "Visual Studio 16 2019")
foreach ($gen in $generators) {
    $tryArgs = $cmakeArgs + @("-G", $gen)
    Write-Info "尝试生成器: $gen"
    & cmake @tryArgs 2>&1 | Write-Host
    if ($LASTEXITCODE -eq 0) {
        $configured = $true
        break
    }
}

if (-not $configured) {
    Write-Warn "指定生成器失败，使用默认生成器..."
    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Err "CMake 配置失败"
    }
}

# --- 编译 ---
Write-Info "开始编译 (可能需要几分钟)..."
$cpuCount = $env:NUMBER_OF_PROCESSORS
cmake --build $BuildDir --config $BuildType --parallel $cpuCount
if ($LASTEXITCODE -ne 0) {
    Write-Err "编译失败"
}

# --- 完成 ---
Write-Host ""
Write-Info "=========================================="
Write-Info "构建成功!"
$exePath = Join-Path $BuildDir "bin\$BuildType\supertuxkart.exe"
if (Test-Path $exePath) {
    Write-Info "可执行文件: $exePath"
} else {
    Write-Info "可执行文件: $BuildDir\bin\ (检查具体路径)"
}
Write-Info "构建目录:   $BuildDir"
Write-Host ""
Write-Host "  注意: 多键盘功能在 Windows 上尚未接线，" -ForegroundColor Yellow
Write-Host "  构建可运行但多键盘分屏不会工作。" -ForegroundColor Yellow
Write-Info "=========================================="

if ($Run) {
    Write-Info "启动游戏..."
    if (Test-Path $exePath) {
        & $exePath
    } else {
        $exes = Get-ChildItem (Join-Path $BuildDir "bin") -Filter "supertuxkart.exe" -Recurse
        if ($exes) {
            & $exes[0].FullName
        }
    }
}
