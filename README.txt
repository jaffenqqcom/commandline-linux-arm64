=========================================================================
 commandline-linux-arm64
 HarmonyOS NEXT HAP 构建工具链（arm64 Linux 版）
=========================================================================

一、概述
-------------------------------------------------------------------------
本目录是一套自包含、可移植的 HarmonyOS NEXT HAP 构建工具链，专门用于
在 arm64 Linux（如 openEuler 24.03 aarch64）上编译打包 .hap 应用。

核心思路：在官方Command Line Tools for Linux(x86) 6.1.1.300（2.0GB）基础上，
  - 能平替的工具替换成 arm64 原生版本
  - 华为专有、无 arm64 版本的工具用 box64 仿真运行
保证目录整体复制到其他 arm64 Linux 机器后可直接使用。

hvigor       : 6.24.4
codelinter   : 6.0.240
hstack       : 5.1.0
ohpm         : 6.1.2.285
releaseType  : releaseMr2
HarmonyOS SDK: HarmonyOS 6.1.1 Release (include Ohos_sdk_public 6.1.1.125 (API Version 24 Release))
apiVersion   : 24

本工具链已实测完整构建 HAP 成功：
  原生编译 + ArkTS 编译（es2abc） + HAP 打包全部通过。

二、安装方式
-------------------------------------------------------------------------
  1、下载原始包：从华为开发者网站下载原始包Command Line Tools for Linux(x86) 6.1.1.300（2.0GB）并解压
  2、在linux下，将本文件夹复制覆盖解压后的文件夹，就完成了替换。

三、修改内容（相对 linux-x64 原版）
-------------------------------------------------------------------------
1) 新增 box64 仿真器（arm64 运行 x86-64 程序）
     box64/box64        box64 可执行文件（arm64，v0.2.7 源码编译）
     box64/x64lib/      x86-64 基础库集合（libgcc_s/libstdc++ 等）

2) 替换为 arm64 原生版本（有 arm64 替代的工具）
     tool/node/bin/node          x86-64 v18.20.1 → arm64 v22.14.0
     sdk/.../native/llvm/bin/lld                 → 系统 arm64 lld
     sdk/.../native/build-tools/cmake/bin/cmake  → 系统 arm64 cmake 3.27.9
     sdk/.../native/build-tools/cmake/bin/ninja  → 系统 arm64 ninja 1.11.1
     sdk/.../toolchains/hdc ark_disasm syscap_tool
       rawheap_translator libusb_shared.so       → 从现有 SDK 复制的 arm64 版
     sdk/.../toolchains/restool                  → Node wrapper 脚本

3) 包装成脚本（华为专有、无 arm64 版本的工具，用 box64 仿真）
     原二进制改名 .orig 保留，创建 bash wrapper 调 box64：
     sdk/.../ets/build-tools/ets-loader/bin/ark/build/bin/目录：
       es2abc  merge_abc  ark_aot_compiler  panda_guard  profdump
     sdk/.../native/llvm/bin/目录：
       clang / clang++      → 系统 clang-17 wrapper（交叉编译 OHOS 目标）
       llvm-ar / llvm-objcopy → 系统 ar / objcopy wrapper

4) 新增配套数据/文件
     sdk/.../native/build-tools/cmake/share/cmake
                          系统 /usr/share/cmake 数据目录（否则 CMake 报
                          CMAKE_ROOT 错误）
     sdk/.../toolchains/restool-wrapper.js
                          restool Node wrapper 的配套脚本

	 sdk/default/openharmony/oh-uni-package.json	（SDK	版本标记，hvigor	校验需要）
	 sdk/default/hms/uni-package.json		（SDK	版本标记）

5) 未替换的 x86-64 程序（ARM 下不可用，构建 HAP 不调用，无需处理）
     sdk/.../toolchains/idl  hnpcli  spirv-remap  glslang_validator 等
     sdk/.../native/llvm/bin 下大量未用的 llvm 工具
   另外，hvigor/ ohpm/ codelinter/ hstack是 Node.js/JS 程序，不是 x86-64 机器码，架构无关，
     用 arm64 node 运行即可，无需替换。

四、对外依赖
-------------------------------------------------------------------------
node（arm64 v22.14.0）与 box64 已随本目录自带，无需额外安装。
（注意：restool 等 Node wrapper 脚本通过 PATH 找 node，构建时需
  确保 PATH 包含 node，可用目录内 tool/node/bin 或系统 node。）

以下为需要外部提供或满足的条件：

1) glibc：arm64 Linux 系统库（openEuler 24.03 = glibc 2.38）
2) 系统工具（openEuler 默认大多自带）：
     /usr/bin/clang        /usr/bin/clang++      （clang 17）
     /usr/bin/lld
     /usr/bin/cmake        /usr/bin/ninja
     /usr/bin/ar           /usr/bin/objcopy      （binutils 提供）
3) JDK 17+：hvigor 打包 HAP（PackageHap/SignHap）必须，系统无默认 JDK
4) 网络：ohpm 依赖安装需要访问 registry
     （默认 https://ohpm.openharmony.cn/ohpm/，已实测可达）
5) 共享文件系统限制：CMake 在共享文件系统（如 /mnt/linux_share）上
     会因 configure_file/FetchContent 写操作失败（Operation not permitted）。
     构建必须在本地文件系统（如 /tmp）进行。

五、安装依赖（openEuler 24.03）
-------------------------------------------------------------------------
1) 安装 JDK 17（hvigor 打包必需）：
     sudo dnf install -y java-17-openjdk java-17-openjdk-devel
   验证：java -version

2) 确认系统工具齐全（openEuler 24.03 默认已装）：
     clang --version      # clang 17.x（提供 /usr/bin/clang、clang++）
     cmake --version      # 3.27.x（提供 /usr/bin/cmake）
     ninja --version      # 1.11.x（提供 /usr/bin/ninja）
     lld --version        # 提供 /usr/bin/lld
     ar --version         # binutils（提供 /usr/bin/ar、/usr/bin/objcopy）
   说明：本工具链的 llvm-ar / llvm-objcopy 依赖 /usr/bin/ar 与
     /usr/bin/objcopy（属于 binutils）。
   如缺失：sudo dnf install -y clang cmake ninja-build lld binutils

3) 网络：确保可访问 https://ohpm.openharmony.cn/ohpm/（ohpm 依赖源）

4) 安装项目 ohpm 依赖（前提：HAP 工程已复制到本地文件系统 /tmp/hap，
   其外部依赖 depends 已复制为 /tmp/depends 真实目录，不能用软链接）：
     先 source 环境：source <本目录>/env.sh
     再执行：bash -c "cd /tmp/hap && ohpm install --all"

5) 设置环境变量，构建 HAP（source env.sh 后所有环境变量已就位，无需手动传参）：
     source <本目录>/env.sh

   说明：env.sh 一次设置 PATH（加入目录内 node）、NODE_CMD、
     HVIGOR_DIR、DEVECO_SDK_HOME、HARMONYBREW_SDK，如不能覆盖要求，可以修改。

七、目录结构（关键部分）
-------------------------------------------------------------------------
  commandline-linux-arm64/
  |-- box64/                  # box64 仿真器 + x64lib 库
  |-- hvigor/                 # hvigor 构建工具（Node 程序，用 arm64 node）
  |-- ohpm/                   # ohpm 包管理器（Node 程序）
  |-- sdk/
  |   `-- default/
  |       |-- openharmony/    # OHOS SDK：ets/native/toolchains/js/previewer
  |       `-- hms/            # HMS 组件（native 等）
  |-- tool/
  |   `-- node/               # arm64 node（已替换）
  |-- env.sh                  # 环境设置脚本（source 一次配好构建所需环境变量）
  |-- bin/hvigorw             # hvigor 启动脚本
  `-- README.txt              # 本文档

八、共享文件系统构建（bind mount 方案）
-------------------------------------------------------------------------
背景：在共享文件系统（如 /mnt/linux_share，NFS/9p 挂载）上构建 HAP 时，
CMake 的 configure_file / FetchContent 写操作会报 "Operation not
permitted"（共享文件系统不支持特定写操作），导致 Native 编译失败。
hvigor 把构建目录（.cxx、build）硬编码在源码目录下，无法直接配置到
本地磁盘。

解决方案：用 bind mount 把构建目录映射到本地磁盘 /tmp。
关键点：bind mount 不改变路径字符串——工具看到的仍是
/mnt/.../hap/...，内容实际存 /tmp，因此 hvigor 记录的路径与 es2abc
realpath 后一致，构建正常。
（对比：软链接会使 realpath 跳变成 /tmp/...、路径不一致，导致 ArkTS
编译失败，不推荐。）

操作步骤：
1) 准备本地构建目录（一次性）：
     mkdir -p /tmp/hap-entry-cxx /tmp/entry-build \
              /tmp/native-ability-cxx /tmp/native-ability-build

2) bind mount（每次开机/重启后执行，需 sudo）：
     HAP=/mnt/linux_share/workspace/hap
     NA=/mnt/linux_share/workspace/depends/openharmony-ability/native_ability
     mkdir -p $HAP/entry/.cxx $HAP/entry/build $NA/.cxx $NA/build
     sudo mount --bind /tmp/hap-entry-cxx        $HAP/entry/.cxx
     sudo mount --bind /tmp/entry-build          $HAP/entry/build
     sudo mount --bind /tmp/native-ability-cxx   $NA/.cxx
     sudo mount --bind /tmp/native-ability-build $NA/build

3) 设置环境并构建：
     source <本目录>/env.sh
     cd $HAP
     这里就可以执行编译脚本了

4) 产物：/tmp/entry-build/default/outputs/*/entry-*-unsigned.hap
   （拷贝回共享项目后，可 sudo umount 卸载上述挂载）

九、常见问题
-------------------------------------------------------------------------
1) CMake 报 "CMAKE_ROOT" 错误：cmake 数据目录缺失，需保留
   sdk/.../native/build-tools/cmake/share/cmake/
2) CMake 报 "Operation not permitted"：构建目录在共享文件系统，使用
   第八节的 bind mount 方案（将构建目录映射到本地 /tmp）
3) 报 "Cannot find module @ohos-rs/ability"：未执行 ohpm install --all，
   或 depends 用了软链接（需复制为真实目录）
4) 报 "spawn java ENOENT"：未安装 JDK，见第五节第 1 条
5) ArkTS 编译报 send/recv 隐式声明错误：clang wrapper 已内置
   -Wno-implicit-function-declaration，无需处理
=========================================================================
