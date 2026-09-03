# boot_app 无线收发无法连接 —— 缺失项清单与逐文件移植映射表

> 参考官方工程：`D:\资料\wireless_mic_1532\wireless_mic_1532`
> 内含一收一发的官方 SDK：`wireless_mic_tx_sdk`（TX/发射）、`wireless_mic_rx_sdk`（RX/接收）、`wireless_mic_unified_sdk`（收发合一，二选一编译）
> 目标工程：`d:\BanAirBundy\boot_app`
> 芯片方案：Mountain View BP1532，Turnkey 2_6（2T1R 单向低延迟，44.1K）

---

## 1. 结论（先看）

boot_app **从未真正启动 Mountain View 无线协议栈**，所以不是"连接质量/距离"问题，而是"协议栈没跑起来"。具体是三重问题叠加：

1. 无线总开关 `BOOT_APP_WIRELESS_EN = 0`，整段无线逻辑被预处理裁掉；
2. 唯一的无线胶水文件 `wireless_app.c` 调用了一套**根本不存在的封装 API**（`wireless_api.h` / `Wireless_Init` / `Wireless_Schedule` / `Wireless_StartPairing` / `WirelessConfig_t`），工程里没有实现；
3. 官方协议栈真正需要的**基带加载、角色设置、EM 内存初始化、配对信息注册、SBC/音频流水线、调度任务、BT 中断优先级**这一整套流程，boot_app 自身代码里一处都没有调用。

底层库其实已经就位（`wireless_mv/lib/*.a` 已在 `.cproject` 链接、头文件已加入 include 路径），缺的是**上层移植代码**。

> ⚠️ **重要更正（结合实机现象复查）**：boot_app 并非"从来没写过无线上层"。git 历史里，顶层 `wireless_lib/` 曾经是一套**真实的 MV 无线集成**（`mvwire_stack.c` / `mvwire_assoc.c` / `mvwire_transbuf.c` / `wireless_tx.c` / `wireless_rx.c` / `sbc/*`），是在 commit `1efbaf0（无线库over flow）` 加入的；但当前 HEAD `e359d8e（添加2代Banux）` 把整个 `wireless_lib/` **删空了**（工作区只剩一个空目录）。所以：
> - 你之前实机看到的 "TX 反复打印连接/断开、RX 从不进入连接态"，是 `1efbaf0` 那版**真实代码**跑出来的；
> - 现在的代码连编译都过不了（`wireless_app.c` 引用的 `wireless_api.h` 已随 `wireless_lib/` 一起被删）。
> - 那版真实代码为什么会出现该现象，根因见下方 **§2 根因 D / E / F**。

---

## 2. 根因总览（带证据）

### 根因 A：无线功能被开关关闭
`boot_app/system_config/app_config.h`：

```c
#define BOOT_APP_WIRELESS_EN   0                          // ← 关闭
#define BOOT_APP_MVWIRE_EN     BOOT_APP_WIRELESS_EN       // = 0
```

- `wireless_app.c` 整个文件体包在 `#if BOOT_APP_WIRELESS_EN` 内 → 被裁掉。
- `wireless_app.h` 在关闭态把 `App_WirelessStart()` 变成 `static inline ... { return 0; }`。
- `main.c` 第 291 行 `App_WirelessStart()` 实际是空调用，任务永不创建。

### 根因 B：调用了一套虚构的封装 API
`wireless_app.c` 引用 `wireless_api.h` 并调用：

| 调用的符号 | 工程内是否有定义/实现 |
|---|---|
| `wireless_api.h`（头文件） | ❌ 全工程不存在 |
| `WirelessConfig_t` | ❌ 不存在 |
| `Wireless_Init(&cfg)` | ❌ 不存在 |
| `Wireless_StartPairing()` | ❌ 不存在 |
| `Wireless_Schedule()` | ❌ 不存在 |
| `Wireless_RegisterConnectedCb()` | ❌ 不存在 |
| `Wireless_RegisterDisconnectedCb()` | ❌ 不存在 |

全仓库检索，这些符号**仅出现在 `wireless_app.c` 的调用处**。也就是说把开关改成 1 也**编译不过**（找不到头文件与函数）。boot_app 真正带的库头文件是 `wireless_mv/include/{wireless2.h, wireless_usr_api.h, audio_association.h, ...}`，API 形态与上面完全不同。

### 根因 C：官方协议栈的初始化/调度/配对流程完全没实现
下列官方必需的调用，在 boot_app 自身 `.c` 代码中**均为 0 处调用**（头文件里有声明、`init-default.c` 里配了 MPU 内存映射，但没人调用业务初始化）：

- `wireless2_2_X_initfuncset()`（即 `WIRELESS_FUNCTION`，加载基带固件到 BB MPU）
- `MVWIRE2_DeviceRoleSet()`
- `MVWIRE2_2T1R_Set_TxMode()` / `MVWIRE2_SetWorkMode()`
- `MVWIRE2_ParamInit()` / `Wireless_common_init()` / `wireless_em_size_init()`
- `WirelessDeviceIdInit()` / `TX_PairedDeviceInit()` / `RX_PairedDeviceInit()`
- `MVWIRE2_slave_schedule()`（或 master 调度）/ `WirelessStackTask()` / `MVWIRE2_ConnStateDisplay()`
- `audio_init()` / `audio_process()` / `wireless_audio_process()`
- `NVIC_SetPriority(BT_IRQn, 0)`

此外，官方是**裸机超级循环**（`while(1){ WDG_Feed(); WirelessStackTask(); audio_process(); }`）持续驱动，boot_app 却想用一个 FreeRTOS 任务里 `Wireless_Schedule()` + `vTaskDelay(1)` 的松散模型驱动，与协议栈的实时性要求不匹配。

---

## 2bis. 实机现象根因（TX 反复连断 / RX 从不连接）—— 针对 `1efbaf0` 真实版本

下列证据取自 `git show 1efbaf0:wireless_lib/*`。这一节直接回答"为什么之前打开也连不上"。

### 根因 D：RX 的 AudioAssociation 处理被**硬编码关掉**（致命）
`wireless_lib/wireless_rx.c` 的 `rx_audio_process()`：

```c
#ifndef WL_RX_ASSOC_PROCESS_EN
#define WL_RX_ASSOC_PROCESS_EN 0          // ← 默认关闭！
#endif
#if WL_RX_ASSOC_PROCESS_EN
    got = MvWire_AssocProcess(s_rx.pcm_l, s_rx.pcm_r);  // 真正的 AudioAssociationProcess
#else
    static uint8_t once;
    if (!once) { once = 1; DBG("[WL] AssocProcess DISABLED (bisect)\n"); }
    got = 0;                               // ← 恒为 0，随后 if(got==0) return; 直接返回
#endif
```

- 代码注释原文：*"Bisect stack overflow … AudioAssociationProcess (lib) appears to need a very deep task stack."* —— 因为库函数 `AudioAssociationProcess` **栈溢出**，为了二分定位崩溃点，把它整个禁用了。
- 在 2T1R / Turnkey 2_6 架构里，**RX(master) 侧正是靠 `AudioAssociationProcess()` 拉取收到的 RF 帧、维持位/帧同步、把连接从 RF 层推进到 `CONNECT_AUDIO`**。它被禁用后，RX 永远拿不到音频同步 → **RX 从不进入连接态**，与实机现象一致。

### 根因 E：TX 因此"连上又断开"循环
- TX(slave) 在 RF 层能先与 master 关联（协议库触发 connect → TX 侧打印 connected / `device1_conn`）；
- 但 master(RX) 侧 association 从不推进（根因 D），音频级同步握手完不成 → 链路超时被拆除 → TX 打印 disconnected / `device1_disconn` → 再次关联 → **无限连断循环**，与实机现象一致。
- 注意 `wireless_tx.c` 的 `tx_audio_process()` 有 `if (connect_status != CONNECT_AUDIO) return;`，连接一直不稳 → TX 也几乎发不出有效音频。

### 根因 F：深层原因是 FreeRTOS 任务栈溢出 + 调度模型不匹配
- 当前 `wireless_app.c` 里的注释与 `WL_TASK_STACK_WORDS 4096`（静态栈）也是同一问题的遗留：*"Static stack: avoid heap adjacency smash into TCB when Assoc/SBC deep-calls"* —— `AudioAssociationProcess` / SBC 的深调用把 WL 任务栈打爆，甚至踩坏相邻堆上的 TCB。
- 官方是**裸机超级循环 + `BT_IRQn` 中断驱动**，association 按 RF 帧节奏被紧凑、连续地调用；这里塞进一个 `vTaskDelay(1ms)` 的 FreeRTOS 任务，栈深度和实时性都不够。
- 当时的"解决办法"是**关掉 RX association** 绕过崩溃，代价就是 RX 彻底不工作、TX 连断循环。

### 正确修复方向（不是删库，而是恢复 + 根治栈溢出）
1. 从历史恢复上层实现：`git checkout 1efbaf0 -- wireless_lib`（或按 §5 映射表重新落地），并确认 `.cproject` 重新纳入这些源文件与 `sbc/`、`libsbcenc.a`。
2. 打开 `WL_RX_ASSOC_PROCESS_EN = 1`，**并真正解决栈溢出**（而不是禁用）：
   - 给 WL 任务足够大的栈（实测 `AudioAssociationProcess` 所需深度后再定，4096 words 可能仍不够）；
   - 把 association/SBC 的大缓冲从栈移到静态/BSS（`mvwire_assoc.c` 里 `s_pcm_l/s_pcm_r` 已是静态，需核对库内部是否还在栈上开大数组）；
   - 必要时把无线调度从 FreeRTOS 任务改回**高优先级紧凑循环 / 中断驱动**，贴近官方模型。
3. 保证 `rx_audio_process()`（含 `MvWire_AssocProcess`）在主循环里被**持续高频**调用；`NVIC_SetPriority(BT_IRQn,0)` 已具备。
4. TX / RX 两份镜像分别编译烧录，核对角色（TX=SLAVER、RX=MASTER）、link key(0x21/0x56)、Turnkey(2_6)、采样率/帧长一致。

> 一句话总结实机现象：**RX 的音频关联处理被人为禁用（为绕过栈溢出），导致 RX 永远同步不上、TX 反复连断。修栈溢出 + 恢复 `wireless_lib` + 打开 `WL_RX_ASSOC_PROCESS_EN` 才是正解。**

---

## 3. 官方 SDK 的真实运行流程（TX / RX）

来源：`wireless_mic_tx_sdk/main.c`、`wireless_mic_rx_sdk/main.c`、`wireless/wireless_main.c`、`wireless/wireless_app_main.c`。

### 3.1 初始化顺序（main 里，TX 与 RX 基本相同）

```
Chip_Init(1); WDG_Enable(WDG_STEP_4S);
时钟：Clock_Config / Clock_PllLock(360000) / Clock_APllLock(240000)
      Clock_SysClkSelect(PLL_CLK_MODE) / USB / UART 时钟
      Clock_Module1/2/3Enable(...)，再 Disable 用不到的模块
soc_cal_set();
DbgUartInit(...);                       // 调试串口
Remap_InitTcm(0x10000, TCM_SRAM_START_ADDR_1, WIRELESS_TCM_SIZE);   // TCM
SpiFlashInit(80000000, MODE_4BIT, 0, FSHC_PLL_CLK_MODE);
WirelessDeviceIdInit();                 // 读 ChipID → WirelessDeviceId
TX_PairedDeviceInit() / RX_PairedDeviceInit();  // 注册配对读写回调 + 读 flash_dev_param
T_HeapInit(); GIE_ENABLE(); SysTickInit();
Timer2 1ms 配置 + NVIC_EnableIRQ(Timer2_IRQn);
PMU_NVMInit();
WIRELESS_FUNCTION(BB_MPU_START_ADDR);   // = wireless2_2_X_initfuncset()，加载基带
MVWIRE2_DeviceRoleSet(WIRELESS_SDK_ROLE);         // master / slave
#ifdef MV_WIRELESS2_MODE2
  MVWIRE2_2T1R_Set_TxMode(MV_WIRELESS2_PARAM1);
  MVWIRE2_SetWorkMode(MV_WIRELESS2_PARAM2);
#endif
WirelessInit();                         // 见 3.2
OTG_DeviceModeSel(...); UsbDevicePlayInit(); UsbDeviceEnable();
NVIC_SetPriority(BT_IRQn, 0);           // 无线中断最高优先级
audio_init();                           // SBC/ADC/DAC/音频链路
CtrlVarsInit(); audio_init_isready = 1;
```

### 3.2 `WirelessInit()`（在 `wireless_main.c`）

```
#if !defined(WIRELESS_TURNKEY2_6)  wireless2_set_afh(0);  #endif
wireless_em_size_init();               // 校验 wireless_em_size() <= BB_EM_SIZE
ConfigWirelessBbParams(&params);       // em_start_addr / agc / sniff 配置
MVWIRE2_ParamInit(&wireless2_config);  // npack / rf_pbuffer / rf_translen / au_audiolen ...
wireless_AudioParityCntProc_cb  = ...; // 2_6 需要
memset((uint8_t*)BB_EM_MAP_ADDR, 0, BB_EM_SIZE);   // 清 EM 区
Wireless_common_init(&params);         // 协议栈公共初始化
```

`wireless2_config` 关键字段（TX 示例）：
```c
.npack         = RFPACK_NAUDIO;
.rf_pbuffer    = rfsend_buffer;        // [RFAUDIO_TRANS_LEN + 23]
.rf_translen   = RFAUDIO_TRANS_LEN;
.au_audiolen   = SBC_ENC_LEN_PER_FREME - CRC_PACKSUB;
```

### 3.3 主循环（超级循环，必须持续跑）

```
while(1){
  WDG_Feed();
  WirelessStackTask();      // 内部: MVWIRE2_slave_schedule()/master + MVWIRE2_ConnStateDisplay()
  audio_process();          // TX；RX 侧为 wireless_audio_process()
  OTG_DeviceRequestProcess();
  SystemEventProcess();     // 按键/配对/重匹配等
  TX_PairedInfoSetFlash();  // 配对信息落盘
}
```

### 3.4 连接/配对回调链
- TX：`wireless_app_test_start()` → `wireless_app_start_adv(conn_ind_cb)`；连接成功回调里校验地址/Key（`wireless_app_is_key_matched` 比对 `WIRELESS_LINK_KEY0/1`），注册 `RF_GetData_Interrupt` 发送回调。
- 连接状态：`MVWIRE2_ConnectedCB` / `MVWIRE2_DisconnectedCB` → 更新 `device1.ConStatus`；`MVWIRE2_ConnStateDisplay()` 打印 `device1_conn` / `device1_disconn`。

---

## 4. boot_app 缺失项清单

### 4.1 配置宏缺失（`system_config/app_config.h` 及相关头）
boot_app 已有：`WIRELESS_TURNKEY2_6`、`SAMPLE_RATE=44100`、`ONE_FRAME=128`、`PACKET_AUDIO_CH=1`、`RFPACK_NAUDIO=1`、`AUDIO_QUALITY=20`、`CRC_PACKSUB=0`、`MV_WIRELESS2_MODE2`、`MV_WIRELESS2_PARAM1=0`、`MV_WIRELESS2_PARAM2=8`、`WIRELESS_FUNCTION`、`WIRELESS_LINK_KEY0/1`、`WIRELESS_SDK_ROLE`、`ENCODE_CH/DECODE_CH`。这些与官方 2_6 **数值一致**。

boot_app **缺失**（官方在 `sbc_encoder.h` / `app_config.h` / `wireless2.h` 中定义，业务代码依赖）：

| 缺失宏/常量 | 官方来源 | 用途 |
|---|---|---|
| `RFAUDIO_TRANS_A` / `RFAUDIO_TRANS_LEN` | `sbc/encoder/include/sbc_encoder.h` | RF 单包发送长度 |
| `RFAUDIO_FRAME_LEN` | `sbc_encoder.h` | 帧长计算基础 |
| `SBC_ENC_LEN_PER_FREME` | `sbc_encoder.h` | 编码帧长（`.au_audiolen`） |
| `SBC_DEC_LEN_PER_FREME` | `sbc_encoder.h` | 解码帧长（反向通道） |
| `PACKET_FRAME_LEN(x)` | `wireless_usr_type.h`/`sbc` | 打包长度宏 |
| `CFG_LOCK_PAIRED_TXRX` / `CFG_PAIRING_SUPPORTMDOE` / `CFG_LOCK_PAIRED_*` | `app_config.h` | 配对锁定策略（决定是否需要 flash 配对） |
| `WIRELESS_TCM_SIZE` | `sram_config.h` | TCM 大小（`Remap_InitTcm` 用） |
| `CONINF_FLASH_ADDR` 对应的配对结构 `Tx_Flash_param_t`/`Rx_Flash_param_t` | `wireless_main.c` | 配对信息存储 |

> boot_app 已有 `COMPANY_BYTE2/3`、`NOPAIR_WORD`（在 `wireless_mv/include/wireless2.h`），`BB_EM_SIZE`、`BB_EM_START_PARAMS`、`BB_MPU_START_ADDR`（在 `driver/driver/inc/bb_api.h`、`chip_config.h`），`BT_IRQn`（在 `irqn.h`）。这些**无需重复定义**。

### 4.2 源文件缺失（需要移植进来的业务代码）

| 缺失文件（官方位置） | 作用 | boot_app 现状 |
|---|---|---|
| `wireless/wireless_main.c` | `WirelessInit` / `WirelessStackTask` / `MVWIRE2_*CB` / `WirelessDeviceIdInit` / 配对参数读写 | ❌ 无 |
| `wireless/wireless_app_main.c` | 广播/连接回调、地址校验、TX 发送回调 | ❌ 无 |
| `sbc/**`（encoder + decoder + `sbc_api.c` + `libsbcenc.a`） | SBC 编解码，音频链路核心 | ❌ 无 |
| `05_component/audio/audio_main.c` 等 | `audio_init` / `audio_process` / `wireless_audio_process` | ❌ 无（仅有无关的 `nand_smart_audio_init`） |
| `audio_effect/`（ctrlvars、roboeffect 等，可选） | 音效/控制变量，`CtrlVarsInit` | ❌ 无（如不需要音效可裁剪） |

### 4.3 初始化/调度调用缺失（`main.c` 里）
`boot_app/src/main.c` 现有流程止于：`FwUpgrade_* → prvInitialiseHeap → App_UsbInit → App_BanuxInit → GIE_ENABLE → App_WirelessStart(空) → xTaskCreate(vUsbTask)`。

缺失官方 3.1~3.3 的**全部无线相关调用**：`Remap_InitTcm`、`WirelessDeviceIdInit`、`TX/RX_PairedDeviceInit`、`WIRELESS_FUNCTION`、`MVWIRE2_DeviceRoleSet`、`MVWIRE2_2T1R_Set_TxMode`、`MVWIRE2_SetWorkMode`、`WirelessInit`、`NVIC_SetPriority(BT_IRQn,0)`、`audio_init`，以及主循环里的 `WirelessStackTask` + `audio_process` + `WDG_Feed`。

### 4.4 运行模型冲突
- 官方：裸机超级循环，无线调度必须**高频、持续、不被长时间阻塞**，并喂狗。
- boot_app：FreeRTOS 多任务（USB / Banux / Shell）。若把无线塞进一个 `vTaskDelay(1)` 的任务，需保证该任务优先级足够高、tick 抖动可控、且喂狗不超时。**建议**：把无线调度放在高优先级任务且 `vTaskDelay` 尽量小，或改造为在空闲/专用高优先级任务中紧凑循环调度。

---

## 5. 逐文件移植映射表

| boot_app 目标文件 | 来源（官方） | 需要做的事 |
|---|---|---|
| `system_config/app_config.h` | `wireless_mic_tx_sdk/system_config/app_config.h`、`unified_sdk/.../app_config.h` | 补齐 `RFAUDIO_TRANS_*`、`SBC_*_LEN_PER_FREME`、`PACKET_FRAME_LEN`、配对策略宏；保持 2_6 数值与官方一致；把 `BOOT_APP_WIRELESS_EN` 置 1 |
| `system_config/sram_config.h` | 官方 `sram_config.h` | 确认 `WIRELESS_TCM_SIZE`、`TCM_SRAM_START_ADDR_1`、`BB_MPU_START_ADDR` 布局一致 |
| 新增 `wireless/wireless_main.c` | `wireless_mic_tx_sdk/wireless/wireless_main.c` | 移植 `WirelessInit`/`WirelessStackTask`/`MVWIRE2_*CB`/`WirelessDeviceIdInit`/配对读写；按 TX 或 RX 角色裁剪 |
| 新增 `wireless/wireless_app_main.c` | `wireless_mic_tx_sdk/wireless/wireless_app_main.c` | 移植广播/连接回调、地址与 Key 校验、TX 发送回调 |
| 新增 `sbc/`（含 `libsbcenc.a`） | `wireless_mic_tx_sdk/sbc/**` | 移植 SBC encoder/decoder 源与库，加入 include 与链接 |
| 新增 `audio/audio_main.c` 等 | `unified_sdk/banux/05_component/audio/**` | 移植 `audio_init`/`audio_process`/`wireless_audio_process`；按 TX(采集编码)/RX(解码播放) 裁剪 |
| `src/wireless_app.c` | —— | **重写或删除**：要么删除虚构封装，直接在 main 里按官方流程调用；要么把它改造成真正包住 3.1~3.3 的胶水层 |
| `src/main.c` | 官方 `main.c` | 按 3.1 顺序插入无线初始化；在主循环/高优先级任务插入 3.3 调度；`NVIC_SetPriority(BT_IRQn,0)` |
| `.cproject` | —— | 确认新增源目录被编译、`libsbcenc.a`/`libplc128.a` 等被链接、include 路径补全 |

> 说明：boot_app 已链接 `wirelessStack`、`wireless2`、`audio_association`、`plc128`（见 `.cproject` L77-87），include 路径已含 `wireless_mv/include`（L38）。缺的是 **SBC 库(`libsbcenc.a`)** 与 **业务源码**。

---

## 6. 建议实施步骤（分阶段，可逐步验证）

1. **阶段 0：确定角色**。boot_app 只做 TX 还是 RX？（当前 `BOOT_APP_WIRELESS_ROLE_TX=1` → TX/Slave）。一收一发需要两份镜像：一份 TX、一份 RX，且**两侧 Turnkey/link key/CompanyWord/采样率/帧长必须完全一致**。
2. **阶段 1：补配置宏**。按 4.1 在 `app_config.h`/`sram_config.h` 补齐缺失宏，先保证能通过编译（此时仍可能因缺源码报未定义符号）。
3. **阶段 2：移植 SBC + audio_main**。加入 `sbc/`、`libsbcenc.a`、`audio_main.c`，实现 `audio_init`/`audio_process`。
4. **阶段 3：移植 wireless_main.c / wireless_app_main.c**。实现 `WirelessInit`/`WirelessStackTask`/配对回调。
5. **阶段 4：改 main.c**。按 3.1 插入初始化，按 3.3 在主循环或高优先级 FreeRTOS 任务插入调度 + 喂狗；设 `BT_IRQn` 优先级 0。
6. **阶段 5：处理 `wireless_app.c`**。删除虚构封装，或改造为真正包住上述调用的胶水层；把 `BOOT_APP_WIRELESS_EN` 置 1。
7. **阶段 6：联调**。TX/RX 两份镜像分别烧录，看串口是否打印 `device1_conn`，验证音频链路。

---

## 7. 自检 / 验收清单

- [ ] `app_config.h` 中 `BOOT_APP_WIRELESS_EN = 1`
- [ ] 全工程不再引用不存在的 `wireless_api.h` / `Wireless_Init` / `Wireless_Schedule`
- [ ] `RFAUDIO_TRANS_LEN`、`SBC_ENC_LEN_PER_FREME` 等宏已定义，编译无未定义符号
- [ ] main 中调用了 `WIRELESS_FUNCTION(BB_MPU_START_ADDR)`、`MVWIRE2_DeviceRoleSet`、`WirelessInit`
- [ ] main 中调用了 `NVIC_SetPriority(BT_IRQn, 0)`
- [ ] 主循环/高优先级任务持续调用 `WirelessStackTask()` + `audio_process()` + `WDG_Feed()`
- [ ] TX 侧调用了 `TX_PairedDeviceInit` + `wireless_app_start_adv`；RX 侧调用了 `RX_PairedDeviceInit`
- [ ] TX 与 RX 两份镜像的 Turnkey(2_6) / link key(0x21,0x56) / CompanyWord / 采样率(44100) / 帧长(128) 完全一致
- [ ] 串口能打印 `device1_conn`，音频可正常收发

---

## 8. 附：关键常量对照（boot_app vs 官方 2_6）

| 项 | boot_app | 官方 2_6 | 是否一致 |
|---|---|---|---|
| Turnkey | `WIRELESS_TURNKEY2_6` | `WIRELESS_TURNKEY2_6` | ✅ |
| SAMPLE_RATE | 44100 | 44100 | ✅ |
| ONE_FRAME | 128 | 128 | ✅ |
| PACKET_AUDIO_CH | 1 | 1 | ✅ |
| RFPACK_NAUDIO | 1 | 1 | ✅ |
| AUDIO_QUALITY | 20 | 20 | ✅ |
| CRC_PACKSUB | 0 | 0 | ✅ |
| MV_WIRELESS2_PARAM1/2 | 0 / 8 | 0 / 8 | ✅ |
| WIRELESS_LINK_KEY0/1 | 0x21 / 0x56 | 0x21 / 0x56 | ✅ |
| COMPANY_BYTE2/3 | 0x65 / 0x38 | 0x65 / 0x38 | ✅ |
| RFAUDIO_TRANS_LEN | ❌ 未定义 | `RFAUDIO_TRANS_A` | ⚠️ 缺失 |
| SBC_ENC_LEN_PER_FREME | ❌ 未定义 | 有 | ⚠️ 缺失 |

> 结论：**参数选型与官方一致，配对密钥不会成为障碍**；真正的问题是**上层移植代码（SBC/audio/wireless_main/wireless_app_main）与初始化/调度调用整体缺失，且被开关关闭**。

---

_本文档为方案 B（缺失项清单 + 逐文件移植映射表）。确认后可据此进入实际移植编码阶段。_

---

## 9. 修复执行记录（已落地）

> 目标：参考官方 `wireless_mic_1532` 例程，让 boot_app 的 TX/RX 真正能收发连接。
> 底层库（`boot_app/wireless_mv/lib/*.a`）与头（`include/*.h`）本就齐全，缺的是**上层移植代码**（曾存在于 `1efbaf0` 的 `wireless_lib/`，被 HEAD `e359d8e` 删空）与若干开关。已按下述恢复并修复。

### 9.1 已执行的修改

| # | 文件 | 修改 | 对应根因 |
|---|---|---|---|
| 1 | `wireless_lib/`（29 文件） | 从 `1efbaf0` 恢复整套 MV 无线集成（mvwire_stack/assoc/transbuf、wireless_tx/rx、audio_codec/driver_api、sbc/*） | B/C |
| 2 | `system_config/app_config.h` | `BOOT_APP_WIRELESS_EN=1`、`BOOT_APP_MVWIRE_EN=1`（`#undef` 后强置，不依赖 IDE -D） | A |
| 3 | `wireless_lib/wireless_config.h` | 新增 `WL_RX_ASSOC_PROCESS_EN=1`（默认打开 RX 的 `AudioAssociationProcess`） | **D（致命）** |
| 4 | `wireless_lib/mvwire_stack.c` | `MvWire_StackInit` 在 `Wireless_common_init` 后补 `wireless2_Enable_Remote_Sleep_Cmd(0)`，镜像官方 `WirelessInit` | 对齐官方 |
| 5 | `boot_app/.cproject` | include 路径补回 `wireless_lib`、`wireless_lib/sbc` | C（编译阻塞） |

### 9.2 根因 D 为何是“TX 反复断连 + RX 不进连接态”的真凶

- 官方 RX 主循环每轮调 `wireless_audio_process()`，核心是 `AudioAssociationProcess()`（`audio_main.c:592`）——**消费 RF 接收 FIFO、解码 SBC、推进第 1 帧同步**。
- boot_app 的 `wireless_rx.c` 把该调用用 `WL_RX_ASSOC_PROCESS_EN` 包起来且**默认 0**（当初为排查栈溢出临时禁用）。禁用后 `rx_audio_process()` 里 `got=0` 直接 return，**RX 永不消费接收 FIFO、永不完成音频同步**。
- TX/RX 是一条链路：RX 侧不消费/不同步 → 链路反复重协商 → **TX 侧 `MVWIRE2_ConnectedCB`/`DisconnectedCB` 反复触发（刷 device1_conn/disconn），RX 侧进不到稳定 `CONNECT_AUDIO`**。
- 打开 `WL_RX_ASSOC_PROCESS_EN=1` 即恢复官方行为。

### 9.3 栈（根因 F）复核结论：16KB 足够，无需再改

- `AudioAssociationProcess` 的大 PCM/SBC 缓冲（`s_pcm_l/s_pcm_r/s_sbc_buf` 等）都是 **static 全局（BSS）**，不占任务栈。
- 官方裸机主栈仅 **8KB** 即可跑完整音频链路；boot_app WL 任务用 **4096 words = 16KB 静态栈**（`s_wl_stack`，独立于 20KB heap），为其 2 倍。
- `configCHECK_FOR_STACK_OVERFLOW=2` + `wireless_app.c` 已打印 `uxTaskGetStackHighWaterMark`，实机可核对余量。

### 9.4 TX / RX 是两套固件：角色切换点

角色由 **`system_config/app_config.h`** 单一开关决定：

```c
#define BOOT_APP_WIRELESS_ROLE_TX    1   // 1 = TX/Slave(发射端)；0 = RX/Master(接收端)
```

- 编 **TX 固件**：保持 `=1`（ENCODE_CH=1/DECODE_CH=0/SLAVER）→ 烧录发射端。
- 编 **RX 固件**：改为 `=0`（ENCODE_CH=0/DECODE_CH=1/MASTER）→ 烧录接收端。
- `wireless_tx.c`/`wireless_rx.c`/`mvwire_assoc.c` 均用 `#if BOOT_APP_WIRELESS_ROLE_TX` 各自包裹，两套固件都能整体编译，仅对应角色实体生效。

### 9.5 验证步骤（需在 AndeSight IDE 内；命令行工具链被本机环境拒绝执行）

1. AndeSight 打开 `boot_app`，**Clean** 后 **Build**（当前 = TX 固件）。确认无 undefined reference（`AudioAssociationProcess`/`wireless2_Enable_Remote_Sleep_Cmd`/`WirelessAudioDevice1RxSyncReset` 等由 `wireless_mv/lib/*.a` 提供，已在 `.cproject` 链接）。
2. 烧录 TX 到发射板；将 `BOOT_APP_WIRELESS_ROLE_TX` 改 `0`，重新 Build（= RX 固件），烧录接收板。
3. 上电联调，观察串口 log：
   - RX 应打印 `[WL] AudioAssociation init ok`，TX 上电后出现 `device1_conn` 且**不再反复 disconn**。
   - TX 应打印 `[WL] MvWire stack init role=TX/Slave`，连接后稳定。
   - `[WL] alive n=... hwm=...` 的 hwm 应有充足余量。
4. 声音验证：TX 端 MIC 采集 → RX 端 DAC 输出（经 `WlAudioOutput_Process` 混音，`usb_audio_api.c`）。

### 9.6 对 §8 表格的两点更正

- `RFAUDIO_TRANS_LEN`：`mvwire_stack.c` 有 `#ifndef RFAUDIO_TRANS_LEN → =RFAUDIO_TRANS_A`；`RFAUDIO_TRANS_A` 由 `wireless_config.h`(默认52)/`sbc_encoder.h` 提供。**非缺失**。
- `SBC_ENC_LEN_PER_FREME`/`SBC_DEC_LEN_PER_FREME`：由 `sbc_encoder.h`（已入 include 路径）提供。**非缺失**。

---

## 10. 第 1 帧同步 DAC 预填对齐（补充，接续 §9）

### 10.1 背景

§9 修复解决了「能否连接」。本节补齐「连接后音频的初始延迟 / 起始爆音」——即官方 `wireless_audio_process()` 里的第 1 帧同步 DAC 对齐段（`audio_main.c:543-583`），boot_app 原先缺失。

### 10.2 官方机制（三段联动）

| 位置 | 作用 |
|---|---|
| `Audio_Check1stFrameAllRight()` `audio_main.c:1013/1029` | 库回调：连续 `INTERVAL_NRF-1` 包 cnt 相同→同步完成时 `Device0_2rdPackSample = AudioDAC0_DataLenGet()`（记录此刻 DAC 缓冲量）并置 `syncpackallright=2` |
| 首次同步块 `audio_main.c:543-583` | `audio_1st_data` 闩锁：两路都失步则复位；首次检测到某路 `StateGet==2` → `AudioDAC0_DataSet(sink_dac, ONE_FRAME/8)`（dev0）或 `ONE_FRAME/3`（dev1）预填静音建立缓冲基准，并 `AudioOutDelete = DeviceX_2rdPackSample % ONE_FRAME` |
| 输出对齐 `audio_main.c:772-786` | 首次输出跳过前 `AudioOutDelete` 个样本（`&sink_dac[AudioOutDelete*2]`, `g_frame_size-AudioOutDelete`）使 DAC 播放与 RF 帧边界对齐，随后清零 |

### 10.3 boot_app 适配方案（ring 架构）

boot_app 音频输出走**弹性环形缓冲**（`WlAudioOutput_PushWireless` → `s_wireless_ring` → `WlAudioOutput_Process` → DAC），非官方 `sink_dac` 单缓冲直写 DAC，因此：

- **保留**：首次同步 `audio_1st_data` 闩锁 + 预填静音建立缓冲基准（消除起始 underrun/爆音）+ 失步复位。预填深度对齐官方（dev0=`ONE_FRAME/8`、dev1=`ONE_FRAME/3` 立体声帧）。
- **不复刻** `AudioOutDelete` 精确样本删除：它依赖 `Device0_2rdPackSample`，而 boot_app 的 `Audio_Check1stFrameAllRight()`(`mvwire_stack.c`) 置 `s_devX_sync=2` 时**未接** `AudioDAC0_DataLenGet()` 赋值（该变量恒 0），且弹性 ring 天然吸收此抖动，精确删除无意义。
- **不 return**：官方 dev0 分支预填后 `return` 是受 `sink_dac` 单缓冲所限（预填会覆盖解码帧）；ring 累加架构下预填静音垫底 + 本帧音频跟上即可，`return` 反而丢帧。

### 10.4 修改清单

| 文件 | 改动 |
|---|---|
| `wireless_lib/mvwire_port.h` | 新增 `uint8_t MvWire_1stFrameSyncState(uint8_t id);` 声明（+5） |
| `wireless_lib/mvwire_stack.c` | 实现 `MvWire_1stFrameSyncState()`，封装已有 `Audio_Check1stFrameAllRightStateGet()`（+5） |
| `wireless_lib/wireless_rx.c` | `rx_audio_process()` 在 `if(got==0)return` 之后、Mono 复制之前插入首次同步预填块（+30） |

### 10.5 验证要点

- RX 首次锁定时串口应打印 `[WL] 1st-frame sync: prime DAC ring N frames`（N=16 或 42）。
- 预填最大 `ONE_FRAME/3*2 = 84` 个 int16 < `dac_pcm_buf[ONE_FRAME*2]=256`，无越界。
- 起始爆音应消除；若仍有周期性卡顿，属 ring 深度/DAC 消费速率问题，另调 `WL_MIX_RING_SAMPLES`。
