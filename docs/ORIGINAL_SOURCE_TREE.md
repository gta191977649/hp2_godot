# 原版源码目录结构与类名（从 ELF 字符串直接恢复）

> TASK-0016 更正（E-0016-N01）：下面最初的 93 处计数不是完整扫描，`.hpp` 的 50 处
> 是分页截断。2026-10-12 重新通过 Ghidra MCP 扫描规范地址空间得到 43 个 `.cpp`、
> 246 个 `.hpp`、9 个 `.h`，共 298 处路径匹配（54 种不同字节串）。对全部 43 个 `.cpp`
> 地址复查，有 40 个存在 PARAM 引用；本页原先“全部没有交叉引用”的陈述被反证。
> `fmenus.cpp`、`lzcompress.cpp`、`movieplayer.cpp` 仍未报告引用。方法文字也得到更完整
> 的目录；存在文本不等于已确认函数身份。具体计数、地址及逐文件核对见
> [NAMING_AUDIT.md](NAMING_AUDIT.md) 和
> [原版标识符目录](../reverse/naming/task0016-original-identifiers.csv)。本页历史描述保留，
> 对与新证据冲突的计数及全局“无引用”结论，以该更正为准。

完成日期：2026-10-12，Australia/Sydney。这不是某个子系统任务的产物，是**项目级**的证据，
因为它一次性覆盖了多个子系统（bWare 中间件、前端菜单、摄像机、声音、回放、AI、movieplayer）。
后续任何任务涉及到这些子系统时都应该引用本文件，命名/组织代码时优先向这里恢复的真实原版
标识符看齐，而不是继续发明 port-side 名字。

## 证据来源与方法

对已授权 ELF（`SLUS_203.62`，SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`）做只读字符串搜索
（GhidraMCP `memory search`，search_type=string），搜索词 `.cpp`/`.hpp`，命中 43+50=93 处。
这些字符串几乎可以肯定是 `__FILE__` 宏在 `assert()`/调试日志/性能分析埋点中的展开结果（PS2
SN Systems 工具链的常见模式）。本会话**没有找到**这些字符串被代码直接引用的交叉引用
（`references_to` 对检查过的几个精确地址均返回空）——这很可能说明相关的 assert/profiler 代码
在零售版里被条件编译裁掉了，但字符串数据本身仍然链接进了最终 ELF。**因此：我们有文件路径和
附近的类/方法名字符串证据，但还没有把它们逐条钉死到具体的调用点地址**，这是明确的未解决项，
不是已证实的"某函数位于某文件某行"。

## 恢复的原版源码目录结构

编译期路径没有出现盘符前缀，只恢复到这个粒度（可能是构建系统用相对/映射路径调用编译器）：

```text
indep\src\
├── fmenus.cpp                          # 前端菜单系统（见下方，与 TASK-0011 证据高度吻合）
├── frontendscreens\
│   ├── selectcar.cpp                   # 选车界面
│   ├── racesetup.cpp                   # 比赛设置界面（Race Setup / Knockout Setup / Lap Knockout / Free Run / Select Car2P）
│   ├── options.cpp                     # 选项界面
│   └── joystickconfiguration.cpp       # 手柄配置界面
├── car.cpp                             # 车辆（CloseCars/CloseCops/CarSlidingSideways/CarCollision/FlattenFern…）
├── racecoordinator.cpp                 # RaceCoordinator 类实现
├── cameramover.cpp                     # 摄像机（含 CM_CUBIC_DAMPER 等枚举、MenuCarDispl…字符串，和 FrontEnd 有交集）
├── zonecameras.cpp                     # ZonePreview 相关
├── player.cpp                          # Player（ScorePoints 等）
├── carinfo.cpp                         # 车辆信息（%s_REVERSELIGHT_ON 等贴图名模板）
├── spline.cpp / quickspline.cpp        # 样条
├── carsounds.cpp                       # 车辆音效（CopSiren0/TirePop1/SirenSound/HornSound3…）
├── table.cpp                           # （PidError…）
├── animateddriver.cpp                  # AnimatedDriver 类
├── sound.cpp                           # 声音子系统
├── lzcompress.cpp                      # LZ 压缩（即 .LZC 文件的压缩/解压实现所在）
├── trackroute.cpp                      # 赛道路径
├── eventtree.cpp                       # 事件树（TASK-0012 里见过的 "View Event Tree" 字符串所在子系统）
├── beltmover.cpp                       # BeltMover 类
├── trackpath.cpp                       # TrackPathC… （赛道寻路）
├── speech\sedwedge.cpp
├── movieplayer\
│   ├── cyclicbuffer.cpp
│   ├── dctimage.cpp
│   ├── movieplayer.cpp                 # MOVIE_%d_%d_Y / UA / VA / US / VS（电影解码，含 DCT 图像）
│   └── ringbuffer.cpp
└── bDebug.cpp 等见下方 bWare 中间件部分
```

另有不带 `indep\` 前缀、直接在 `src\`/`src/` 下的一组文件，几乎可以肯定是 **bWare 中间件**本身
（与 TASK-0012 已确认的 "bWare/inc/bWare.hpp"、"bSound/include/bFragmentedVector2.hpp" 调试
路径一致，这里进一步确认了中间件的 `.cpp` 源文件名，并且这批文件里嵌有真实的 `assert()` 表达式
文本，证据质量更高）：

```text
src\bDebug.cpp             -- assert: "return_code < max_chars"；日志前缀 "**** Assertio[n...]"
src\bFile.cpp              -- assert: "bStrLen(filename) + 1 < (int)sizeof(bFile::Filename)"
src\bFileE.cpp             -- "WARNING:  Too many CD errors.  Will now call bFatalCDErrorFunction() at 0x%08x"；assert: "num_read_chain_tags > 0"
src\bMemory.cpp            -- assert: "(size >= (int)sizeof(FreeBlock)) && ((size & 3) == 0)"
src\EspClient.cpp          -- assert: "bfBufferCurrent <= bfBufferEnd"；"EspressoS[erver?]"
src/bDriverPS2.cpp         -- "SOUND_Memory_EE"，"bSound : EE Mem Usage: %d bytes"，"bSound Multistream Error: Can't init IOP"
src/bEffect.cpp            -- assert: "!m_OnlyOne"
src/bRegion.cpp            -- "bSoundRegions"
src/bSample.cpp            -- assert: "voiceId != BSND_CHANNEL_BUSY"，"m_pVoice != NUL[L]"
src/bSegment.cpp           -- "bSound: API not initialized!"
src/bSound.cpp             -- assert: "pRegion != 0"，"cannot load more than one region simultaneously!"
src/bVoice.cpp             -- assert: "VoiceId < BSND_PS2_NUM_VOICES"
src/bJukebox.cpp           -- assert: "nCatNum < m_nNumCategories"，引用 include/bJukebox.h
src\care.cpp               -- CARFX_BREAK_WINDOW_RIGHT_FRONT/REAR、CarEmitterPositionSlotPool、CarRenderInfo（"care" 可能是 "car effects" 的缩写，未证实）
src\acidfxe.cpp            -- XE_WaterFall_Drops_01（特效相关，"acid fx e[ngine]"？未证实）
src\memorycard.cpp         -- sceMcErrSemapho/sceMcErrUnBind/sceMcErrNullStr（PS2 记忆卡 API 封装）
```

## 恢复的原版类名、方法名（部分，均来自相邻字符串，未逐条钉死到地址）

这些是从同一批字符串附近直接读到的 C++ 符号文本（很可能是断言消息里的类名前缀，或调试打印里的
`"Class::Method()"` 格式字符串），**均为原版真实标识符，不是本项目起的名字**：

| 原版标识符 | 上下文 / 可能含义 |
|---|---|
| `World::DoTimestep` | 性能分析类别字符串，紧跟 `- UpdatePlayersGameState()` 和 `- Race::Update()` 两个子项，21 次重复出现（像是一个按子系统展开的性能计数器名字表）。**确认了 TASK-0010 一直假设但未证实的 "World" 类名**。 |
| `Race::Update()` | 同上，确认 `Race` 类。 |
| `RaceCoordinator` | 另见 "RaceCoordinator.Memory Leak - Track Specific" 警告文本，以及独立文件 `racecoordinator.cpp`。 |
| `ReplayManager` / `ReplayManager::HandleSnapshot()` | 回放系统，附近还有 `REPLAY_MANAGER_STATE`、`DEMO_MODE_SNAPSHOT`、`CHECKSUM0..3`、`REPLAY0.BIN`。 |
| `TrafficCar::Move()` | 确认 `TrafficCar` 类及其 `Move()` 方法——与 TASK-0008 `POLICE_AI_MAP.md` 里 CopMover/CopDriver 的 "Move" 假设是同一套虚函数命名惯例的旁证。 |
| `RenderManager::RenderLayer()` | 渲染管理器。 |
| `FlailerRenderManager::RenderScreenPrintf()` | "Flailer" 未知含义，可能是调试专用渲染管理器子类。 |
| `GrandSceneryCullInfo::CullView` / `::DoCulling()` | 远景裁剪系统。 |
| `List::Sort()` | 通用容器类（可能对应 reverse/functions 里已有的 `HP2_SortListByAdjacentSwaps` 等容器函数族，未交叉核实）。 |
| `AnimatedDriver` | 类名，紧跟在同名字符串后出现（典型的 RTTI/工厂注册模式，与 TASK-0011 的 "类分发函数传入名字字符串构造对象" 模式一致）。 |
| `PsychoDriver`、`RealDriver` | 两个 AI 驾驶员类候选名（"PsychoDriver" 疑似特技/疯狂 AI 行为，未证实）。 |
| `NisCameraCoordinator` | NIS（过场动画）摄像机协调器。 |
| `QuickSpline` | 对应 `quickspline.cpp`。 |
| `BeltMover` | 对应 `beltmover.cpp`。 |
| `EspressoServer`（推断，字符串被截断为 "EspressoS"） | 对应 `EspClient.cpp`，猜测是 PS2 开发工具链的调试/日志服务器组件，未证实。 |
| `TrackPathC[...]` | 对应 `trackpath.cpp`，类名被截断。 |
| `JR2Server` | 未知系统，出现在摄像机/traffic 相关字符串附近。 |
| `TexturePack` / `TextureAnimPack` | **原版对我们一直称为 "TPK" 格式的真实类名**；"TPK" 这个缩写本身大概率就来自 "TexturePack" 的实际文件/类名首字母缩写，与 docs/ASSET_PIPELINE.md 里引用的社区工具命名（`TPK_HP2_*`）吻合，不是社区凭空发明的。 |
| `LoadedTextureData` | 高频重复出现的字符串（很可能是贴图加载失败/调试日志的固定前缀文本，不是类名，谨慎对待）。 |

## 与 TASK-0011（前端菜单）的直接关联

`indep\src\fmenus.cpp` 字符串前后紧邻的内容是：`MENUITEM_%d`、`RECORD_LABEL`、`ARTIST_NAME`、
`SONG_TITLE`、`RECORD_TITLE`、`CHYRON_BG_%d`、`LOADING_TRACK`、`tHelpMenu`、`tPopupMenu`、
`GlobalBackgroundAnimat[ion]`。这与 TASK-0012 在 `FRONTEND/FRONTB.BUN` 资源文件里独立发现的
`UwEMENUITEM_*`、`MENUITEM_*`、`UwEPOPUP_TITLE` 等控件名字符串高度吻合（见
docs/ASSET_PIPELINE.md §3），强烈暗示：

- **`fmenus.cpp` 就是 TASK-0011 里追踪的 `FUN_0016eef8`（TitleScreen 实例消息处理）、
  `FUN_0016f440`/`FUN_0016f810`（类分发函数）、`FUN_0016c4c0`/`FUN_0014cc78`（屏幕切换管理器）
  等函数的真实原版源文件**，虽然本会话没能把具体某一行代码钉死到某个精确地址（见上方"未解决"
  说明），但文件级别的对应关系证据很强。
- `frontendscreens\selectcar.cpp`、`racesetup.cpp`、`options.cpp`、
  `joystickconfiguration.cpp` 是各个**具体菜单页面**各自独立的源文件，说明原版架构是
  "fmenus.cpp 提供通用菜单框架 + frontendscreens\ 目录下每个页面一个文件"，这与我们在
  `src/game_core/frontend/screen_manager.cpp` 里实现的"通用 ScreenManager + 具体页面"分层
  设计在**结构上**是吻合的（并非巧合复刻，是先做了结构分析后才搭的，但这里是第一次有原版
  源码目录证据直接支持这个分层假设）。

## 对项目命名的影响

- 后续如果要给 TASK-0011 涉及的函数赋予 `HP2_` 前缀的"推测名"，应当优先选择与
  `fmenus.cpp`/`frontendscreens\*.cpp` 语境一致的名字，而不是泛泛的 `HP2_GameFlowXxx`
  风格——但在没有把具体函数钉死到具体文件的情况下，不应该在 `reverse/symbols.csv` 的
  `original_identifier` 列里写 "fmenus.cpp"，那样会把"极可能"冒充"已证实"。
- `TexturePack`/`TextureAnimPack` 应该作为 docs/ASSET_PIPELINE.md 里 "TPK" 格式的原版类名
  补充记录一下（已在本文件记录；建议下次更新 ASSET_PIPELINE.md 时加一条交叉引用）。
- `World`、`Race`、`RaceCoordinator`、`ReplayManager`、`TrafficCar` 这几个类名现在有了字符串级
  证据支持，比之前"从变量名猜的"置信度高得多，下次做 TASK-0009/TASK-0010 的命名工作时应该
  优先使用这些真实类名，而不是继续用 `HP2_` 推测前缀。

## 未解决问题

1. 这 93 处字符串目前都**没有**在 ELF 里找到直接引用它们的代码位置（`references_to` 返回空）。
   没有进一步确认这是因为零售版裁掉了对应的 assert/profiler 调用代码（字符串数据是"孤儿"），
   还是 Ghidra 的静态交叉引用分析漏掉了间接/计算得出的引用。
2. 文件路径里 `\indep\` 之前是否还有更长的盘符/项目名前缀（比如 `X:\NFSHP2\indep\...`）未确认，
   本会话读取的上下文窗口里没有看到更多前缀字符。
3. 本文件列出的类名/方法名都是"字符串相邻"证据，不是"确认属于哪个具体函数/地址"的证据；
   逐条钉死需要进一步对每个地址做更细致的交叉引用或模式匹配工作，未执行。
4. 还有一些字符串本会话没有展开调查（比如 `JR2Server`、`EspClient.cpp`、`care.cpp`、
   `acidfxe.cpp` 的具体含义），留作后续任务线索。

延续提示：如果要把某个具体地址（比如 `FUN_0016eef8`）和 `fmenus.cpp` 的具体行号对应起来，
下一步可以尝试找这批 `.cpp`/`.hpp` 字符串所在的"字符串表"结构（是否是一个统一的
`{file, line, expr}` 断言记录数组？），如果能找到这个表的内存布局，也许能反向解出哪些函数在
哪个文件区间内被编译，从而把更多函数批量归类到正确的原版源文件。
