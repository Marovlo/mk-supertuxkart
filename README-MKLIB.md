# mk-supertuxkart

SuperTuxKart + **mklib**：让一台电脑上的**多把物理键盘**各自控制一个玩家，实现真正的本地分屏多人。

上游 SuperTuxKart 的分屏多人只支持"一个玩家一把手柄"，键盘则被所有玩家共用——因为操作系统/SDL 的键盘事件**不带设备来源**，两把键盘按同一个键在游戏里无法区分。`mklib` 补上了这个缺失：它报告每个按键事件来自哪个物理设备。

本仓库只改了输入层；玩法、数值、UI、美术音乐全部沿用上游，资产也继续使用官方 SVN。

> 当前状态：**macOS 已实机验证双键盘分屏游玩**。Windows 需要完成[文末](#windows-后续开发工作)的事件循环接线。

---

## 目录结构

代码与资产是分开的两个来源（与上游一致），请放在**同级目录**：

```
supertuxkart/
├── stk-code/      ← 本仓库（Git）
└── stk-assets/    ← 官方资产（SVN，不入库）
```

CMake 默认开启 `CHECK_ASSETS` 检查 `../stk-assets` 是否存在。

---

## 一、获取代码与资产

```bash
mkdir -p supertuxkart && cd supertuxkart

# 代码（本仓库）
git clone git@github.com:Marovlo/mk-supertuxkart.git stk-code

# 资产（官方 SVN，约 1.4 GB）
svn checkout https://svn.code.sf.net/p/supertuxkart/code/stk-assets stk-assets
```

Windows 上没有 `svn` 可用时：

- 安装 [SlikSVN](https://sliksvn.com/download/) 或 `winget install TortoiseSVN`（命令行用 `svn`）
- 或用支持 SVN 的图形客户端（TortoiseSVN）检出同一地址

---

## 二、依赖

### macOS

```bash
brew install cmake subversion sdl2 openal-soft libogg libvorbis curl libjpeg libsamplerate
```

首次运行需在 `系统设置 → 隐私与安全性 → 输入监控` 中授权 SuperTuxKart（mklib 需要）。

### Linux（Debian/Ubuntu）

```bash
sudo apt-get install build-essential cmake libbluetooth-dev libsdl2-dev \
  libcurl4-openssl-dev libenet-dev libfreetype6-dev libharfbuzz-dev \
  libjpeg-dev libogg-dev libopenal-dev libpng-dev libssl-dev libvorbis-dev \
  libmbedtls-dev pkg-config zlib1g-dev subversion
```

### Windows

- Visual Studio（2019 或更新，含 C++ 桌面开发）
- CMake
- SDL2 开发库
- 或直接下载上游提供的 [Windows 依赖包](https://github.com/supertuxkart/stk-code/releases)（含 SDL2/OpenAL/curl 等预编译库）

---

## 三、编译

### 关键 CMake 选项

| 选项 | 默认 | 说明 |
|---|---|---|
| `USE_MKLIB` | `OFF` | **必须设为 `ON`** 才启用 mklib 多键盘输入 |
| `MKLIB_ROOT` | 空 | `USE_MKLIB=ON` 时**必填**，指向 mklib 源码目录 |
| `CHECK_ASSETS` | `ON` | 检查 `../stk-assets` 是否存在 |
| `CMAKE_BUILD_TYPE` | — | `Debug` 或 `Release` |
| `DEBUG_SYMBOLS` | `OFF` | Release 下是否需要调试符号 |
| `SERVER_ONLY` | `OFF` | 无图形/声音的服务端构建 |

### Release（日常使用、性能优先）

```bash
cd stk-code

cmake -S . -B build-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DUSE_MKLIB=ON \
  -DMKLIB_ROOT=/path/to/mklib

cmake --build build-release --target supertuxkart -j 8
```

产物：

- macOS：`build-release/bin/supertuxkart.app`
- Linux：`build-release/bin/supertuxkart`
- Windows：`build-release/bin/supertuxkart.exe`

### Debug（开发 mklib/输入层时推荐）

```bash
cmake -S . -B build-debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DDEBUG_SYMBOLS=ON \
  -DUSE_MKLIB=ON \
  -DMKLIB_ROOT=/path/to/mklib

cmake --build build-debug --target supertuxkart -j 8
```

Debug 版会输出更多日志，便于看到 mklib 的设备注册与按键路由：

```
[info   ] mklib: Registered keyboard id=1 name=... persistent=...
[info   ] mklib: Keyboard input started on macOS (access=granted, 3 keyboard(s)).
[info   ] mklib: key phys=1 usage=0x4f irr=0x27 shift=0 -> device=-1
```

日志同时写入：

- macOS：`~/Library/Application Support/SuperTuxKart/config-0.10/stdout.log`
- Linux：`~/.config/supertuxkart/config-0.10/stdout.log`
- Windows：`%APPDATA%\SuperTuxKart\config-0.10\stdout.log`

### Windows（MSVC）

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
  -DUSE_MKLIB=ON -DMKLIB_ROOT=C:\path\to\mklib
cmake --build build --config Release --target supertuxkart
```

---

## 四、运行与验证

```bash
# macOS
./build-release/bin/supertuxkart.app/Contents/MacOS/supertuxkart

# Linux
./build-release/bin/supertuxkart
```

验证步骤：

1. 主菜单用**任意**键盘操作；
2. 进入 **Split Screen（分屏）**；
3. 卡丁车选择界面：玩家 1 用键盘 A 按 **Space** 加入；
4. 玩家 2 用键盘 B 按 **Space** 加入 → 屏幕一分为二；
5. 开始比赛，两人各用各的键盘驾驶（方向键转向/油门、Space 开火、N 氮气、V 漂移）。

预期：两人操作互不影响。默认键位见 `src/input/keyboard_config.cpp`。

---

## Windows 后续开发工作

macOS 由 IOHID 后台直接收事件；Windows 走 Raw Input，需要窗口句柄并转发窗口消息。以下按优先级排列。

### 1. 挂接 SDL 系统消息事件（必须，尚未接线）

Windows 上 STK 用 SDL 窗口，原始 Windows 消息被 SDL 吞掉。需要启用并转发 `SDL_SYSWMEVENT`，mklib 才能收到 `WM_INPUT` / `WM_INPUT_DEVICE_CHANGE`。

桥接层已提供接口（`src/input/mklib/mklib_input_bridge.cpp`）：

```cpp
bool MklibInputBridge::processWindowsMessage(unsigned int message,
                                             uintptr_t wparam, intptr_t lparam);
```

在窗口创建之后、事件循环中加入：

```cpp
// 初始化时
SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

// SDL 事件处理处（CIrrDeviceSDL::run() / handleEvents，或 STK 的事件过滤器）
if (event.type == SDL_SYSWMEVENT)
{
    const SDL_SysWMmsg *msg = event.syswm.msg;
    if (msg->subsystem == SDL_SYSWM_WINDOWS)
    {
        MklibInputBridge::get()->processWindowsMessage(
            msg->msg.win.msg,
            (uintptr_t)msg->msg.win.wParam,
            (intptr_t)msg->msg.win.lParam);
    }
}
```

### 2. 确认窗口附加成功（接口已实现，需验证）

`attachToSDLWindow()` 已实现，并在 `update()` 中每秒重试（启动时窗口可能还没焦点）。日志中出现以下内容即成功：

```
[info   ] mklib: Attached game window for Raw Input.
[info   ] mklib: Registered keyboard id=... name=... persistent=...
```

### 3. 待确认项

| 项目 | 说明 |
|---|---|
| 是否加 `MKLIB_WINDOWS_ATTACH_INPUT_SINK` | 决定窗口无焦点时是否仍接收输入；按游戏需要决定 |
| 热插拔 | `WM_INPUT_DEVICE_CHANGE` 是否触发；拔插键盘后玩家绑定是否保留 |
| `device_id` 稳定性 | macOS 上 `persistent_id` 稳定；Windows 需复核重启后编号是否变化 |
| 焦点丢失 | 切出窗口再回来，按键状态是否残留（macOS 路径已由 mklib 处理） |

### 4. 调试建议

先在 Windows 上用 Debug 构建跑一次，确认：

1. `mklib: Keyboard input started on Windows (access=not applicable, N keyboard(s))`
2. 每把键盘都出现 `Registered keyboard`
3. 按不同键盘时 `key phys=... ` 的 `phys` 值不同，且 `-> device=` 与注册时一致

三项都符合后再进入分屏测试。若 `Registered keyboard` 未出现，优先检查窗口附加；若按键无响应，优先检查 `SDL_SYSWMEVENT` 转发。

---

## 故障排查

| 现象 | 原因与处理 |
|---|---|
| 日志 `access=denied` | macOS 未授权输入监控；到系统设置授权后重启游戏 |
| 键盘完全没反应 | mklib 启动失败时应自动回退到普通键盘事件；若仍无反应，检查是否 `USE_MKLIB=ON` 但 `MKLIB_ROOT` 错误 |
| 两把键盘控制同一个玩家 | 多半是玩家加入时按"按键"而非"设备"匹配；确认 `getKeyboardFromDeviceId()` 生效（日志中 `-> device=` 应为各自物理 ID） |
| 每局后 `input.xml` 里键盘变多 | 设备复用逻辑失效；检查 `refreshDevices()` 是否复用了 `getMklibId() < 0` 的设备 |
| 文本框打不出字 | 字符映射缺失；检查 `usageToChar()` 与合成事件是否被 `InputManager` 放行 |

---

## 设计说明

适配原理、改动清单与路由策略见 [`MKLIB_INTEGRATION.md`](MKLIB_INTEGRATION.md)。

## 许可证

本仓库基于 SuperTuxKart，沿用 **GNU GPL v3**（见 `COPYING`）。mklib 部分遵循其自身许可证。
