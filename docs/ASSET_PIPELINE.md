# TASK-0012：PS2 原始资源管线（eLab 容器格式 / .tpk 贴图格式）

**2026-10-12 更新**：用户指出本机已有一个成熟、经过验证的同类项目
`D:\dev\eagl-godot\map_tools_ps2`（同一用户的既有 HP2 PS2 资源导出工具链，覆盖赛道网格/贴图/
音频/环境等），其 `src/map_tools_ps2/textures.py` 是已验证可用的贴图解码参考实现。对照后发现
本任务最初的像素解码少做了一步：**贴图数据池（chunk4）的起始偏移需要向上对齐到 0x80 字节**
（`data_base = align(data_chunk.data_offset, 0x80)`），本任务最初的实现直接用了未对齐的原始
偏移，导致每张解码出的贴图都带有一层细密噪点（§4.4 原先记录为"未解决"）。按参考实现移植
对齐逻辑 + 完整的 Modelulator legacy PS2 RWBuffer 解码路径（而不是之前简化的 8bpp-only 路径）
后，问题完全消失，并且原来因为是 4bpp 索引色而被跳过的贴图（`LOGO_NFS`、`CARLOGO_*` 等）现在
也能正确解码——`FRONTEND/FRONTB.BUN` 的 365 条贴图记录里成功解码数从 210 提升到 357。
下文已更新为反映修复后的状态；原始"未解决"记录保留在 §4.4 末尾作为过程记录，不要删除。

完成日期：2026-10-12，Australia/Sydney。范围是**容器格式与 P8/RGBA 贴图像素格式的只读解析**，
已能从用户本机授权的 PS2 光盘资源目录中解码出大量真实原版 UI/HUD 贴图。**菜单布局/控件树的
语义解析（哪张贴图放在哪个菜单、具体像素坐标）尚未完成**，不得把当前结果当作"完整还原的菜单
画面"。4bpp 贴图、DXT 压缩贴图、带 swizzle 标志的 32 位贴图尚未实现解码，明确拒绝而不是猜测。

## 背景与用户要求

用户提供了本机一份授权的 PS2 原版光盘资源目录 `D:\ps2_game\GAME\ZZDATA`，并要求：

1. 不要在代码里硬编码这个路径，在仓库里提供一个全局统一的配置入口。
2. 逆向工作应当能够直接把原版 UI 绘制出来，且与原版 PS2 行为一致。
3. 项目文件名、函数名结构也要体现出被逆向对象的真实结构（而不是随意命名）。

## 1. 全局资源配置入口

新增 `tools/ps2_assets.py`，是仓库中**唯一**允许解析 ZZDATA 路径的地方；任何需要读取原始资源的
脚本都必须 `import ps2_assets` 并调用 `ps2_assets.zzdata()` / `ps2_assets.path(*parts)`，禁止在
别处硬编码路径。解析顺序：

1. 环境变量 `HP2_PS2_ASSET_ROOT`。
2. `config/ps2_assets.local.json`（已加入 `.gitignore`，机器相关，不进仓库）。

仓库内提交的是 `config/ps2_assets.example.json`（占位路径 + 使用说明），真实路径只存在于本机的
`config/ps2_assets.local.json` 里。本机已写入 `{"root": "D:/ps2_game/GAME/ZZDATA"}`。

## 2. 容器格式：eLab 嵌套 chunk

证据来源：`FRONTEND/FRONTB.BUN` 内嵌字符串字面量 `Global\Pipeline\eLabFrontEnd.bin`
（ELF 地址无关，这是资源文件内的调试路径字符串），确认引擎工具链代号为 **eLab**（EA 内部管线
工具，与 bWare 中间件同属该年代 EA 工具体系，`bWare/inc/bWare.hpp` 调试路径字符串在同一资源区
域内多次出现，两者关系未进一步确认）。

通过手工字节分析 + 完整文件消费校验（见下）确认的格式假设：

```c
struct Chunk {
    uint32_le tag;    // bit 0x80000000 置位 = 容器（后续 size 字节是嵌套 Chunk 序列）
                       // 清零 = 叶子（后续 size 字节是不透明数据：名字字符串/浮点数/贴图字节等）
                       // (tag & 0x7fffffff) 在同一文件内按前序遍历逐一自增，像是文件级 chunk 序号
    uint32_le size;    // 紧随其后的 payload 字节数
    uint8 data[size];
};
```

实现：`tools/asset_extractor/elab_chunks.py`，核心函数 `parse_chunks()`/`parse_chunks_top()`，
纯只读、不修改任何文件。验证方式：对完整文件递归解析后检查 `consumed_bytes == total_bytes`
（即整份文件**精确**被消费完，没有剩余/越界字节）——这是比"能跑不报错"更强的证据，因为错误的
chunk 边界几乎不可能让随机数据恰好消费到文件末尾。

| 文件 | 大小 (bytes) | fully_consumed | 顶层 chunk 数 | 总 chunk 数 |
|---|---|---|---|---|
| FRONTEND/FRONTB.BUN | 14,385,132 | true | 21 | 1,760 |
| GLOBAL/HUDTEX.BIN | 1,540,664 | true | 3 | 9 |

两个来源不同（前端管线 bundle vs. 贴图包）的文件用**同一套通用解析器**都能精确消费完，是这个
假设成立的强证据（E-0012-01）。

## 3. 真实原版文本（证据，非编造）

`elab_chunks.py` 对叶子 chunk 做字符串扫描后，在 `FRONTEND/FRONTB.BUN` 的尾部约 300KB 区域
（偏移 ~14,080,000 起）发现大量干净、规则排布的资源/控件名字符串，其中包含开发者遗留的调试/
占位文本（证明这是未经清理的原始资源，不是我方编造）：

- `"You need at least one menu item"`、`"Phil is a loser, so I have to make this fake menu item"`、
  `"This is The Title of the Event Phil is still poo poo. Greg will always be the greatest"`、
  `"DummyMenuItem"`、`"Silly menuitem"` 等开发期占位/玩笑文本。
- 真实 UI 文本：`"PRESS START"`（出现两次）、`"Championship"`、`"Options"`、`"Credits"`、
  `"Controller"`、`"Language Select"`、`"Player Options"`、`"Setup"`、`"Race Type"`、
  `"Next Track:"`、`"Select Laps"`、`"Look Back"`、`"Menu Music"`、`"Sound Effects"` 等。
- 大量 `UwE` 前缀的控件类型名：`UwEMENUITEM_n`、`UwEANIMATING_MENUITEM`、`UwEPOPUP_TITLE`、
  `UwEBACKGROUND_FADE`、`UwESCROLLING_MENU_CLIP_REG`、`UwECONTROLLER_BITMAP` 等。

证据 ID：E-0012-02。**这些字符串尚未与具体菜单/页面建立确认的对应关系**（例如哪一组
`UwEMENUITEM_*` 属于 "New Main Menu"）；与 TASK-0011（`docs/FRONTEND_MENU.md`）里从 ELF 代码
反编译得到的 "New Main Menu" / "TitleScreen" 证据是**两条独立证据链**，尚未交叉关联。

## 4. 贴图格式：HP2 的 TPK 容器 + PS2 GS P8/RGBA 像素格式

### 4.1 Chunk 结构（引用并校验过的社区参考实现）

`GLOBAL/HUDTEX.BIN`、`FRONTEND/TSELECT.BIN` 以及 `FRONTEND/FRONTB.BUN` 内嵌的一个 1249 万字节
子容器，顶层都带有一个 `0xB0300001` 容器 chunk，其子节点为 `0x30300002`（包名/来源路径叶子）、
`0x30300003`（贴图头数组）、`0x30300004`（共享像素/调色板数据池）。这组 tag 值与开源工具
[`xan1242/xnfstpktool`](https://github.com/xan1242/xnfstpktool)（`TPKTool/TPKTool.h` 中的
`TPK_HP2_MAIN_CHUNKID`/`TPK_HP2_CHUNK2/3/4_CHUNKID` 常量）**逐字节完全一致**，该工具源码里显式
标注了 "HP2 chunks" 和 `TPKTOOL_READINGMODE_HP2`，确认这是为本游戏（SLUS-203.62 对应的 PC/PS2
NFS:HP2）准备的读取模式。

贴图头结构体（该工具的 `AncientTextureInfo`，164 字节）：

```c
struct AncientTextureInfo {
    uint32_t Unknown[2];          // +0x00，含义未解析
    char DebugName[24];            // +0x08
    uint32_t NameHash;              // +0x20
    uint16_t Width;                 // +0x24
    uint16_t Height;                // +0x26
    uint8_t ImageCompressionType;   // +0x28  0x08=P8, 0x20=RGBA, 0x22/24/26=DXT1/3/5, 其余含 0x4 位=4bpp 变体
    uint8_t RenderingOrder;         // +0x29
    uint8_t unk1..unk5, AlphaUsageType; // +0x2A..+0x2F，含义未解析
    int32_t ImagePlacement;         // +0x30  相对于 chunk4 数据池起始的像素数据偏移
    int32_t PalettePlacement;       // +0x34  相对于 chunk4 数据池起始的调色板偏移
    int32_t ImageSize;              // +0x38
    int32_t PaletteSize;            // +0x3C
    uint32_t unk6, unk7;             // +0x40..+0x47，含义未解析
    uint8_t ShiftWidth, ShiftHeight; // +0x48,+0x49
    uint8_t UnkData[0xB];            // +0x4A..+0x54，含义未解析
    uint8_t swizzled;                // +0x55  非零=像素数据需要 PS2 GS 块 swizzle 还原
    uint8_t RestOfData[0x4E];        // +0x56..+0xA3，含义未解析
};
```

### 4.2 本仓库独立做过的校验（不是盲信引用来源）

以 `GLOBAL/HUDTEX.BIN` 的第一条贴图记录 `"VIPERGTS_TACH"` 为基准，逐字节核对：`DebugName`
在 +0x08 处精确匹配；`NameHash` 在 +0x20 处为 `0x08a0f7f2`；`Width`/`Height` 在 +0x24/+0x26
处均为 128；`ImageCompressionType` 在 +0x28 处为 `0x08`（P8）；`ImageSize`（+0x38）=0x4000=16384
恰好等于 128×128×1；`PaletteSize`（+0x3C）=0x400=1024 恰好等于 256×4（标准 P8 调色板大小）。
贴图头数组总大小 14,596 字节除以 164 字节/条，结果**恰好**是整数 89——与 HUDTEX.BIN 实际包含
89 张贴图完全吻合（E-0012-03）。对 `FRONTEND/FRONTB.BUN` 内的 365 条贴图头做同样的批量校验
（`image_placement`/`palette_placement` 严格前后相接、无重叠、无越界），365 条记录全部通过。

### 4.3 像素格式解码（已改为移植本机既有项目的权威实现）

实现：`tools/asset_extractor/hp2_tpk_textures.py`。像素解码部分**不是本仓库独立推导的**，而是
逐函数照搬自用户本机既有项目 `D:\dev\eagl-godot\map_tools_ps2\src\map_tools_ps2\textures.py`
（`decode_indexed_texture`/`_decode_modelulator_indexed_texture`/
`_legacy_ps2_rw_plan`/`_legacy_ps2_rw_buffer`/`_legacy_ps2_swizzle_psmt4/8/16/32`/
`_legacy_ps2_block_address`/`_decode_palette`/`_decode_ps2_alpha` 等，逻辑完全未改动）。该项目
自己的文档 `D:\dev\eagl-godot\docs\hp2_ps2_texture_decoding.md` 又说明其 swizzle/RWBuffer 算法
源自 Modelulator（知名 NFS 模组工具）的 `eTexturePack.lua`/`PSX2.lua`。本节以下内容是历史记录
（本任务最初自行实现时参考的公开社区工具），保留供过程追溯；**当前代码已不再使用下面这套
简化算法**，改用上述移植版本——原因和验证过程见 §4.4。

- **历史记录（已不再使用）——P8（8 位索引色）早期实现**：
  - 像素字节在文件里是按 PS2 GS PSMT8 块/列 swizzle 方式存放的（当 `swizzled=1`，HUDTEX.BIN
    里检查过的贴图均为 1）。采用的还原算法来自一个 Silent Hill 2（PS2）PIC.IMG 提取工具的
    `UnSwizzle8` 函数（[gist](https://gist.github.com/infval/b23380076d77341ccc379a62ef9f23b3)，
    该工具又注明引用自 2012 年 XeNTaX 论坛帖）；用随机数据做了"正向 swizzle 再反向 unswizzle
    还原原始字节"的自洽性测试，往返结果完全一致（E-0012-05，不等于"正确"，只证明我的移植没有
    转写错误）；另有三个相互独立的开源实现（`xnfstpktool` 文档注释、
    [`PS2HomeDeveloper/ps2-tim2-tool`](https://github.com/PS2HomeDeveloper/ps2-tim2-tool)、
    [`leeao/PS2Textures`](https://github.com/leeao/PS2Textures)）都把同一形状的算法列为标准
    8bpp 方法，互相佐证。
  - 调色板（256×RGBA32）需要做 PS2 CSM1 重排（32 色一组，组内 4 条 8 色"条纹"顺序从
    `0,1,2,3` 变为 `0,2,1,3`，且该置换是自逆的）。来源：
    `PS2HomeDeveloper/ps2-tim2-tool` 的 `_unswizzle_clut8`；用 `leeao/PS2Textures` 的
    `unswizzlePalette`（按位操作实现，交换索引的 bit3/bit4）做了独立交叉验证，两种写法在数学
    上等价（同一置换表），确认不是抄错（E-0012-05）。
  - Alpha 通道：PS2 GS 原生 alpha 范围是 0..0x80（不是 0..0xFF），还原公式
    `a_255 = min(round(a_ps2 * 255 / 128), 255)`，同样取自 `ps2-tim2-tool`。
- **RGBA（32 位直接色）**：仅在 `swizzled=0` 时解码（直接按行读取 4 字节一个像素 + 同样的
  alpha 还原）；`swizzled=1` 的 RGBA 贴图目前**明确拒绝解码**（抛错而不是输出错误图像），因为
  32 位 GS 块 swizzle 算法虽然在 `ps2_tim2_tool.py` 的 `_gs_swizzle_32bit` 里能找到，但本次
  会话没有拿到一张已确认 `swizzled=1` 的 RGBA 贴图做校验，不贸然应用。
- 4bpp 索引色（`ImageCompressionType & 0x4`，如 `CARLOGO_*`、`BIG_ARROW_*`）和 DXT1/3/5
  压缩贴图**完全未实现**，运行时会被明确跳过并打印原因，不会被静默忽略或猜测输出。

### 4.4 解码结果（已修复）

**根因与修复**：最初实现里 `data_base` 直接用了 chunk4 的原始 `data_offset`，没有按 0x80 字节
对齐；参照 `D:\dev\eagl-godot\map_tools_ps2` 的 `read_ps2_tpk_bytes` 发现正确做法是
`data_base = align(data_chunk.data_offset, 0x80)`。以 `GLOBAL/HUDTEX.BIN` 为例，chunk4 的原始
偏移是 14712，对齐后是 14720，相差 8 字节——这个固定的小偏移量正好解释了"构图完全正确、但带有
细密规则噪点"的现象（所有索引都系统性地错位了几个字节，而不是整体错乱）。同时把像素解码从
之前简化的"仅 8bpp"路径换成参考实现的完整 Modelulator legacy PS2 RWBuffer 路径（支持 4bpp 和
8bpp，依据 `pixel_storage_mode` 字段的真实类型索引而不是猜测的压缩类型表），并加入参考实现中
"导出用途需要额外纵向翻转一次"的步骤（`frontend_textures.py` 对前端贴图专门这样处理，因为内部
解码路径是按 3D 网格 UV 习惯做的，纵向翻转过一次）。

**修复后验证**：对 `GLOBAL/HUDTEX.BIN`（89 条）与 `FRONTEND/FRONTB.BUN`（365 条）做批量解码，
分别成功 89/89 与 357/365（剩余 8 条是名字/尺寸为空的占位记录，被显式跳过而不是报错）。目视
检查确认完全无伪影：`VIPERGTS_TACH` 转速表能清晰读出 "RPM X1000" 字样和刻度；`DISPLAYSETUP_BG`
是完全清晰的赛道画面（红色跑车、车牌、行道树、护栏、远景云层，全部干净无噪点）；`LOGO_NFS`
清晰显示红色 "NFS" 带拖尾效果的官方标志；此前因 4bpp 被跳过的 `LOGO_NFS`、`CARLOGO_*` 等现在
也能正确解码。证据 ID：E-0012-06。

## 5. 与 TASK-0011 的关系

TASK-0011（`docs/FRONTEND_MENU.md`）从 ELF 代码反编译确认了 `TitleScreen -> New Main Menu`
的跳转边和管理器级 API；本任务（TASK-0012）从资源文件反编译确认了容器/贴图格式并解码出大量
真实原版美术资源。**两条证据链目前相互独立，尚未交叉关联**——换句话说，我们还不知道 "New Main
Menu" 这个屏幕具体使用哪些已解码出的贴图、文本和控件记录。把两者关联起来（即解析
`FUN_0016fa80` 揭示的菜单状态机 + 资源文件里的 `UwEMENUITEM_*` 控件记录,
找出它们与贴图名字/哈希的引用关系）是让 Godot 真正"画出与原版一致的 New Main Menu" 的
下一步，还没有开始。

## 5a. 2026-10-12 补充：原生 C++ 运行时加载器（替代手动导出流程）

用户明确要求："不要手动导出贴图再接到 Godot，逆向出来的代码应该能在运行时直接从原版 PS2
资源加载一切"。因此把 `hp2_tpk_textures.py` 的解码逻辑（对齐修复 + 完整 Modelulator legacy
RWBuffer 路径）逐函数移植成了 Godot 无关的原生 C++：

- `src/game_core/assets/elab_chunks.{hpp,cpp}` — chunk 容器解析器的 C++ 版本。
- `src/game_core/assets/hp2_tpk_texture.{hpp,cpp}` — `Hp2TpkArchive` 类：解析贴图头数组 +
  完整像素/调色板解码（P8/4bpp，含 PS2 legacy RWBuffer swizzle）。**已通过字节级对比测试**
  （`src/tests/hp2_tpk_texture_tests.cpp`，CTest 用例 `hp2_tpk_texture`）：用同一张
  `VIPERGTS_TACH` 贴图，C++ 解码结果与 Python 参考实现逐字节完全一致。
- `src/platform/ps2_asset_root.{hpp,cpp}` — 运行时 ZZDATA 路径解析（与 `tools/ps2_assets.py`
  同样的解析顺序：`HP2_PS2_ASSET_ROOT` 环境变量优先，否则从可执行文件所在目录向上查找
  `config/ps2_assets.local.json`），Godot 无关，供桥接层和原生测试共用。
- `src/platform/godot_bridge/ps2_texture_loader.{hpp,cpp}` — Godot `RefCounted` 包装类
  `Ps2TextureLoader`，暴露 `load_texture(relative_path, texture_name) -> ImageTexture`
  给 GDScript，内部直接调用 `Hp2TpkArchive`，**不经过任何预导出的 PNG 文件**。

`godot/scripts/frontend_menu_observer.gd` 已改为在 `_ready()` 时实例化 `Ps2TextureLoader`
并在运行时直接调用 `load_texture("FRONTEND/FRONTB.BUN", "LOGO_NFS")` 等，不再 `load()`
任何预先导出的贴图资源；之前放在 `godot/assets/extracted/frontend/` 的手动导出 PNG 已删除。
`tools/asset_extractor/hp2_tpk_textures.py`（Python 版）保留作为离线分析/快速查看工具，但
**不再是任何运行时路径的一部分**。

关于"原版是否真的有这段 CPU 解码代码"：对 ELF 做了只读字符串/十六进制搜索（chunk tag 常量
`0xB0300001` 等既不作为代码立即数、也不作为数据字出现在 ELF 里；"TPK"/".tpk"/"eLab" 字符串
也完全不在 ELF 里，只在资源文件里），说明这些 tag 值是纯资源文件数据，原版代码大概率只是按
固定的子节点顺序读取，不靠字面匹配 tag 值分发。更进一步：PS2 的 GS 硬件在采样阶段原生支持
swizzled/索引色贴图寻址，原版代码很可能只是把原始字节配置好 GS 寄存器（TEX0/TBW/PSM/CBP）
后直接 DMA 到显存，从来不需要在 CPU 侧把它转成线性 RGBA——这一步"解码成线性 RGBA" 很可能是
我们这个移植任务本身才需要做的事（因为目标是现代 GPU/Godot，不是真·PS2 硬件），原版未必存在
一一对应的函数。这是推理，不是证实的结论，记为未解决项，见
[docs/ORIGINAL_SOURCE_TREE.md](ORIGINAL_SOURCE_TREE.md) 里 `TexturePack`/`TextureAnimPack`
原版类名的记录（搜索到了类名，但没找到具体解码函数的地址）。

## 6. 已集成到仓库的产物

- `tools/ps2_assets.py`：全局资源路径解析入口（唯一允许访问 ZZDATA 路径的地方）。
- `config/ps2_assets.example.json`（提交）/ `config/ps2_assets.local.json`（本机专用，已忽略）。
- `tools/asset_extractor/elab_chunks.py`：通用 chunk 树解析器 + CLI。
- `tools/asset_extractor/hp2_tpk_textures.py`：HP2 TPK 贴图头解析 + P8/RGBA 解码器 + CLI
  （离线分析用途；不再是运行时路径的一部分，见 §5a）。
- `reverse/assets.csv`：已分析资源文件的 SHA-256 provenance 记录。
- `tools/asset_extractor/output/`（已忽略，不进仓库）：解码出的 PNG 样本、chunk 树 JSON 转储。
- `src/game_core/assets/elab_chunks.{hpp,cpp}` / `hp2_tpk_texture.{hpp,cpp}`：原生运行时解码器
  （见 §5a），已通过 `src/tests/hp2_tpk_texture_tests.cpp` 与 Python 参考实现字节级比对。
- `src/platform/ps2_asset_root.{hpp,cpp}`：运行时 ZZDATA 路径解析。
- `src/platform/godot_bridge/ps2_texture_loader.{hpp,cpp}`：Godot `Ps2TextureLoader` 桥接类。
- `godot/scenes/frontend_menu_observer.tscn` + `godot/scripts/frontend_menu_observer.gd`：
  运行时直接从原版资源加载贴图的可操作菜单闭环 Demo（见 docs/FRONTEND_MENU.md）。

未提交任何原始资源字节、解码出的贴图 PNG 或任何可能含版权内容的文件到 Git——全部落在
`.gitignore` 覆盖的 `tools/asset_extractor/output/` 下。

## 7. 未解决问题与下一步

1. ~~P8/RGBA 解码残留的细密伪影~~ 已解决（§4.4，E-0012-06）。
2. DXT1/3/5 压缩贴图仍未实现（本任务未遇到需要用到的真实样本；4bpp/8bpp 索引色已随 §4.3 的
   移植一起解决）。RGBA（非索引色）贴图解码尚未从参考实现移植（当前 `decode_texture` 只处理
   索引色路径），FRONTB.BUN 里的少量 RGBA 条目（如 `ARROW_L`）会被跳过。
3. `FRONTB.BUN` 尾部的 `UwEMENUITEM_*` 等控件记录的二进制布局（位置、尺寸、关联贴图引用）
   完全未解析——这是"按原版布局摆放贴图"的前提，目前完全没有做。
4. `FRONTEND/TSELECT.BIN`（赛道选择贴图包，2064 万字节）、`GLOBAL/DYNTEX.BIN`、
   `GLOBAL/CHARACTERSA.BIN`、`LANGUAGES/FE_*.BIN`（本地化文本，很可能是菜单文本的权威来源，
   本会话只看了文件头，未解析其记录格式）均未处理。
5. TASK-0011 与 TASK-0012 两条证据链尚未交叉关联（见 §5）——这是当前最高优先级，见下方延续
   提示；用户已明确要求结合 Ghidra MCP 完整逆向原菜单源码，接下来的工作重心转向 ELF 一侧。

延续提示：读取本文件 + `docs/FRONTEND_MENU.md`，若要让 Godot 画出真正贴近原版的 New Main
Menu，建议顺序是：先解析 `LANGUAGES/FE_ENGLISH.BIN` 拿到权威菜单文本（比资源包内零散字符串
更可信，文件头已见到字符串表结构雏形）；再解析 `UwEMENUITEM_*` 等控件记录的二进制布局；最后
解决 4bpp/DXT 贴图解码和残留伪影问题。每一步都应先验证、后使用，不得用占位图像冒充已解码的
原版美术。
