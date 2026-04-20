#!/bin/bash

# --- 配置路径变量 ---
# 源文件路径 (桌面)
SRC_FILE="/home/book/Desktop/linux_bsp/100ask_imx6ull-14x14.dts"

# 内核源码目录
KERNEL_DIR="/home/book/100ask_imx6ull-sdk/Linux-4.9.88"

# 目标设备树目录
DTS_DIR="${KERNEL_DIR}/arch/arm/boot/dts"

# 目标文件名 (根据你的描述，源文件和目标文件名一致)
DTB_FILE_NAME="100ask_imx6ull-14x14.dtb"

echo "----------------------------------------"
echo "1. 正在复制源文件到内核源码目录..."
echo "----------------------------------------"

# 检查源文件是否存在
if [ ! -f "$SRC_FILE" ]; then
    echo "❌ 错误：源文件不存在！路径: $SRC_FILE"
    exit 1
fi

# 执行复制并覆盖 (-f 强制覆盖)
cp -f "$SRC_FILE" "$DTS_DIR/"

if [ $? -eq 0 ]; then
    echo "✅ 文件复制成功。"
else
    echo "❌ 文件复制失败。"
    exit 1
fi

echo ""
echo "----------------------------------------"
echo "2. 正在编译设备树 (make dtbs)..."
echo "----------------------------------------"

# 进入内核目录
cd "$KERNEL_DIR"

# 执行编译
# 注意：这里假设你已经配置好了交叉编译工具链 (ARCH, CROSS_COMPILE 等环境变量)
make dtbs

if [ $? -eq 0 ]; then
    echo "✅ 编译成功。"
else
    echo "❌ 编译失败，请检查错误信息。"
    exit 1
fi

echo ""
echo "----------------------------------------"
echo "3. 正在通过 ADB 推送文件到开发板..."
echo "----------------------------------------"

# 检查生成的 dtb 文件是否存在
TARGET_DTB="${DTS_DIR}/${DTB_FILE_NAME}"
if [ ! -f "$TARGET_DTB" ]; then
    echo "❌ 错误：编译生成的 dtb 文件未找到！路径: $TARGET_DTB"
    exit 1
fi

# 检查 adb 连接
adb devices | grep "device$"
if [ $? -ne 0 ]; then
    echo "⚠️ 警告：未检测到 adb 设备，请确保开发板已连接且 adb 驱动正常。"
fi

# 推送文件
adb push "$TARGET_DTB" /boot/

if [ $? -eq 0 ]; then
    echo "✅ 推送成功！文件已更新至 /boot/${DTB_FILE_NAME}"
    echo "💡 提示：请记得在 Uboot 中设置 bootcmd 或重启以加载新设备树。"
else
    echo "❌ 推送失败。"
    exit 1
fi

echo "----------------------------------------"
echo "🎉 所有步骤完成！"