# TASK-0011：前端菜单（New Main Menu）最小闭环

完成日期：2026-10-12，Australia/Sydney。范围是**菜单切换管理器的结构性复原 + 一条确认的页面跳转边**
（TitleScreen -> New Main Menu），以及一个可操作的 Godot 演示闭环。菜单内部渲染、具体菜单项内容、
完整输入链路均未恢复，明确标注为未解决项，不得当作已验证行为使用。

二进制：`SLUS_203.62`，SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`。
USA/NTSC 推断；实际可执行文件修订版 unknown，启动文件 VER 1.00。
Ghidra 12.0.4，`r5900:LE:32:default` / default，GhidraMCP 只读连接（127.0.0.1:8080）。

## 用户三项要求的对应情况

1. **"用原版截图和操作记录确认一个主菜单及其相邻页面"**：本会话没有原版截图/录像可用，改用
   ELF 内字符串与交叉引用作为证据来源（见下）。确认了字符串字面量 `"New Main Menu"`
   （ELF 0x002b2f00）与其相邻页面 `"TitleScreen"`（0x002b2f68）、`"MumboJumboScreen"`
   （0x002b2f78）。TitleScreen -> New Main Menu 的跳转边通过反编译确认（见 E-0011-12）；
   MumboJumboScreen 与前两者的边**未被确认**，仅作为并列注册的同级屏幕类记录。
2. **"追踪 NewMainMenu 的创建、更新、输入回调和页面切换"**：
   - **页面切换**：完全确认。`FUN_0014cc78`（RequestScreen 包装）-> `FUN_0016c4c0`
     （真正的切换/激活/销毁/历史栈逻辑）-> `FUN_0016c8c8`（按名字查找已注册屏幕）。
   - **创建**：找到了 `TitleScreen`（`FUN_0016f440`）与 `MumboJumboScreen`（`FUN_0016f810`）
     各自的"类分发函数"（message==0xb 构造、0xc 析构、默认转发到实例处理函数），这是同一引擎
     约定下的创建范式证据。但 **"New Main Menu" 自身的类分发/构造函数本会话未找到**——它的三处
     字符串引用都只是"请求切换到它"的调用点，不是构造调用点。明确记为未解决项。
   - **更新**：`FUN_0016fa80`（巨型函数，entry 0x0016fa80，Ghidra 报告的函数包络一直延伸到
     0x001e3dbf，约 465KB）在其开头约 176 字节内多次把状态值路由到 "New Main Menu"
     （state 2-7）或空字符串（state 8-9），是 New Main Menu 自身更新/tick 逻辑的强候选，但
     TASK-0003/0006/0007 已多次标记这个函数包络可能是 Ghidra 边界误判，本会话**没有**对它做
     完整反编译或边界审计，只做了只读 listing 切片检查。
   - **输入回调**：找到了 `FUN_0016eef8`（TitleScreen 实例消息处理函数），其 `message==1`
     分支在满足内部状态与忙碌标志条件时触发 `RequestScreen("New Main Menu", 1)`——这是从
     "消息"到"页面切换"的确认链路。但从**原始手柄输入**（本会话也找到了
     `JOY_EVENT_MENU_SELECT`/`_CANCEL`/`_BACK`/`_HELP` 等字符串常量）到"message==1"派发的具体
     路径**本会话未追踪**，是明确的未解决项。
3. **"在原生核心恢复导航状态，Godot 显示可操作界面"**：已完成。见下方实现与验收记录。

## 证据总览

详见每个函数各自的记录文件（`reverse/functions/<binary-id>/frontend/*.md`，Evidence ID
E-0011-01 至 E-0011-17）与 `reverse/symbols.csv` 的 frontend 子系统行。核心链路：

```mermaid
flowchart LR
    TS[TitleScreen 实例\nFUN_0016eef8 message=1] -->|RequestScreen name push=1| REQ[FUN_0014cc78]
    REQ --> SET[FUN_0016c4c0\nSetActiveFrontEndScreen]
    SET --> FIND[FUN_0016c8c8\nFindScreenByName]
    SET --> PUSH[FUN_0016b418\nPushHistory]
    SET --> POP_NOTE["FUN_0016b438 PopHistory\n(用于 Back 组合, 未在单一调用点证实)"]
    SET --> RESET[FUN_0016b460\nResetManager]
    FIND -->|strcmp| CMP[FUN_002535f8]
    FA[FUN_0016fa80\n(超大/未审计, 多状态路由到\n"New Main Menu")] -.-> SET
```

关键字符串与地址（均来自对已授权 ELF 的只读 `memory search`/`read` 调用）：

| 字符串 | ELF 地址 | 角色 |
|---|---|---|
| `New Main Menu` | 0x002b2f00 | 唯一确认的主菜单屏幕名 |
| `TitleScreen` | 0x002b2f68 | 确认的相邻/来源屏幕（class FUN_0016f440） |
| `MumboJumboScreen` | 0x002b2f78 | 确认存在的同级屏幕类（class FUN_0016f810），边未确认 |
| `Title Screen` / `Legal MumboJumbo` / `BootAutoload` / `Language Options Pre FE` | 0x002b2f90 / 0x2b2fa0 / 0x2b2fb8 / 0x2b2fc8 | `FUN_0016f950` 推送到历史栈的"已展示"标记，**不是**证实的导航顺序 |
| `JOY_EVENT_MENU_SELECT` / `_CANCEL` / `_BACK` / `_HELP` | 附近 0x002b02xx-0x2b033x 区域 | 存在手柄菜单事件名常量；未追踪到与 `FUN_0016eef8` 的调用关系 |
| `GRID` / `MASTER_BP_OBJECT` / `FADING_BP_OBJECT` | 0x00331e60 / 0x002befd0 / 0x002befe8 | `FUN_00249a40` 绑定的三个命名行为对象（疑似 bWare 引擎 blueprint 系统，而非逐键输入回调） |

管理器单例（gp 相对全局 `uGpffffacb4`）结构性字段（均来自反编译证据，完整布局未知）：

| 偏移 | 含义 | 证据 |
|---|---|---|
| `+0x000..0x180` | 历史栈，12 个 0x20 字节槽位（仅槽位首部的名字字段被证实） | E-0011-05/06 |
| `+0x180` | 历史栈计数 (int32) | E-0011-05/06 |
| `+0x184` | 未解析的标志位，与历史区相邻但不被 Reset 清除 | E-0011-02/07 |
| `+0x194` | 已注册屏幕的循环侵入式链表头 | E-0011-03 |
| `+0x19c` | 当前屏幕指针 | E-0011-02 |
| `+0x1c8` | 行为绑定记录（GRID / MASTER_BP_OBJECT / FADING_BP_OBJECT） | E-0011-11 |

## 原生实现（结构性投影，非逐字节复刻）

`src/game_core/frontend/screen_manager.hpp/.cpp`：`hp2::frontend::ScreenManager`，Godot 无关的
纯 C++17。复刻了确认的管理器级 API：`register_screen` / `request_screen(name, push_history)`
（对应 `FUN_0014cc78`->`FUN_0016c4c0`->`FUN_0016c8c8`）/ `go_back()`（组合 PopHistory +
`request_screen(name,false)`，是**结构性假设**而非单一调用点证实）/ `reset()`
（对应 `FUN_0016b460`，只清历史栈，不清注册表）。历史栈容量硬编码为证据支持的 12 槽；
第 13 次 push 返回 `HistoryFull` 而不是编造原版的溢出行为。

**未复刻的部分**（明确排除，避免编造）：屏幕内部渲染/子控件树、`FUN_0016eef8` 的实例级状态机、
`FUN_0016fa80` 的完整更新逻辑、`FUN_00249a40` 的行为绑定、原始手柄输入到 message 的映射。

单元测试：`src/tests/screen_manager_tests.cpp`，9 个用例覆盖：空状态、未注册名查找、已证实的
TitleScreen->MumboJumboScreen->New Main Menu 链路历史压栈、Back 导航、空历史 Back、清空当前屏幕、
历史容量上限、Reset 保留注册表。`ctest --preset native-debug` / `--preset godot-debug`：7/7 通过
（含既有 5 项测试，无回归）。

## Godot 可操作界面

新场景 `godot/scenes/frontend_menu_observer.tscn` + `godot/scripts/frontend_menu_observer.gd`，
桥接类 `port::godot_bridge::FrontEndMenuObserver`（`src/platform/godot_bridge/
frontend_menu_observer.hpp/.cpp`，已在 `register_types.cpp` 注册）。原 `main.tscn`、
`main_loop_observer.tscn` 与既有 API 全部保留，未修改。

界面显示：当前屏幕名、历史栈深度/容量、历史栈内容、已注册屏幕列表（高亮当前项）、证据脚注。
按钮：
- **Press Start（confirmed: TitleScreen -> New Main Menu）**：调用证实边；在 New Main Menu 时
  再次按下会切回 TitleScreen（同样走 push_history=true 路径，用于演示闭环，不代表原版有此反向
  调用点）。
- **Construct MumboJumboScreen（registered sibling; edge unconfirmed）**：明确标注为"注册但边
  未证实"，防止把它误当作验证过的游戏行为。
- **Back**：弹出历史栈并以 push_history=false 请求该屏幕。
- **Reset**：重新调用 `initialize()`，回到 TitleScreen、空历史。

```powershell
# 首次或源码改变后编译并打开
./tools/run_frontend_menu_observer.ps1 -Build
# 使用当前 DLL 打开（可交互）
./tools/run_frontend_menu_observer.ps1
# 无界面验收
./tools/run_frontend_menu_observer.ps1 -Smoke
# 截图（需要绝对路径）
./local/tools/godot-4.3/Godot_v4.3-stable_win64_console.exe --path godot res://scenes/frontend_menu_observer.tscn --quit-after 300 -- --frontend-menu-capture=<绝对路径.png>
```

## 验收记录

- `cmake --preset native-debug` / `--build` / `ctest`：7/7 通过（新增 `screen_manager` 测试用例）。
- `cmake --preset godot-debug` / `--build --parallel 4` / `ctest`：7/7 通过。
- Godot headless `--editor --import`：无 ERROR。
- `res://scenes/frontend_menu_observer.tscn -- --frontend-menu-smoke`：
  `FRONTEND_MENU_OBSERVER_SMOKE_OK` 通过（初始屏幕、证实边、Back 导航、占位同级切换、Reset）。
- 既有 `main_loop_observer.tscn -- --observer-smoke` 与原 `--smoke-test`：均无回归，分别打印
  `MAIN_LOOP_OBSERVER_SMOKE_OK` 与 `PORT_SMOKE_OK: 42`。
- 实际渲染截图：`local/frontend-menu-observer.png`（初始 TitleScreen 状态）与
  `local/frontend-menu-observer-after-press-start.png`（按下 Press Start 后，当前屏幕为
  New Main Menu，历史栈显示 TitleScreen），均为 1380x760，人工检查文字/布局正确，这两张图片
  在 `.gitignore` 的 `local/` 下，不随仓库分发。
- 工具版本：Godot 4.3.stable.official.77dcf97d8；GCC 13.2.0；Python 3.14.4；CMake 3.29.2。
- 未做：PCSX2/原版行为对照、任何 Ghidra 注解/名称/边界修改、Git 提交（由用户自行管理）。

## 9. 2026-10-12 补充：原版源码文件路径（`indep\src\fmenus.cpp` 等）

对 ELF 做 `.cpp`/`.hpp` 字符串搜索（`__FILE__` 宏残留）直接恢复出了原版源码目录结构，其中
`indep\src\fmenus.cpp` 和 `indep\src\frontendscreens\{selectcar,racesetup,options,
joystickconfiguration}.cpp` 与本文件追踪的函数高度吻合（附近字符串 `MENUITEM_%d`、
`tHelpMenu`、`tPopupMenu` 等与 TASK-0012 在资源文件里独立发现的 `UwEMENUITEM_*` 吻合）。
详见新文档 [docs/ORIGINAL_SOURCE_TREE.md](ORIGINAL_SOURCE_TREE.md)，这是项目级证据，不止
覆盖前端菜单。尚未把具体函数地址钉死到具体文件/行号，这是明确的未解决项。

## 8. 2026-10-12 补充：中央 GAMEFLOW_STATE_* 状态机（重大结构性发现，修正此前假设）

用户指出本机已有一个成熟、独立验证过的同类 HP2 PS2 资源工具项目
（`D:\dev\eagl-godot\map_tools_ps2`），并要求"不要自己猜，结合 Ghidra MCP 完整逆向出原菜单
源码"。先用那个项目的贴图解码参考实现修复了 TASK-0012 的贴图解码 bug（见 docs/ASSET_PIPELINE.md
§4.4），然后回到 ELF 一侧继续推进，有了以下重大发现：

### 8.1 `FUN_0016fa80` 此前的假设是错的

TASK-0011 第一轮只读了 `FUN_0016fa80` 开头约 176 字节的 listing，结合 Ghidra `functions get`
报告的 `end_address: 0x001e3dbf`（约 465KB），**错误地假设**它是 New Main Menu 自己的
超大更新/tick 函数。本轮对它做了完整 `decompile`，实际函数体只有 **312 字节**，干净完整。
`functions get` 报告的超大 end_address 很可能是 Ghidra 函数边界记录的一个记账异常（不代表
真实可达代码），不是此前反复提到的"边界合并错误"。**这个假设已被推翻**，不要再使用。

### 8.2 真正发现：原版开发者的 `GAMEFLOW_STATE_*` 枚举

反编译 `FUN_0016fa80` 的唯一调用者 `FUN_00168830`（地址 0x00168830）发现它在收尾时调用
`FUN_001680d8(4, 0)`——而 `0x001680d8`/`0x001680e8` 正是 TASK-0010 主循环记录里已知的
"State Module::State, Phase::StateDispatch" 调用目标（`0x001680e8` 出现在每帧调度里）！
完整反编译两者后确认这是一套标准的**状态机调度器**：

```c
// 0x001680d8：请求切换到某个 gameflow 状态
void HP2_RequestGameFlowState(int state, int param) {
    DAT_00330548 = state;   // 目标状态
    DAT_0033054c = param;
}

// 0x001680e8：每帧调度泵（已知的主循环调用目标）
void HP2_PumpGameFlowStateMachine(void) {
    // 若当前状态 != 目标状态：调用当前状态 exit -> 切换 -> 调用新状态 enter -> 循环直到稳定
    // 若相等：调用当前状态的 update（每帧一次）
}
```

状态表位于 ELF `0x002faea4`，每条记录 16 字节：`+0x0=enter函数指针, +0x4=exit函数指针,
+0x8=update函数指针, +0xC=指向原版 ASCII 枚举名字符串的指针`。读取 +0xC 处的字符串，直接拿到了
**原版开发者写的真实枚举常量名**（不是猜的，是 ELF 里现成的 ASCII 字符串）：

| 行 | 原版名字（ELF 字符串，逐字节确认） | enter | exit | update |
|---|---|---|---|---|
| 0 | `GAMEFLOW_STATE_LOADING_FRONTEND` | 0x00168650（空函数，仅 `jr ra`） | 0 | 0 |
| 1 | `GAMEFLOW_STATE_UNLOADING_FRONTEND` | 探测到但未确认为真函数入口 | 0x001687a8（已反编译） | 探测到但未确认 |
| 2 | `GAMEFLOW_STATE_IN_FRONTEND` | 0x00168830（已反编译，见下） | 0 | 0 |
| 3 | `GAMEFLOW_STATE_LOADING_TRACK_COMMON` | 0 | 0 | 0 |
| 4 | `GAMEFLOW_STATE_LOADING_TRACK_SPECIFIC` | 0x00168db8（未反编译） | 0 | 0 |
| 5 | `GAMEFLOW_STATE_RACING` | 0x001690a0 | 0x00169168 | 0x00169160（均未反编译） |
| 6 | （未读字符串） | 0 | 0 | 0 |
| 7 | `GAMEFLOW_STATE_UNLOADING_TRACK_COMMON`（表顺序推断，未逐条重新核对） | 0x00169288 | 0 | 0 |
| 8 | `GAMEFLOW_STATE_EXIT_DEMO_DISC` | 0x00168ea0 | 0 | 0 |
| 9 | （+0xC 字段为 0，无名字） | 0x00169490 | 0x00169528 | 0x001694a0 |

### 8.3 与 TASK-0011 已确认证据的连接

- **启动序列**（TASK-0010 已记录但当时不知道含义）：主循环启动区 `0x0012f048` 调用
  `HP2_RequestGameFlowState(1, 0)`，即请求 `GAMEFLOW_STATE_UNLOADING_FRONTEND`——这正是
  TASK-0010 `gp_a54c==0` 分支里标注为"调用 0x001680D8"但未解读参数含义的那个调用点。
- **`GAMEFLOW_STATE_UNLOADING_FRONTEND` 的 exit 回调**（`0x001687a8`，已完整反编译）在收尾时
  调用 `FUN_0016f950()`——正是 TASK-0011 已确认的"attract 序列门控，在特定条件下请求切换到
  New Main Menu"的那个函数！这第一次把顶层 gameflow 状态机和 TASK-0011 已经吃透的前端屏幕
  管理器证据链连起来了。
- **`GAMEFLOW_STATE_IN_FRONTEND` 的 enter 回调**（`0x00168830`，已完整反编译）调用
  `FUN_0016fa80()`（见 §8.1 修正），然后**无条件**调用 `HP2_RequestGameFlowState(4, 0)`，
  即立刻请求切换到 `GAMEFLOW_STATE_LOADING_TRACK_SPECIFIC`。换句话说，在这个调度器里，
  "进入 IN_FRONTEND" 并不是一个会持续停留、逐帧更新的状态（它的 update 槽位是空的），而是
  一个一闪而过的中转状态，进入后立刻请求加载赛道。这一点出乎意料，记为未解决的假设：
  可能 PS2 版主菜单背景是一段实时渲染的 attract 赛车画面（不是预渲染视频），需要先把赛道
  加载起来；也可能还有另一条路径驱动真正"玩家在菜单里浏览"的持续更新逻辑，但本轮没有找到。

### 8.4 诚实评估

这是一次重大、有真实 Ghidra 证据支撑的结构性发现（尤其是拿到了原版真实枚举名字符串），但
**"完整逆向出原菜单源码"这个目标远未达成**：仍有 7 个未反编译的 gameflow 状态处理函数、
`FUN_00168830` 剩余 7 个未追踪的子系统调用、`FUN_0016fa80` 剩余 4 个未追踪的调用，以及
最关键的——**真正驱动"玩家在 New Main Menu 里选择菜单项"的持续更新/输入循环仍未定位**。
不要把本节内容当作完整的菜单系统还原。

## 未解决问题与下一步

**2026-10-12 更新**：第 1 项（`FUN_0016fa80` 边界/用途）已在 §8 解决并修正——它不是超大更新
函数，是一个 312 字节的一次性 gameflow 转场清理函数。以下列表已更新为反映 §8 发现后的真实
优先级：

1. **最高优先级**：真正驱动"玩家持续停留在 New Main Menu 并浏览/选择菜单项"的逐帧更新与输入
   循环仍未定位。已知 `GAMEFLOW_STATE_IN_FRONTEND` 的 update 槽位为空，`enter`
   （`FUN_00168830`）进入后立即请求切换到 `GAMEFLOW_STATE_LOADING_TRACK_SPECIFIC`，与"持续
   停留在菜单"的直觉矛盾（§8.3 最后一段）；需要先搞清楚这个矛盾才能继续。
2. 7 个已定位但未反编译的 gameflow 状态 enter/exit/update 函数
   （0x00168698、0x00168718、0x00168db8、0x001690a0、0x00169160、0x00169168、0x00169288、
   0x00168ea0、0x00169490、0x001694a0、0x00169528，见 §8.2 表格），以及 `FUN_00168830`/
   `FUN_0016fa80` 各自剩余的未追踪子系统调用。
3. "New Main Menu" 自身的类构造/析构分发函数未定位。
4. 原始手柄输入（`JOY_EVENT_MENU_*` 常量）到 `FUN_0016eef8` message==1 的派发路径未追踪。
5. `MumboJumboScreen`、`FUN_0016f950`（attract 序列门控）与 New Main Menu/TitleScreen 之间的
   真实导航顺序未证实；`FUN_0016c7b8` 的历史栈"空白项"维护语义未完全解析。
6. `FUN_00249a40` 绑定的 GRID/MASTER_BP_OBJECT/FADING_BP_OBJECT 行为对象系统身份未解析。
7. 子控件列表（`screen+8`）、`FUN_0016ad38`/`FUN_0016a858`/`FUN_0016a940` 等子节点生命周期函数
   均未反编译，是菜单"内容"下一步工作的前提。

延续提示：读取本文件（尤其是 §8）+ `reverse/symbols.csv` 中 `frontend` 子系统行，先解开第 1 项
的矛盾（反编译 0x00168db8、0x001690a0 等状态，看 RACING/LOADING_TRACK_* 这几个状态之间是否还
会绕回 IN_FRONTEND，或者"浏览菜单"根本不是由这套 gameflow 状态机驱动的，而是在某个状态的
update 回调内部用自己的子状态机实现的）。
