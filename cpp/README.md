# PUSCH C++17 学习实验

这是教程第 19–31 章使用的参考工程。逐章 TODO 练习入口见 [learning_path/README.md](../learning_path/README.md)。先用清楚、可调试的标量实现学习规范，再讨论实时架构与优化。它包含真实可运行的算法，但**不是完整、可接入空口的 NR PUSCH 收发机**。

## 构建与运行

需要 C++17 编译器、CMake ≥ 3.16；核心实验无第三方运行依赖。在教程目录执行：

```sh
cmake -S cpp -B cpp/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpp/build -j 4
ctest --test-dir cpp/build --output-on-failure
cpp/build/pusch_demo budget
cpp/build/pusch_demo coding
cpp/build/pusch_demo link 12 2 2026
cpp/build/pusch_demo link 12 4 2026
cpp/build/pusch_demo channel
cpp/build/pusch_demo measure
cpp/build/pusch_sweep 2 100 > cpp/results/qpsk.csv
cpp/build/pusch_sweep 4 100 > cpp/results/qam16.csv
cpp/build/pusch_bench
```

`link` 参数：数据 RE 上的 Es/N0（dB）、Qm（2 或 4）、随机种子。`sweep` 参数：Qm、每个 SNR 点的帧数。这里一帧指一次 1380 符号的实验批次，不是 10 ms NR 无线帧。`sweep` 绕过 OFDM 和导频，用理想信道作为调制的独立基线。

内存与未定义行为检查：

```sh
cmake -S cpp -B cpp/build-asan -DCMAKE_BUILD_TYPE=Debug -DPUSCH_SANITIZE=ON
cmake --build cpp/build-asan -j 4
ctest --test-dir cpp/build-asan --output-on-failure
```

本机验证环境是 macOS arm64、AppleClang 21、CMake。Linux GCC/Clang 是目标学习环境，尚未在本次交付中执行 Linux、AVX512、SVE 或 AMX 硬件测试。

## 实现范围与规范入口

五份主规范固定为 V18.5.0 / Release 18，链接见主教程第 18 章。

| 文件/入口 | 已实现 | 规范入口及约束 |
| --- | --- | --- |
| `resource.cpp` | MCS table 1 初传索引 0–28、RIV、14 符号 SLIV、资源预算、整数 TBS | 38.214 §6.1.2、§6.1.4；TBS 引用 §5.1.3.2。未实现 RRC/DCI 解码和表选择条件 |
| `coding.cpp` | CRC16/24A/24B、BG/Zc/iLS/K/N 参数规划、码块分段与 CRC、filler mask | 38.212 §5.1、§5.2.2、§5.3.2、§6.2.1–§6.2.4。**未实现 LDPC 校验位生成或译码**；仅接受 B/C 为整数的兼容长度 |
| `rate_match/recover` | 循环选位、跳过 NULL、Qm 交织、回填及重复 LLR 累加 | 38.212 §5.4.2、§6.2.5。Ncb=N，不含 LBRM、CBG、UCI 资源扣除 |
| `phy.cpp` | Gold、数据加扰、QPSK/16QAM、硬判决/max-log LLR | 38.211 §5.1、§5.2.1、§6.3.1.1–§6.3.1.2。q=0，无 UCI 占位符 |
| `dmrs_type1/map_grid` | 普通 type-1 单符号 DM-RS、port 1000、nSCID=0、一 CDM group、单层资源映射 | 38.211 §6.4.1.1、§6.3.1.6。无跳频、PTRS、变换预编码、增强 DM-RS。序列以 CRB 0 为参考，格子以 BWP 起点为参考 |
| `FFT/OFDM/LS` | 基 2 单位化 FFT、BWP 居中映射、固定 CP、全导频平均的平坦信道 LS | 算法教学；固定 72 样点 CP 不等于 38.211 §5.3 的完整 NR 波形定时；未实现相位补偿/Point A 到 RF 中心频率转换 |
| `power/linear_mean` | 简化单次功率上限计算；线性功率平均 | 38.213 §7.1 与 38.215 测量解释。无 TPC 状态机或完整测量滤波 |
| `channel.cpp` | 静态整数延迟 SISO FIR、CFO、AWGN、RMS 时延 | 信号处理教学；不是 38.901 TDL/CDL 或数字孪生 |
| `bench.cpp` | SoA 复乘标量、x86 AVX512F 分派分支、SVE 条件编译分支、尾部处理、参考值对比 | ISA 参考见教程第 30 章。本机只执行了 portable 分支；无 AMX 内核 |

`Config` 的检查仅覆盖实验的数据形状与范围，**不替代完整 NR 合法配置验证**。默认配置是教程第 12 章的受限场景；修改配置时需另查 Mapping Type、DM-RS 位置表及 DCI/RRC 的合法组合。

## 端到端实验的含义

```
随机调制级输入比特 G
 -> 加扰 -> QAM -> 数据/DM-RS 映射 -> IFFT+CP
 -> 常数复信道 + 时域 AWGN
 -> 去CP+FFT -> 全导频平均 LS -> 单抽头均衡
 -> max-log LLR -> LLR解扰 -> 硬判决 -> 输入比特 BER
```

输入是速率匹配之后那一级的随机比特，没有合法 LDPC 码字约束。`coding` 演示中用于速率匹配的是显式标注的合成输入。因此不能报告 TB CRC 成功率、编码增益、HARQ 增益或 PUSCH BLER。

QAM 的等概率星座平均 Es=1；FFT/IFFT 都除以 sqrt(N)，因此频域和时域白噪声方差一致。信道 h 后设置复噪声方差 `sigma2=|h|²*10^(-SNR/10)`。I/Q 各占 `sigma2/2`。这是数据 RE 上的 Es/N0，不是包含空闲子载波、CP、导频和编码率的波形总功率 SNR 或 Eb/N0。16QAM 的单个随机批次平均能量可能偏离 1。

LS 采用所有 DM-RS 估计一个常数 h；只适用于此平坦静态模型。LLR 用 `sigma2/|h_hat|²`，未包含估计误差方差，属于教学近似。LLR 正数表示更相信 bit=0；解扰对应符号翻转。`rate_recover` 的 `source_positions` 已包括解交织关系，在真实接收机中必须从授权独立重建，不能由发射机偷偷传递。

`tapped_channel` 输出与输入等长：历史为零，尾部截断，CFO 相位从样点 0 开始。真实流式信道需跨块保存历史与累计相位。此 FIR 的重复延迟需先合并复增益，才能把离散抽头能量解释为功率时延分布；当前示例使用互不相同的延迟。

## 对接 LDPC 后端

接口 `LdpcEncoderBackend::encode(cb, plan)` 的输入是 K 个 unpacked bits，filler 值置 0；输出为 N 个比特，**已经删掉前 2Zc 位**。接口只是契约，没有可直接运行的后端实现。对接流程：

1. 根据 BG、Zc 和 iLS 选取规范的移位矩阵，编码并检查完整校验方程 `H*x=0`。
2. 对比独立参考的编码输出；检查是否已截去 2Zc，避免重复截断。
3. 对 N 长输出应用 `filler_mask_after_puncture`，相应位置改为 NULL，再速率匹配。
4. RX 解调/解扰后按同一配置重建选位，回填 N 长 LLR；各次 RV 对应相同母码位置后才能 HARQ 合并。
5. 若译码器需要完整 68Zc/52Zc 位置，前补 2Zc 个零 LLR 表示擦除；filler 置译码器约定的“已知 0”正饱和值。译码器接口若自行补位，不重复处理。
6. 译码后去 filler、检查 CB CRC（有分段时）、重组 TB、检查 TB CRC；按 MAC HARQ 的 NDI/进程状态处理缓冲。

开源实现可从 [srsRAN Project](https://github.com/srsran/srsRAN_Project) 的 PHY 模块阅读；选择版本、能力和许可证后再集成，不把任意源码片段当作稳定 API。也可用已有 5G 工具生成独立测试向量，保留工具版本和配置。

## 测试与结果解释

`test_main.cpp` 使用显式异常检查，Release 构建也有效。覆盖已知数值、CRC 长除法、独立 packed Gold、全 QAM 标签、直接 DFT、无噪声回环、RV/NULL/交织/重复回填、信道冲激、输入拒绝。

这些是学习实现的回归测试，尚未以独立完整 PUSCH 工具或仪表做跨实现一致性测试，更不是一致性认证。不要把多个回环通过等同于规范正确：TX 和 RX 共享的错误也可能抵消。

随机数引擎固定种子；C++ 标准库的正态分布变换不保证不同平台逐样点一致。要跨平台比较，保存实际输入、噪声样点、期望输出及浮点容差，而不仅保存 seed。

`results/` 保存本次运行结果和说明。`pusch_bench` 报告 101 个批次中每次调用平均耗时的 p50/p95/p99；不是逐次时隙的尾延迟。portable 源码可能被编译器自动向量化，其结果不能称为纯标量硬件性能。AVX512 与 SVE 分支需要对应硬件重新验证。

## 下一步任务

- 先扩展单符号映射与 LDPC 后端，完成 TB → TB 无噪声回环。
- 再加入多径频域估计、频偏/定时、双天线合并、多层均衡与 UCI。
- 最后做流式缓冲/任务调度、Linux 性能追踪、SIMD 数值回归和 38.901 信道验证。
- 所有变更保持规范版本、配置、测试向量、随机样本、性能环境可追溯。详细验收标准见教程第 31 章。
