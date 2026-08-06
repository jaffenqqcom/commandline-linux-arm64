#!/bin/bash
# =========================================================================
# commandline-linux-arm64 环境设置脚本
# 用法：source env.sh
# 一次性设置所有工具链环境变量，之后可直接运行 ohpm / hvigor / build-hap.sh
# =========================================================================

export TOOLCHAIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 1) node 与 ohpm：加入 PATH（目录内自带 arm64 node、ohpm 包管理器）
export PATH="$TOOLCHAIN_DIR/tool/node/bin:$TOOLCHAIN_DIR/ohpm/bin:$PATH"
export NODE_CMD="$TOOLCHAIN_DIR/tool/node/bin/node"

# 2) hvigor / SDK：构建工具链路径
export HVIGOR_DIR="$TOOLCHAIN_DIR/hvigor"
export DEVECO_SDK_HOME="$TOOLCHAIN_DIR/sdk"
export HARMONYBREW_SDK="$TOOLCHAIN_DIR/sdk"

echo "[env.sh] 工具链环境已设置："
echo "  TOOLCHAIN_DIR   = $TOOLCHAIN_DIR"
echo "  HVIGOR_DIR      = $HVIGOR_DIR"
echo "  DEVECO_SDK_HOME = $DEVECO_SDK_HOME"
echo "  node            = $(command -v node)"
