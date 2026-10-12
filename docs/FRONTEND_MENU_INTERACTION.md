# TASK-0014：前端菜单的交互、选中与音效（从 ELF 逆向，非设计）

完成日期：2026-10-12，Australia/Sydney。二进制 `SLUS_203.62`，SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`；Ghidra 12.0.4，
`r5900:LE:32:default`，GhidraMCP 只读（127.0.0.1:8080），**未做任何注解/改名/边界修改**。

本任务把 TASK-0013 渲染出来的原版界面变成**可操作**的：菜单项列表、选中移动、高亮配色、
激活回调、菜单音效事件，全部来自反编译的原版代码，不是重新设计的 UI。命名规则见
[AGENT.md](../AGENT.md)：恢复到原版字面量的标识符逐字使用，没恢复到的一律带 ELF 地址。

## 1. 关键发现一览

| 发现 | 证据 | 影响 |
|---|---|---|
| **字符串哈希函数**（`FUN_002534A8`）：`h=-1; h=h*0x21+c` | E-0014-07 | 能把名字和哈希互转，下面几乎所有发现都靠它 |
| **菜单项 = 屏幕上名为 `MENUITEM_0..15` 的 type-1 控件** | E-0014-04 | 菜单结构不再靠猜，直接从布局数据构造 |
| **JOY_EVENT_\* 枚举值↔名字表**（ELF 0x002FA2D0） | E-0014-01/02 | 输入语义确定，键盘映射有据可依 |
| **高亮=橙色 (0x80,0x40,0x00)，禁用=深灰 (0x18,0x18,0x18)** | E-0014-09 | 来自 ELF 已初始化数据，不是取色 |
| **菜单音效事件名表**（0x002FB0A8） | E-0014-11 | `MenuUpDown`/`MenuSelect`/`MenuWrong`… 逐字原版名 |
| **屏幕自己的名字哈希在 0x10203 头 +0x08**，18/41 个名字已破解 | E-0014-16/17 | **screen 0 = `NEW_MAIN_MENU`**，TASK-0013 的内容推断被证实 |
| `HIGHLIGHT_BAR` = 0x27C5F4AB | E-0014-06 | 从哈希反解出的原版控件名 |

## 2. 字符串哈希（E-0014-07）

`FUN_002534A8` 反编译后就三行：

```c
int StringHash(const char* s) { int h = -1; while (*s) h = h * 0x21 + *s++; return h; }
```

**验证不是看着像**：`FUN_0016B478` 里硬编码的四个立即数 `0xE0105663 / 0xE0105664 /
0xE0105665 / 0xAB671C9A`，用这个函数算 `POPUP_1` / `POPUP_2` / `POPUP_3` /
`ANIMATING_MENUITEM` 逐位相等。另外它能重算出 FRONTB.BUN 里 1549 个控件名字哈希中的 **1531
个**；对不上的 18 个**全部**是存储名字恰好 23 字符的记录——也就是被 24 字节名字字段截断的那些，
反过来说明原版是**先算哈希再截断存盘**。

## 3. 菜单是怎么构造出来的（E-0014-04）

`FUN_0016A858`（菜单组首次使用）→ `FUN_00169F78`（建表）：

```c
// FUN_00169F78(list, group, callback, initial_index, "MENUITEM_%d")
list->next = list->prev = list;                 // 环形哨兵
list->screen = group->screen;  list->pad = group->pad;
for (int i = 0; i < 0x10; ++i) {
    sprintf(buf, name_format, i);
    if (FindWidget(list->screen, StringHash(buf), /*type=*/1))   // 控件存在才加
        append(new MenuThing(list, StringHash(buf), i));
}
SetCurrentIndex(list, initial_index);
list->highlight_bar = FindWidget(list->screen, 0x27C5F4AB, /*type=*/2);
```

- 格式串 `"MENUITEM_%d"` 在 ELF 0x002B2760，被 `FUN_0016A330` 和 `FUN_00169F78` 两处 sprintf 引用，**逐字**。
- 上限 **0..15**（两处循环都是 `i < 0x10`）。
- `type=1` 是文本控件、`type=2` 是四边形控件——正好是布局记录 +0x28 的取值，这也是**代码侧与
  数据侧第一次直接对上**（见 §6）。
- `0x27C5F4AB` 的名字是**反解出来的**：用上面的哈希函数试候选名，`HIGHLIGHT_BAR` 逐位相等，
  其他候选都不碰撞。（FRONTB.BUN 的 41 个界面里没有声明这个控件，所以这些界面的高亮条指针是空的。）

## 4. 选中移动与高亮配色

`FUN_0016A5E0`（下一项）/ `FUN_0016A6C8`（上一项）：环形链表前进/后退，碰到哨兵就绕回表头/表尾，
并**跳过 enabled 位为 0 的项**，绕回起点就放弃。是否绕回与跳过由全局 `gp-0x533C`
（= 0x003305B4）决定，**零售镜像里该值为 1**，所以原版菜单是会绕圈的（E-0014-10）。

高亮本身是**改控件颜色**，不是画一个选择框。`FUN_00169BD0` / `FUN_00169CB0` 按
`state & 3` 选色（state 在菜单项 +0x08，bit0 = 可用、bit1 = 高亮）：

| state & 3 | 含义 | 颜色来源 |
|---|---|---|
| 1 | 可用、未高亮 | 控件自己的作者设定色（`FUN_00169818` 从控件 +0x5C 快照下来的） |
| 3 | 可用、高亮 | `gp-0x5330/-0x532C/-0x5328`，并用 `lui a1,0x8000` 把 alpha 强制成 0x80 |
| 0 或 2 | 不可用 | `gp-0x5324/-0x5320/-0x531C` |

gp 的值不是猜的：入口 0x00100008 是 `lui a0,0x33; addiu a0,a0,0x58f0; move gp,a0`，
所以 **gp = 0x003358F0**（Ghidra 也在该地址放了 `_gp_1` 标签）。代入读出来：

- 高亮 = **(0x80, 0x40, 0x00) + alpha 0x80** → 橙色，正是 HP2 菜单的选中色。
- 不可用 = **(0x18, 0x18, 0x18)** → 深灰。

`FUN_00140CA0` 写色时**保留 alpha 字节**只换低 24 位，并把控件 +0x2b 盖成 7（脏标记）。
`state&3 == 0` 和 `== 2` 两个分支加载的是同一组全局——这是在反汇编 0x00169C1C / 0x00169C38
两处逐条核对过的，不是反编译器把分支合并了。

## 5. 输入：JOY_EVENT_\* 枚举（E-0014-01/02）

ELF 0x002FA2D0 是一张 `{ uint32 value, const char* name }` 表，8 字节一条，名字指针指向
0x002B0218 起的连续字符串块。**枚举值和名字是表里直接读出来的，不需要假设顺序**：

```
0 JOY_EVENT_NULL          6 JOY_EVENT_SCROLL_RIGHT    12 JOY_EVENT_MENU_HELP
1 JOY_EVENT_TYPE_CHANGED  7 JOY_EVENT_START           13..16 MENU_L1/L2/R1/R2
2 JOY_EVENT_EXIT_DEMO_DISC 8 JOY_EVENT_SELECT         17 JOY_EVENT_MENU_SQUARE
3 JOY_EVENT_SCROLL_UP     9 JOY_EVENT_MENU_SELECT     18..21 MENU_ANALOG_LEFT/RIGHT/UP/DOWN
4 JOY_EVENT_SCROLL_DOWN   10 JOY_EVENT_MENU_CANCEL    22 JOY_EVENT_SKIP_DEMO_MODE
5 JOY_EVENT_SCROLL_LEFT   11 JOY_EVENT_MENU_BACK      23 ..._MAIN_MENU_SELECT_TRANSMISSION
```

**独立交叉验证**：`FUN_0016A458` 把 3 和 0x14 归成一组、4 和 0x15 归成一组；`FUN_00169D98`
把 5 和 0x12 归成一组、6 和 0x13 归成一组。只有在上面这张表下，这四组才分别等于
"上/摇杆上"、"下/摇杆下"、"左/摇杆左"、"右/摇杆右"。两份证据互相印证。

事件分发链（E-0014-03）：

```
FUN_0016CAB0  管理器级（先给两个 tPopupMenu，再给当前界面）
  └─ FUN_0016B140  遍历界面 +0x08 的菜单组链表
       └─ FUN_0016AA00  菜单组：pad 过滤 + START/SELECT/L1/R1 四个组级消息
            └─ FUN_0016A458  菜单表：上/下移动（消息 8/9，音效 3/4）
                 ├─ FUN_0016A5E0 / FUN_0016A6C8
                 └─ FUN_00169D98  当前项：左右调节（消息 6/7）、START/MENU_SELECT 激活
                                   （消息 1，音效 5 MenuSelect）、MENU_SQUARE（消息 10）
```

回调消息号（原版只有数字，没有恢复到名字，所以代码里的枚举名只带数字）：
2 刷新项、4 获得高亮、5 失去高亮、8 上移、9 下移、0x0B 菜单创建（可改初始下标）、
0x0D START、0x0E SELECT、0x0F L1、0x10 R1、1 激活、6/7 左右调节、0x0A 方块键、
0x15 弹窗项激活、0x16/0x17 弹窗上一项/下一项、0x18 界面未处理的事件。

事件结构体 16 字节：`{+0x00 名字哈希或界面句柄, +0x04 pad, +0x08 结果/否决, +0x0C 音效 id}`，
回调返回后调用方用 +0x0C 播音效。

## 6. 菜单音效（E-0014-11）

`FUN_0016E038(id)` 查 0x002FB0A8 起、16 字节一条的表
`{u32 id, u32 flag, const char* name, u32 voice}`，voice 字段正好落在 `0x002FB0A4 + id*0x10`。
名字是原版 bSound 事件名，**逐字**：

| id | 名字 | 用在哪 |
|---|---|---|
| 1 / 2 | `MenuLeftRight` | 弹窗上一项 / 下一项 |
| 3 / 4 | `MenuUpDown` | 菜单表上移 / 下移 |
| 5 | `MenuSelect` | 菜单项激活 |
| 6 | `MenuCancel` | |
| 7 | `MenuStart` | |
| 8 / 9 | `MenuWrong` | |
| 10..21 | `MenuTrackRightLeft`、`MenuCarRight/Left/Colour`、`MenuCarDisplayOn/Active/Off`、`MenuCarSelect`、`TreeUpDown`、`TreeLeftRight`、`TreeZoomInOut` | 选车/事件树 |

`FUN_0016E038` 里还有一个"同一个 voice 在 34.0 个单位内重复触发就丢弃"的防抖；**34.0 的单位
（帧？毫秒？）没有恢复**，所以移植里**没有**复刻这个防抖——编一个单位就是编行为。

音效的**选择逻辑**是逆向出来的并且实现了；**发声**本身是 PS2 bSound 硬件的事
（`FUN_00273618`/`FUN_00274080` + 未解析的音频库），按 AGENTS.md 第 7 条放在平台契约后面：
`PlayMenuSound_0016E038` 把 id 和原版事件名交给注入的 sink，Godot 侧把名字显示出来，不合成音频。

## 7. 代码侧与数据侧终于对上了（E-0014-05）

`FUN_00143D50(screen, name_hash, type)` 遍历界面的控件侵入式链表，比较**节点 +0x08**（名字哈希）
和**节点 +0x28**（类型）。这两个偏移正是 TASK-0013 从 FRONTB.BUN 记录里读出来的名字哈希和类型
字段；`FUN_00140CA0` 写的 **+0x5C** 正是 TASK-0013 定位的文本控件颜色；它盖的 **+0x2b** 正好在
TASK-0013 的 type/visible/layer（+0x28/+0x29/+0x2a）后面一个字节。

也就是说：**磁盘上的 144 字节记录就是运行时控件节点本身**，+0x00/+0x04 的"文件里恒为 0 的指针
占位"就是链表的 next/prev。TASK-0013 当时只能说"统计相关性支持这个字段图"，现在是代码直接按这些
偏移读写。

## 8. 界面名字（E-0014-16/17）

`FUN_0016FBB0` 用 `StringHash("LOADING_TRACK")` 当作"文档句柄"传给 `FUN_00143D50`——说明界面是
**按名字哈希**寻址的。回到布局数据：每个界面 0x10203 头的 **+0x08 是一个全界面唯一的 u32**，
而且 screen 5/6 和 8/9 的值正好相差 1（哈希函数的性质：只差最后一个字符的名字，哈希也差 1）。

把候选名字用恢复出来的哈希函数算一遍去撞这 41 个值，**撞中即证实**（32 位精确相等）：

| # | 名字 | # | 名字 | # | 名字 |
|---|---|---|---|---|---|
| 0 | `NEW_MAIN_MENU` | 14 | `POPUP` | 28 | `AUDIO_OPTIONS` |
| 4 | `SELECT_CAR` | 16 | `CREDITS` | 29 | `USERNAME_OPTIONS` |
| 7 | `SELECT_CAR_DIALOG` | 22 | `TITLE_SCREEN` | 30 | `LANGUAGE_OPTIONS` |
| 10 | `SELECT_TRACK` | 24 | `OPTIONS` | 35 | `LOADSAVE` |
| 11 | `CHALLENGE` | 26 | `DISPLAY_OPTIONS` | 36 | `BLANK_SCREEN` |
| 12 | `RACE_SETUP` | 13 | `JOYSTICK_CONFIG` | 40 | `CREDITS_SCREEN` |

**这解决了 TASK-0013 的第 5 个未解决项**：screen 0 确实是主菜单，而且原版就叫
`NEW_MAIN_MENU`——与 TASK-0011 从 ELF 里确认的屏幕类名字符串 `"New Main Menu"`
（0x002B2F00）对应。另外 TASK-0013 当时**靠肉眼**认出的 Title Screen / Options /
Controller Configuration 三个界面，现在分别被哈希证实为 `TITLE_SCREEN`(22)、`OPTIONS`(24)、
`JOYSTICK_CONFIG`(13)——这是一次完全独立的交叉验证。

剩下 23 个界面的名字还没撞出来，哈希值记录在这里供后续继续：

```
1 5b35f533   2 033e7771   3 e55f1297   5 18dc8e52   6 18dc8e53   8 6344da01
9 6344da02  15 74d2dfa2  17 a744ba7b  18 fd297fd4  19 f1eed4cd  20 492e86e9
21 dd4197c9  23 adb8e59e  25 28891a22  27 a4031ef6  31 8bd5060b  32 ddf09124
33 7ab428c1  34 95b2b627  37 9829bfab  38 bec7cd92  39 147f8a59
```

（5/6 和 8/9 两对只差最后一个字符，大概率是 `..._1`/`..._2` 或 `..._P1`/`..._P2` 这类。）

## 9. 实现

- `src/game_core/frontend/fmenus.hpp/.cpp` —— 新增菜单运行时。文件名沿用原版
  `..\indep\src\fmenus.cpp`。类/函数名按 AGENT.md R2：`MenuThing_00169740`、
  `MenuList_00169F78`、`MenuGroup_0016AA00`、`Sub_00169D98`、`StringHash_002534A8` 等，
  **没有恢复到原名的一律把 ELF 地址写进标识符**，解释性措辞只出现在注释里。
- `src/game_core/input/joyevents.hpp/.cpp` —— `JOY_EVENT_*` 枚举，**逐字**。
- `src/game_core/not_implemented.hpp` —— `hp2::NotImplemented` + `HP2_NOT_IMPLEMENTED(addr, what)`。
- `src/platform/godot_bridge/ps2_frontend_layout.*` —— 新增 `menu_open` / `menu_joy_event` /
  `menu_state` / `get_widget_colours` / `find_screen_by_name` / `joy_event_name` /
  `probe_not_implemented`。菜单直接改布局记录里的颜色，渲染端每帧重读颜色即可。
- `godot/scripts/frontend.gd` —— 键盘映射到 `JOY_EVENT_*`，交互、状态行、未实现提示。

### 键盘映射（PC 调试用）

| 键 | JOY_EVENT | | 键 | JOY_EVENT |
|---|---|---|---|---|
| ↑ ↓ ← → | `SCROLL_UP/DOWN/LEFT/RIGHT` | | Q / E | `MENU_L1` / `MENU_R1` |
| W A S D | `MENU_ANALOG_UP/LEFT/DOWN/RIGHT` | | 1 / 3 | `MENU_L2` / `MENU_R2` |
| Enter | `START` | | V | `MENU_SQUARE` |
| Space / Z | `MENU_SELECT` | | H / F1 | `MENU_HELP` |
| X | `MENU_CANCEL` | | \\ | `SELECT` |
| Backspace / C | `MENU_BACK` | | | |

非游戏键：PgUp/PgDn（或 `[` `]`）换界面、Tab 开关信息行、F5 重建、F8/F9/F10 触发未实现路径
（高亮动画 / 本地化文本 / 弹窗）、Esc 退出。

## 10. 明确未实现（会抛 `hp2::NotImplemented`，不会悄悄降级）

| 路径 | 为什么没做 |
|---|---|
| `Sub_0016BAC0` 弹窗/帮助菜单输入 | 逻辑已反编译（见记录文件），但弹窗对象的构造与 +0x24/+0x28/+0x2C/+0x30 处理器表没恢复，填不出来 |
| `Sub_0016FBB0` "FMenus Highlight Animation" 每帧运动 | 构造函数反编译了（绑 `GLOBAL_ANIMATING_MENUITEM`、存 2.0f），但**没有任何恢复出来的代码说明它每帧做什么**——插值、时长、缓动、目标全未知 |
| `localised_text` | `LANGUAGES/FE_*.BIN` 格式未解析；界面上显示的仍是布局文件里的作者占位串 |

按 AGENT.md R3，这些不返回默认值、不做近似、不静默 no-op。Godot 侧把异常信息显示在界面上。

## 11. 验收

```powershell
ctest --preset native-debug          # 9/9，含新增 fmenus_menu（16 组断言）
ctest --preset godot-debug           # 9/9
./tools/run_frontend.ps1 -MenuSmoke  # FRONTEND_MENU_SMOKE_OK
./tools/run_frontend.ps1 -Smoke      # FRONTEND_SMOKE_OK: screens=41 resolved=1524 missing=25
./tools/run_frontend.ps1 -ScreenName NEW_MAIN_MENU   # 交互运行
```

`-MenuSmoke` 在无界面模式下跑完整条链路：按名字解析 `NEW_MAIN_MENU` → 0；从该界面自己的
`MENUITEM` 控件建出 3 项菜单；`JOY_EVENT_SCROLL_DOWN` 移动选中并请求 `MenuUpDown` 音效；
`JOY_EVENT_MENU_ANALOG_UP` 移回；`JOY_EVENT_MENU_SELECT` 触发激活并请求 `MenuSelect`；
确认有控件拿到了恢复出来的高亮色；三个未实现路径都抛出 `hp2::NotImplemented`。

截图（`local/shots/`，不入库）：主菜单初始状态 Championship 为橙色高亮；连按两次下键后
Options 变橙、Championship 回到灰色。

## 12. Open questions

1. 原始手柄输入 → `JOY_EVENT_*` 的生成（ELF 有 `GenerateJoyEvents()`、`Joystick`、`EventNode`、
   `EventNodeSlotPool` 字符串，但生成函数没定位）。
2. `FUN_0016E038` 里 34.0 的单位。
3. 弹窗对象的构造与处理器表（见 §10）。
4. 高亮动画每帧做什么（见 §10）。
5. 剩下 23 个界面的名字（§8 列了哈希）。
6. `HIGHLIGHT_BAR` 控件在哪个 bundle 里声明（FRONTB.BUN 的 41 个界面都没有）。
7. 菜单组是怎么挂到界面上的（界面 +0x08 链表的构造点没定位；移植侧目前由端口代码直接为一个界面
   建一个组）。
8. `FUN_0016FBB0` 解析的 0x120E827C 控件名未反解出来。


## TASK-0015 implementation update — 2026-10-12

Earlier missing-animation claims are superseded for NEW_MAIN_MENU: pulse, bars and live
rendering now run every frame. Global 0016FBB0 bindings remain missing (F8 probes that boundary).
Main actions request registered documents. Options direct actions and exit-before-request
are implemented except Display, whose settings snapshot sources are missing. NameEntry
resolves to screen 29; class identity is retained. Exit is recovered cubic Hermite (E-0015-21).
Main Back is forbidden; Options allows it outside exit animation. No original verification;
clock, sine quantization, localisation and destination internals remain limitations.
See TASK-0015 and PROGRESS for exact checks and continuation.
