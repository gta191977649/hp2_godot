# TASK-0013：原版前端界面布局格式（真实渲染，不是调试界面）

完成日期：2026-10-12，Australia/Sydney。

用户指出此前交付的是**调试/检查界面**，要求的是**像 PS2 原版一样真正渲染出来的前端界面**。
本任务解析了 `FRONTEND/FRONTB.BUN` 里原版自己的界面布局记录和位图字体表，在 Godot 里按原版
坐标、缩放、颜色、贴图和字形逐个绘制出来。**Mode Select 主菜单、Options、Configuration、
Title Screen 等 41 个原版界面现在可以真实渲染**，不是手工搭的 UI。

## 1. 屏幕/控件布局格式（chunk 0x80010200）

`FRONTB.BUN` 顶层的 `0x80010200` 容器就是整个前端的界面列表：

```text
0x80010200  界面列表容器
├── 0x00010213  20 字节表头
└── 0x80010201 × 41   每个 = 一个界面
    ├── 0x00010203  40 字节界面表头（+0x18 u16 宽、+0x1a u16 高 → 全部是 640x480）
    ├── 0x00010210  144 字节  贴图四边形控件
    └── 0x00010212  104–124 字节  文本控件（尾部带变长字符串，所以长度不固定）
```

两种控件记录共用的前半部分（偏移均为记录内偏移）：

| 偏移 | 类型 | 含义 | 判定依据 |
|---|---|---|---|
| 0x00 | u32 ×2 | 运行时指针占位，文件里恒为 0 | 观察 |
| 0x08 | u32 | **控件名字哈希** | 与 ELF 代码里的立即数逐字匹配（见 §4） |
| 0x0c | u32 | 类哈希（同类控件共用，如 0x457755c9） | 跨控件取值一致 |
| 0x10 | char[24] | 控件名（`MENUITEM_1`、`GRID`、`PRESS_START_1`…） | 直接可读 |
| 0x28 | u8 | 类型（0x01 文本 / 0x02 四边形） | 与 chunk id 一一对应 |
| 0x29 | u8 | 可见/启用标志 | 取值 0/1 |
| 0x2a | u8 | **层级**（数值越大越靠后，背景网格是 0x08/0x0a，菜单文字是 0x00） | 按降序绘制后画面正确 |
| 0x30 | float | X（左上角，640×480 空间） | 见下方验证 |
| 0x34 | float | Y | 同上 |
| 0x38 | float | X 缩放（乘贴图原生宽度） | 见下方验证 |
| 0x3c | float | Y 缩放 | 同上 |
| 0x50 | u32 | **贴图哈希**（TexturePack 条目哈希；文本控件是字体贴图） | 统计相关性，见下 |
| 0x58 (四边形) / 0x5c (文本) | u8×4 | 颜色 R,G,B,A（PS2 风格 alpha，0x80 = 不透明） | 0x80808080 中性、0xaa000000 半透明黑等符合观察 |

文本控件额外字段：`0x54` u32 = 作者设定的文本缓冲长度；`0x58` u32 = 对齐（**2 = 左对齐，
4 = 居中**）；`0x64` 起 = 变长默认字符串。

### 字段判定不是猜的

- **贴图哈希在 0x50**：对全部 41 个界面的所有控件做了逐偏移统计——把每个 4 字节对齐位置当作
  u32，去跟真实 TexturePack 条目哈希表比对。`0x50` 在 0x10210 记录里命中 507/1015，在 0x10212
  记录里命中 206/534，其余所有偏移的命中率都远低于此（次高的 0x08 只有 79/1015，那是名字哈希
  偶然碰撞）。
- **缩放是乘以贴图原生尺寸**：`TITLE_FULLSCREEN` 引用 `SPLASH_SCREEN`（512×256），缩放
  (1.25, 1.88) → 640 × 481，正好等于界面表头声明的 640×480。`GRID`（512×384）缩放 1.25 →
  640×480，同样吻合。`WHITE16X16`（16×16）缩放 (32.0, 1.70) → 512×27 的高亮条，与菜单行高一致。
- **对齐 2/4**：`PRESS_START_1` 的 x = 320（屏幕正中）、右侧数据列 `LINE_1..8` 的 x = 494，
  这些都是 align=4；而左侧菜单项 `MENUITEM_1..8` 的 x = 153 是 align=2。
- **层级降序绘制**：按 `0x2a` 降序画出来，网格/蓝图背景正确位于菜单文字之后；用其他顺序
  （文件顺序、反向文件顺序）都会把背景盖在文字上。

## 2. 位图字体格式（chunk 0x00015010）

```text
0x00  u32  字体贴图哈希（对应 FONT_IMPACTSMALL / FONT_IMPACTBIG / BUTTONSOUT …）
0x04  u32  字形数量
0x08  u32  kerning 对数量
0x0c  u32  行高（全部为 28）
0x10  u32  kerning 表的字节偏移（含 chunk 自身 8 字节头）
0x14  起   字形记录，每条 32 字节：
           u16 字符码, u16 标志, float u, v, w, h, advance（单位都是图集像素）
之后       kerning 表，每条 8 字节
```

尺寸自洽校验：`FONT_IMPACTSMALL` 130 字形 × 32 + 20 + 8 = 4188 = 表头里的 kerning 偏移值；
chunk 总长 10412 − 4180 = 6232 = 779 对 × 8 字节，分毫不差。`FONT_EVENTNUMBERS`（10 字形、
50 对、348）同样完全吻合。字形码是**稀疏**的（Impact 系列里没有 `$`），必须按字符码查表，
不能按下标。

已定位的全部字体：

| 字体 | 所在 bundle | 字形数 | 备注 |
|---|---|---|---|
| FONT_IMPACTSMALL / IMPACTBIG / IMPACTNORMAL | FRONTEND/FRONTB.BUN | 130 | 菜单正文/标题 |
| FONT_EVENTNUMBERS | FRONTEND/FRONTB.BUN | 10 | 只有数字 |
| FONT_A / FONT_HUD / FONT_STATS | GLOBAL/GLOBALB.BUN | 130 | |
| **BUTTONSOUT** | GLOBAL/GLOBALB.BUN | 29 | **手柄按键图标字体**——Configuration 界面里的 ○/×/△/R3/摇杆图标就是用它当字符画出来的 |
| FONT_GAUGE / FONT_POSITION | GLOBAL/INGAMEB.BUN | 12 / 9 | 游戏内 HUD |

## 3. 实现

- `src/game_core/frontend/fmenus.hpp/.cpp` — Godot 无关的原生解析器（`parse_screens`、
  `parse_fonts`）。文件名取自 ELF 里嵌入的原版源码路径 `indep\src\fmenus.cpp`
  （见 docs/ORIGINAL_SOURCE_TREE.md）。
- `src/platform/godot_bridge/ps2_frontend_layout.{hpp,cpp}` — `Ps2FrontEndLayout` 桥接类，
  把界面、控件、字体暴露给 GDScript。
- `src/platform/godot_bridge/ps2_texture_loader.*` — 新增 `load_texture_by_hash()`，跨
  bundle 按哈希解析贴图，带 archive 和贴图两级缓存。
- `godot/scenes/frontend.tscn` + `godot/scripts/frontend.gd` — 单个 `_draw()` 即时模式渲染：
  四边形用 `draw_texture_rect`，文本逐字形用 `draw_texture_rect_region` 从字体图集取 UV。
  左右方向键切换界面，Tab 隐藏调试信息行，Esc 退出。

所有数据都在**运行时**从本机配置的 ZZDATA 读取（路径解析见 `src/platform/ps2_asset_root.hpp`），
仓库里不含任何预导出的图片或布局文件。

## 4. 与 ELF 代码的交叉验证

控件名字哈希（记录 +0x08）在 ELF 里是**字面立即数**：
`FUN_0016b478` 里出现的 `0xe0105663`、`0xe0105664`、`0xe0105665` 正好等于布局数据里
`POPUP_1`/`POPUP_2`/`POPUP_3` 的名字哈希；同一函数里的 `0xab671c9a` 等于 `ANIMATING_MENUITEM`。
这把 TASK-0011 从 Ghidra 得到的代码侧证据和本任务的数据侧格式**直接对上了**——同一套哈希，
代码按名字哈希找控件，数据按名字哈希声明控件。另外 TASK-0011 里 `FUN_00249a40` 绑定的三个名字
`GRID` / `MASTER_BP_OBJECT` / `FADING_BP_OBJECT`，在布局数据里确实就是三个真实控件（见
screen 0/2/40 的控件列表），当时标注的"用途未完全解析"现在可以确认是**按名字绑定界面上的
背景网格与蓝图图层控件**。

## 5. 验收

```powershell
# 交互运行（方向键换界面）
./tools/run_frontend.ps1
# 无界面自检：遍历全部 41 个界面，解析每个控件的贴图/字体
./tools/run_frontend.ps1 -Smoke
```

- `FRONTEND_SMOKE_OK: screens=41 resolved=1524 missing=25` — 41 个界面、1524 个控件资源成功
  解析，全前端只有 25 个未解析（分散在少数引用了本会话未覆盖 bundle 的控件上）。
- 已目视确认且截图存档：Mode Select 主菜单（Championship / Speed / Options + HP2 标志）、
  Options（Audio…Language Select）、Configuration（手柄按键图标 + 蓝图网格）、Title Screen
  （HP2 启动画面 + PRESS START）。
- 原生 CTest 8/8 通过；`frontend_menu_observer`、`main_loop_observer`、原 `--smoke-test`
  三个既有场景均无回归。
- 独立可执行文件：`out/windows-debug/port.exe`（+ `port.pck` + `ps2_port.dll`），导出时临时把
  主场景指向 `frontend.tscn`，导出后已还原 `project.godot`。

## 6. 未解决问题

1. **Kerning 未应用**：kerning 表已解析（8 字节/对），但配对语义没完全确定（观察到的条目第二个
   u16 多为 0），字距会比原版宽 1–2 像素。
2. **没有交互与动画**：界面按作者设定的静止状态绘制，高亮、滚动文字、淡入淡出、菜单项选中
   动画都还没驱动；`ANIMATING_MENUITEM` 这类模板控件原版会在运行时移动复用，这里按原始坐标
   画在原处（有些因此停在屏幕外，属于预期）。
3. **文本是作者占位串**：显示的是布局文件里的默认字符串（`MenuItem 1`、`EACOPYRIGHTSTRING-…`、
   `This text will scroll sideways…`）。运行时真实文案来自 `LANGUAGES/FE_*.BIN` 本地化表，
   该格式尚未解析。主菜单/Options 等界面的文案恰好在布局里就是最终文本，所以看起来是对的。
4. 25 个控件的贴图哈希在本会话搜索的 bundle 列表里找不到。
5. 界面与 TASK-0011 的屏幕名（`"New Main Menu"` 等）之间的映射仍未确定：布局数据里没有界面名
   字符串，代码侧按名字查找，两者的对应关系（按索引？按某张表？）还没找到。按内容判断
   screen 0 = Mode Select 主菜单，但这是**内容推断**，不是证实的映射。
