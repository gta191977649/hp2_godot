# TASK-0016：Hot Pursuit / World Racing 两个模式选择界面，以及背景影片是否循环

完成日期：2026-10-13，Australia/Sydney。二进制 `SLUS_203.62`，SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`；Ghidra 12.0.4，
`r5900:LE:32:default`，GhidraMCP **只读**，未做任何注解/改名/边界修改。

## 1. 这两个界面是什么

主菜单（`NEW_MAIN_MENU`，FRONTB.BUN screen 0）三项分别是 **Hot Pursuit / World Racing /
Options**。本地化之后：

| 主菜单项 | 跳转到 | 类名 | 文档哈希 | 布局界面 |
|---|---|---|---|---|
| Hot Pursuit | 警察开启的模式列表 | `Cops On Mode Select` | 0x5B35F533 | screen 1 |
| World Racing | 警察关闭的模式列表 | `Cops Off Mode Select` | 0x033E7771 | screen 2 |

两个界面的菜单项（本地化后）：

```
Hot Pursuit (screen 1)              World Racing (screen 2)
  Quick Race                          Quick Race
  You're The Cop                      Challenge
  Challenge                           Championship   <- 锦标赛
  Ultimate Racer   <- 锦标赛
```

**"Ultimate Racer" 和 "Championship" 是同一个东西在两条线上的名字**：它们分别是这两个界面的
锦标赛入口，激活后都请求同一个界面 `"View Event Tree"`，区别只在于写进去的赛事树 id
（警察开 = 9，警察关 = 8）。

## 2. 逐项行为（`FUN_002174A0` / `FUN_00217AF8`，message 1）

事件结构 `{+0x00 控件名哈希, +0x04 手柄号, +0x08 结果, +0x0C 音效 id}`。

### Hot Pursuit（`FUN_002174A0`）

| 项 | 名字哈希 | 行为 |
|---|---|---|
| Quick Race | 0x52E039F3 | **仅手柄 0**；`FUN_00216C18(instance+0x274)` |
| You're The Cop | 0x52E039F4 | `DAT_002FC06C = 1`；退场动画 → `"ChallengeBeACop"` |
| Challenge | 0x52E039F5 | `DAT_0031AB90 = 1`、`DAT_002FC06C = 0`；退场动画 → `"Challenge"` |
| Ultimate Racer | 0x52E039F6 | **仅手柄 0**；`DAT_0030C77C = 9`、`DAT_0030C784 = 1`、`FUN_001F5D50(0x002FC1B4, 1)`、`DAT_002FC1C0 = 0`；退场动画 → `"View Event Tree"` |
| （MENUITEM_5） | 0x52E039F7 | 退场动画 → `"Options"`。**该布局没有第五项控件，这条分支在 screen 1 上不可达** |

### World Racing（`FUN_00217AF8`）

| 项 | 名字哈希 | 行为 |
|---|---|---|
| Quick Race | 0x52E039F3 | **仅手柄 0**；`FUN_00216C18` |
| Challenge | 0x52E039F4 | `DAT_0031AB90 = 1`、`DAT_002FC06C = 0`；退场动画 → `"Challenge"` |
| Championship | 0x52E039F5 | **仅手柄 0**；`DAT_0030C784 = 1`、`DAT_0030C77C = 8`、`FUN_001F5D50(0x002FC1B4, 0)`、`DAT_002FC1C0 = 0`；退场动画 → `"View Event Tree"` |
| （MENUITEM_4） | 0x52E039F6 | 退场动画 → `"Options"`，同样不可达 |

**手柄门禁是原版行为，不是我加的**：Quick Race 和锦标赛这两项检查事件 +0x04，不等于 0 时把
事件 +0x0C 的音效 id **写成 0** 然后直接返回——既不换界面也不出声，连 `MenuWrong` 都不放。
复刻时逐字照搬。

## 3. 退场动画（`FUN_00217020` / `FUN_002176D8`）

这两个界面**从不直接调用 RequestScreen**。每次激活都是：

1. 从名为 `tExitScreenAnimation` 的池（字符串 0x002BC528）分配 0x1C 字节；
2. `FUN_002490D8(对象, 文档, 目标类名, 1)` 构造；
3. 存进 instance+0x270；
4. `FUN_00248C60` 让 `MI_UNDERSCORE` 以 `gp-0x6AC8` = **650 px/s** 开始移动。

关键在于两个分发函数开头都有 `if (instance+0x270 == 0)` 这道闸：只要退场动画还在，**所有消息
都改送 `FUN_00249128`**，界面在动画期间是锁住的，动画跑完才真正请求目标界面。截图里菜单项整体
向右滑出就是这个。

## 4. 高亮（`FUN_00217380` / `FUN_002179F0`，message 4）

两边结构完全一样：先 `FUN_002488D0(instance+0x23C, 控件名哈希)` 让幽灵重新瞄准（**在 switch
之前**，所以即使表里没有这一项，幽灵也会跟着动），然后查表得到遮罩控件和本地化字符串 id：

| 项 | 遮罩 | Hot Pursuit 串 id | World Racing 串 id |
|---|---|---|---|
| MENUITEM_1 | `MI_1_MASK` 0x6E34E350 | 0x2721 | 0x2716 |
| MENUITEM_2 | `MI_2_MASK` 0x708A0BF1 | 0x271E | 0x2717 |
| MENUITEM_3 | `MI_3_MASK` 0x72DF3492 | 0x271C | 0x2718 |
| MENUITEM_4 | `MI_4_MASK` 0x75345D33 | 0x271D | 0x2719 |
| MENUITEM_5 | `MI_5_MASK` 0x778985D4 | 0x2719 | — |

五个遮罩哈希都用恢复出来的 `StringHash_002534A8` 对 `MI_<n>_MASK` 验算过，逐位相等。
之后用 `FUN_00144280` 取遮罩包围盒，把 `MI_UNDERSCORE` 放到 `y1 - 3`、`MI_OVERSCORE` 放到
`y0 - 6`（`gp-0x3EF0` = -3，`gp-0x3EF4` = 6），再把串 id 交给滚动文字、用 `FUN_0016E270`
切换一个可见性组。

## 5. Quick Race 没有实现（而且不该在前端实现）

`FUN_00216C18` 不打开任何前端界面：它把赛事树 id 设成 2，读主菜单写下的 `DAT_002FC070`
（警察开/关），据此写满一整块比赛配置（`DAT_0031A8CC` 取 7 或 3、`DAT_0031AAF4` 取 4 或 0、
`DAT_0031AB14` 取 2 或 0，以及 `DAT_0031AAF8/AAFC/AB00/AB04/AB08` 全取该标志），再经
`FUN_00153420`/`FUN_00187A68` 登记并交给加载器。这不是前端的活，移植里抛
`hp2::NotImplemented`，不做近似。

## 6. 背景影片：**应该循环播放**（你问的那条）

per-frame 链路是 `FUN_0016CCA8`（前端管理器每帧更新）→ `FUN_0016D028`：

```c
if (manager->movie_0x1CC != 0) {
    if (gp[-0x53B0] != 0) { StopMovie(manager); return; }
    if (FUN_002380A8(/*player*/ 0x002FB308, /*dt*/ gp[-0x4C8C]) != 1)
        FUN_00237AD8(0x002FB308, manager->path_0x1E4);   // 用缓存的路径重新打开
}
```

反汇编是明确的：`jal 0x002380a8` 之后 `xori v0,v0,1` + `beq v0,zero,skip`——**只有返回值恰好
等于 1 才跳过重开**。`FUN_00237AD8` 就是"打开影片"，重开等于从头播。**所以前端背景影片是靠
"解码步进一旦不返回 1 就重新打开同一个文件"来循环的，没有任何一次性播放的分支。**

两条佐证：

- `FUN_0016CE30`（设置影片）在被要求设置**同一个路径**时会提前返回，不重启播放——说明重进同
  一个界面不会让影片从头开始，循环只由上面那条每帧路径负责。
- `FUN_0016D0A0`（停止）清掉 `+0x1CC..+0x1E0` 但**故意保留** `+0x1E4` 的路径缓存。

每个界面各自要哪部影片（`FUN_0016CE30` 的实参，原版字符串逐字）：

| 界面 | 影片 |
|---|---|
| New Main Menu | `MOVIES\mainmenu_movie.AVI` |
| Cops On Mode Select | `MOVIES\hotpursuit_movie.AVI` |
| Cops Off Mode Select | `MOVIES\worldracing_ movie.AVI`（路径里的空格是**原版自带**的） |
| Options | `MOVIES\options_movie.AVI` |

**残留不确定性（明说）**：`FUN_002380A8` 有多条返回 false 的路径，其中包括一条"距上次读取超过
20 个单位就补流并本帧返回 false"的分支，本会话**没有逐条分类**哪一条对应"播完一遍"。能确定的
是：没有一次性播放分支，重开路径就是循环机制。要把"一遍播完"的精确判据钉死，需要继续拆
`FUN_002380A8` 的流式缓冲状态机。

## 7. 顺带的一处修正：幽灵脉冲周期是 4 秒不是 2 秒

TASK-0015 把主菜单幽灵的循环周期设成了 2.0 秒。证据表明那是**管理器级**对象的周期：
`FUN_0016FBB0` 在 `manager+0x1B8` 上把 +0x1C 改写成 2.0，而那个对象绑的是
`GLOBAL_ANIMATING_MENUITEM`、文档是 `LOADING_TRACK`。**界面自己的**幽灵——主菜单
`FUN_00216460` 的 +0x1A8、两个模式选择 `FUN_00216E80`/`FUN_00217538` 的 +0x23C——都只调
`FUN_00248870` 而**不覆写 +0x1C**，所以保持默认 `gp-0x3AC8` = **4.0 秒**。已改正。

同时也补上了 TASK-0015 记为"找不到驱动者"的那一条：驱动者是
`FUN_0016CCA8` → `FUN_0016FCD8` → `FUN_00248970`。

## 8. 背景动画（`FUN_00249AE8`，部分恢复）

`GlobalBackgroundAnimation` 每帧：相位累加器 +0x78 按帧增量推进、到 `gp-0x676C` = **2π** 回绕；
蓝图对象在**头 1 秒淡入**（alpha 0→128）、沿路径漂移、在**最后 1 秒淡出**，整个周期
`gp-0x3A94` = **50 秒**，到点由 `FUN_00249888` 换下一个。淡入淡出包络和 50 秒周期是恢复出来
的；**路径曲线本身**走引擎通用路径动画器（`FUN_001DB000`/`FUN_001DAB38`），曲线数据未解出，
所以没有实现，也没有编一条。

## 9. 实现与验收

- `src/game_core/frontend/frontendscreens/modeselect.{hpp,cpp}` —— 两个界面的 message 1 与
  message 4。原版源文件名**未恢复**（只恢复到池名 `CopsOnModeSelect`/`CopsOffModeSelect` 与
  类名），文件头里已写明这一点。
- `src/platform/godot_bridge/ps2_frontend_layout.cpp` —— 激活分发新增 screen 1/2；激活事件
  改为按引用传入，手柄门禁才能按原版写回音效字段；两个界面启用界面级幽灵。
- `src/tests/frontend_modeselect_tests.cpp` —— 7 组断言：注册表一致性、两边逐项行为与全局量、
  手柄门禁（含"被拒时音效归零、未被拒时保留"）、Quick Race 抛未实现、两张高亮表、
  选中横线落点。

```powershell
ctest --preset native-debug          # 新增 frontend_modeselect 通过
./tools/run_frontend.ps1 -Screen 1   # Hot Pursuit，方向键可操作
./tools/run_frontend.ps1 -Screen 2   # World Racing
```

截图存档在 `local/shots/`（不入库）：主菜单（本地化后 Hot Pursuit / World Racing / Options）、
按下 Hot Pursuit 后进入 screen 1、在 screen 1 下移两次选中 Challenge（上下两条选中横线跟着
跳到该项）、选中 Ultimate Racer 后菜单项整体向右滑出（退场动画）。

**未做**：PCSX2 对照。本任务所有记录停在 `Decompiled`，没有一条标 `Verified`。

## 10. 补充（同日，用户提供原机截图后）：`View Event Tree` 已定位

用户给出了一张原版截图：标题 **Ultimate Racer**，一棵带编号星形节点的赛事树，右侧
`Event 15 / Status: Passed - 1st / Reward: Alpine Trail`，底部 `European Invitational
Tournament`。这正是上一节里"没逆向"的那个界面，现在定位到了。

### 界面身份

| 项 | 值 |
|---|---|
| 类名 | `View Event Tree`（字符串 0x002BA718，**第三份拷贝**；另两份 0x002B2FE0/0x002BC560 只用于比较和请求） |
| 文档名哈希 | **0x147F8A59 = FRONTB.BUN screen 39** |
| 类分发 | `FUN_001FB218` |
| 实例处理 | `FUN_001FAF60` |
| 实例大小 / 池名 | 0x860 字节，池名 **`tEventTreeView`**（原版字面量，0x002BA8F8） |
| 注册点 | 0x001FB860，走另一个包装 `FUN_0016AD88` |

上一节说"类名是运行时 gp 指针所以解不出"——那条**只对后处理界面那一批成立**
（`gp-0x44D8..-0x44A4` 存着 `Points Rewards`/`Race Results`/`Tourney Standings`… 这些静态
初始化的指针）。赛事树这条是 `addiu a1,a1,-0x58E8` 的**字面地址**，当时没找到是因为我只搜了
那两份已知拷贝的指针，漏了第三份字符串本身。

screen 39 的控件名与截图逐项对得上：`EVENT_NUMBER`（星里的大号数字，占位 "88"）、
`SMALL_EVENTNUMBER`（"Event 1245"）、`OVERLAY_STATUS_HEADING`(strid 0x101D0 "Status:")、
`OVERLAY_STATUSTITLE`、`NONAME`(strid 0x101D7 "Reward:")、`OVERLAY_REWARDTITLE`、
`EVENT_TITLE_BAR`（底部横幅）、`EVENT_PANEL`（选中框）、`HLINE`/`VLINE`（树的连线）、
`TRACK_LOGO`/`FADING_LOGO_1`、`EVENT_TREE`、`SPEED_TREE_BG`。

### 交互（`FUN_001FAF60`，状态机 instance+0x74）

**状态 0（浏览树）**：

| 消息 | 来源 | 行为 | 音效 |
|---|---|---|---|
| 1 | 选中 | `FUN_001F5D28` + `FUN_001FAC78` | 0x15 `TreeZoomInOut` |
| 6 / 7 | 左/右（`FUN_00169D98`） | `FUN_001FA9C8(-1)` / `(+1)` | 0x14 `TreeLeftRight` |
| 8 / 9 | 上/下（`FUN_0016A458`） | `FUN_001FA9C8(+2)` / `(-2)` | 0x13 `TreeUpDown` |
| 0x0F | L1 | 刷新全部节点（`FUN_001F8300`） | — |
| 0x11 | | `FUN_001F5D28`，不缩放 | — |

移动失败（没换到新节点）时把事件 +0x0C 的音效 id 写 0，**不出声**。

**状态 2（已选中某个赛事）**：0x11 缩回（`FUN_001FAD20`，音效 0x15）；消息 10 开始比赛
（`FUN_001FAE40`，音效 0x18），但**先检查节点记录的 +0x3E 字节，小于 1 就只放
`MenuWrong`(8)**——这就是锁住的赛事点不进去的原因。其余消息转给详情子菜单 `FUN_001F6B70`。

### 光标移动（`FUN_001FA9C8`）

当前节点在 instance+0x18。方向 **-1/+1 走节点 +0x1C 的链**，**+2 走 +0x20**，**-2 走 +0x28**
——左右和上下走图的两条不同轴。移动成功时把旧节点 +0x0C 写 1.0f、新节点写 **2.0f**，
**这就是截图里被选中的 15 号星比旁边 9/10/16 大一圈的原因**（节点缩放）。随后更新面板、
把滚动文字设成节点记录 +0xB0 的串 id、刷新 Status 与 Reward 两块浮层。

### 还缺什么才能真正跑起来

树的**节点图来自玩家档案**：构造函数是 `FUN_001F96B8(instance, document, FUN_00185568(0x002FC050))`，
0x002FC050 是存档/进度对象。所以移植端即使把 screen 39 画出来（现在已经能画，见
`local/shots/screen39-eventtree.png`，显示的是作者占位串 "This Will Change" / "Event 1245" /
"This is The Title of the Event Phil is still poo poo."），**没有档案数据就没有节点、连线和
状态文字**。这是实现它之前必须先解决的依赖，不是前端渲染问题。

标题 "Ultimate Racer" 是运行时写上去的（screen 39 的标题控件 strid 为 0）。按分支推测对应
0x272D / 0x272A 两个串，但**没有证实**，列为未解决项。

## 11. Open questions

1. ~~`View Event Tree` 界面本身没有逆向~~ —— **已定位，见 §10**。剩下的是：节点图的档案数据源
   （0x002FC050）、运行时标题文本、以及 `FUN_001F6B70` 详情子菜单。
2. 赛事树节点记录只恢复到字段用途（+0x14 数据指针、+0x1C/+0x20/+0x28 三条邻接链、
   +0x3E 解锁位、+0xB0 串 id），整体布局未定；详情子菜单 `FUN_001F6B70` 未反编译。
3. `FUN_001F5D50(0x002FC1B4, 0/1)` 选择哪棵赛事树数据，未反编译。
4. `FUN_002380A8` 各条 false 路径的分类（见 §6）。
5. 背景动画的路径曲线数据（见 §8）。
6. 幽灵颜色取自 `gp-0x3F08`，镜像里的初值是 0x00000080，即 RGB (0x80, 0, 0) 纯红。这是
   **初值**——该全局是 `lw` 读取的，本会话没有确认运行期是否有人改写它。
