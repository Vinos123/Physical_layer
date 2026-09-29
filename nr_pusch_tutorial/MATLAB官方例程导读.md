# MATLAB / 5G Toolbox 官方例程导读

按你的偏好，完整 NR 链路使用 MathWorks 官方已有 MATLAB 示例；本教程新增程序全部使用 C++。这里提供阅读、配置与实验步骤，不另造一套 MATLAB 收发机。

本导读查阅的是在线 R2026b 文档。使用页面上的 **Open Script / Open Live Script**，或从本机 MATLAB 的 Examples 搜索同名示例；若版本较旧，应选择该版本文档。官方示例是否可运行取决于本机产品和版本，本次未在 MATLAB 中实测。

## 1. 先看资源图：NR PUSCH Resource Allocation and DM-RS and PT-RS Reference Signals

打开[官方示例](https://www.mathworks.com/help/5g/ug/nr-pusch-resource-allocation-and-dmrs-and-ptrs-reference-signals.html)。先运行原始配置，找到载波对象、PUSCH 对象、导频生成、索引生成及绘图位置。

建议第一次只阅读 CP-OFDM 部分，关闭 PT-RS 与跳频，单层/端口 0。建立下列配置用于对照教程主实例：

| 配置字段/对象 | 设置 | 观察目标 |
|---|---|---|
| `carrier.SubcarrierSpacing` | 30 | 每时隙时长 0.5 ms |
| `carrier.CyclicPrefix` | normal | 每时隙 14 符号 |
| `carrier.NSizeGrid` / `NStartGrid` | 52 / 0 | 保持足够大的网格和明确的原点 |
| `pusch.NSizeBWP` / `NStartBWP` | 52 / 0 | 与载波网格一致，减少坐标转换 |
| `pusch.PRBSet` | 0:19 | 20 PRB；这里改用起点 0 方便与 C++ DM-RS 序列对照 |
| `pusch.SymbolAllocation` / `MappingType` | [2 12] / A | 数据分配符号 2–13 |
| `pusch.TransformPrecoding` / `NumLayers` | false / 1 | CP-OFDM、单层 |
| `pusch.TransmissionScheme` | nonCodebook | 本练习单层，不研究空间码本 |
| `pusch.Modulation` | 16QAM | 与主例 TBS 参数一致 |
| `pusch.DMRS.DMRSConfigurationType` | 1 | 梳状频域图案 |
| `pusch.DMRS.DMRSTypeAPosition` | 2 | 前置 DM-RS 在符号 2 |
| `pusch.DMRS.DMRSLength` / `DMRSAdditionalPosition` | 1 / 1 | 本配置对应位置 2、11 |
| `pusch.DMRS.NumCDMGroupsWithoutData` | 1 | DM-RS 符号剩余半数 RE 可用 |
| `pusch.DMRS.DMRSPortSet` | 0 | 与 C++ 端口假设一致 |
| `pusch.EnablePTRS` | false | 第一次先排除 PT-RS 影响 |

官方 Live Script 后面的单元可能再次改写对象属性，所以要在实际生成索引之前确认值。期望结果为 240 个 DM-RS RE、2640 个数据 RE，16QAM 单层容量 G=10560。这里的数值是手算/C++ 验证的预期值，不是声称已执行 MATLAB。

第一轮实验只改一个变量：把 `NumCDMGroupsWithoutData` 从 1 改为 2，数据 RE 应减少到 2400。不要把此变化解释为“实际发送端口数自动翻倍”。第二轮再启用 PT-RS，观察新增保留 RE；第三轮打开 transform precoding，观察参考信号结构变化，不能继续套用 CP-OFDM 的 RE 图案。

## 2. 再看完整链路：NR PUSCH Throughput

打开 [NR PUSCH Throughput 官方示例](https://www.mathworks.com/help/5g/ug/nr-pusch-throughput.html)。这一步学习真正的 UL-SCH 编码与接收闭环，是 C++ 原理实验之外的核心实践。

按下列顺序阅读代码，避免一开始陷入信道模型和绘图细节：

1. 找到 `nrPUSCHIndices` 返回的资源信息与 `nrTBS` 调用，确认谁决定 G、谁决定 A。
2. 找到设置运输块和 UL-SCH 编码的位置，追踪 RV 与目标码率。
3. 找到 PUSCH/DM-RS 网格映射与 OFDM 调制的位置。
4. 找到同步、信道估计、均衡、软解调及 UL-SCH 译码的位置。
5. 找到 CRC 结果如何更新 HARQ 状态和吞吐统计。

先保留官方默认参数跑通，再进行以下实验。每组保存配置、MATLAB/工具箱版本、随机种子、仿真帧数和结果。

| 组别 | 只改变什么 | 需要观察什么 | 解释问题 |
|---|---|---|---|
| A | 理想/实际信道估计 | CRC、吞吐、EVM | 性能损失是否来自估计误差？ |
| B | 调制阶数和目标码率 | 同资源下 TB 大小与成功交付速率 | 更高 MCS 是否真的提高有效吞吐？ |
| C | HARQ 开/关 | 首传错误、最终交付、重传次数 | 可靠性提高消耗了多少资源？ |
| D | DM-RS 附加位置 | 高移动性下性能与数据开销 | 导频密度是否值得增加？ |
| E | 层数与天线配置 | 分层 EVM、错误率 | 信道是否支持更多独立流？ |

对照教程主例时，将上述载波/PUSCH 参数同步到官方脚本中的相应结构，目标码率设为 490/1024、额外 TBS 开销设为 0。完整示例可能有自己的默认 DM-RS、接收天线数及信道条件，不能只改 MCS 就宣称复现了主例。

统计解释遵循本教程的独立实验建议：先用少量帧排查配置和尺寸，再增加样本做性能估计；分别统计新 TB 和发送次数；明确 SNR 是每 RE、每接收天线还是某种 Eb/N0。不同口径的横轴不能直接比较。

## 3. 使用官方函数做小范围对拍

这部分是对拍计划，不代表本次已经完成跨实现验证。

| C++ 结果 | 官方接口 | 对齐条件与预期 |
|---|---|---|
| TBS=4992 | [nrTBS](https://www.mathworks.com/help/5g/ref/nrtbs.html) | 16QAM、1层、20PRB、每PRB可用132RE、R=490/1024、无额外开销 |
| G=10560 | [nrPUSCHIndices](https://www.mathworks.com/help/5g/ref/nrpuschindices.html) | 与第一节的完整资源配置相同，无UCI/PT-RS |
| BG1、Zc=240、K=5280 | [nrDLSCHInfo](https://www.mathworks.com/help/5g/ref/nrdlschinfo.html) | 比較共有 LDPC 尺寸过程，A=4992、R=490/1024；字段以本版文档为准 |
| `results/dmrs.csv` | [nrPUSCHDMRS](https://www.mathworks.com/help/5g/ref/nrpuschdmrs.html) | nslot=0、NIDNSCID=42、NSCID=0、PRB从0开始、端口0、基本DM-RS |

MATLAB 线性索引按列优先，C++ 本例数组用 `l*240+k`，CSV 明确记录 l 和 k。逐元素比较前必须按相同 RE 坐标排序；不要直接比较两个不同展开顺序的一维数组。

如果 TBS 相同而 G 不同，先检查 DM-RS 无数据组、PT-RS、资源保留和调制/层数。如果 G 相同而导频不一致，再检查 ID、nSCID、slot、端口以及序列参考起点。

## 4. 进阶：跨时隙 TB

完成单时隙链路后，可阅读 [NR PUSCH Throughput with Transport Block over Multiple Slots](https://www.mathworks.com/help/5g/ug/nr-pusch-throughput-with-transport-block-over-multiple-slots.html)，页面标明自 R2026b 提供。重点比较“同一 TB 跨多个时隙承载”与“同一 TB 重复发送”的区别，追踪编码块、速率匹配与 HARQ 的统计单位。C++ 主例的单时隙容量公式不应未经修改直接用于此场景。
