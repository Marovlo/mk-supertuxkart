#!/usr/bin/env bash
# ============================================================================
# SuperTuxKart + mklib 一键构建脚本 (macOS)
#
# 用法:
#   ./build-macos.sh                # 默认构建 (Release)
#   ./build-macos.sh Debug          # Debug 模式
#   MKLIB_ROOT=/path/to/mklib ./build-macos.sh   # 自定义 mklib 路径
#   ./build-macos.sh --run          # 构建后直接运行
#
# 前置条件:
#   - macOS 10.15+ (推荐 Apple Silicon 或 Intel)
#   - Homebrew (https://brew.sh)
#   - Git
# ============================================================================

set -euo pipefail

BUILD_TYPE="${1:-Release}"
RUN_AFTER=false
if [[ "${2:-}" == "--run" || "${1:-}" == "--run" ]]; then
    RUN_AFTER=true
    [[ "${1:-}" == "--run" ]] && BUILD_TYPE="Release"
fi

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*"; exit 1; }

# --- 路径检测 ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
STK_CODE_DIR="${SCRIPT_DIR}"
BUILD_DIR="${STK_CODE_DIR}/build-macos"

# 自动检测 mklib 路径
if [[ -z "${MKLIB_ROOT:-}" ]]; then
    # 尝试相对路径: stk-code → ../../mklib
    MAYBE_MKLIB="$(cd "${STK_CODE_DIR}/../../mklib" 2>/dev/null && pwd)"
    if [[ -d "${MAYBE_MKLIB}/include" ]]; then
        MKLIB_ROOT="${MAYBE_MKLIB}"
    fi
fi
[[ -z "${MKLIB_ROOT:-}" ]] && error "未找到 mklib。请设置环境变量:\n  export MKLIB_ROOT=/path/to/mklib\n  或将 mklib 放在 stk-code 的 ../../mklib 位置"
[[ ! -d "${MKLIB_ROOT}" ]] && error "mklib 路径不存在: ${MKLIB_ROOT}"
info "mklib 路径: ${MKLIB_ROOT}"

# --- Homebrew 依赖 ---
if ! command -v brew &>/dev/null; then
    error "未安装 Homebrew。请先安装:\n  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
fi

info "检查 Homebrew 依赖..."
brew bundle --file="${STK_CODE_DIR}/Brewfile" --no-lock 2>/dev/null || {
    warn "brew bundle 未完全满足，尝试逐个安装..."
    while IFS= read -r line; do
        [[ "$line" =~ ^brew\ \"([^\"]+)\" ]] && brew install "${BASH_REMATCH[1]}"
    done < "${STK_CODE_DIR}/Brewfile"
}
info "依赖检查完成"

# --- stk-assets 检查 ---
STK_ASSETS_DIR="$(cd "${STK_CODE_DIR}/../stk-assets" 2>/dev/null && pwd)"
if [[ ! -d "${STK_ASSETS_DIR}" ]]; then
    warn "未找到 stk-assets 目录 (${STK_CODE_DIR}/../stk-assets)"
    warn "如果你还没有下载资产，请运行:"
    warn "  svn checkout https://svn.code.sf.net/p/supertuxkart/code/stk-assets ${STK_CODE_DIR}/../stk-assets"
    warn "或者从 GitHub Release 下载: https://github.com/supertuxkart/stk-assets/releases"
    echo ""
    read -r -p "继续构建（无资产将无法完整运行游戏）? [y/N] " yn
    [[ "${yn,,}" != "y" ]] && exit 0
fi

# --- CMake 配置 ---
info "CMake 配置 (Build Type: ${BUILD_TYPE})..."
cmake "${STK_CODE_DIR}" \
    -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DUSE_WIIUSE=OFF \
    -DUSE_MKLIB=ON \
    -DMKLIB_ROOT="${MKLIB_ROOT}" \
    -DCHECK_ASSETS=OFF \
    || error "CMake 配置失败"

# --- 编译 ---
info "开始编译 (可能需要几分钟)..."
CPU_COUNT="$(sysctl -n hw.ncpu)"
cmake --build "${BUILD_DIR}" --parallel "${CPU_COUNT}" \
    || error "编译失败"

# --- 完成 ---
echo ""
info "=========================================="
info "构建成功!"
info "可执行文件: ${BUILD_DIR}/bin/SuperTuxKart.app"
info "构建目录:   ${BUILD_DIR}"
info "=========================================="

if [[ "${RUN_AFTER}" == true ]]; then
    info "启动游戏..."
    open "${BUILD_DIR}/bin/SuperTuxKart.app"
fi
