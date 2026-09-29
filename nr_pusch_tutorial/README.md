# 5G NR 物理层教程：以 PUSCH 为主线

这套教程把 38.211–38.215 串成一个可计算、可运行、可排查的上行链路。起点是 ShareTechnote 的 [5G Handbook 索引](https://www.sharetechnote.com/html/5G/Handbook_5G_Index.html)，技术规则以 [固定版本的 3GPP/ETSI 标准](参考资料与标准索引.md)为准。内容为重新组织的中文讲解与原创数值实验，不是网页逐段翻译。

**适合谁：** 已接触复数、QPSK、FFT、AWGN、信道估计，希望进入 NR PHY 开发的读者。可以接着本项目 `examples_part2`、`examples_part3` 学习。

**学习基线：** 38.211/212/213/214 V18.5.0，38.215 V18.4.0，均为 Release 18、2025-01 出版版本。选择固定版本便于复核，不表示这些是最新版本。资料核查日期：2026-09-29 至 2026-09-30。

## 教程内容

| 文件 | 内容 | 建议用法 |
|---|---|---|
| [教程正文](教程正文.md) | 标准分工、资源与调度、TBS、编码、DM-RS、波形、MIMO、接收、HARQ、功控、测量 | 按章节学习，主实例贯穿全文 |
| [核心术语与公式](核心术语与公式.md) | 中英术语、变量口径、公式速查、易混概念 | 阅读标准和调试时查阅 |
| [MATLAB 官方例程导读](MATLAB官方例程导读.md) | 官方资源网格、完整 PUSCH 吞吐率和 TBoMS 示例 | 使用已有官方 MATLAB 例程 |
| [例程手册](例程手册.md) | 7 个原创 C++17 实验、预期输出、修改练习和答案 | 边读边运行 |
| [参考资料与标准索引](参考资料与标准索引.md) | 标准直达链接、重点条款、ShareTechnote 学习入口 | 按问题查标准 |
| [验证记录](验证记录.md) | 实测环境、数值结果、验证范围与未实现部分 | 判断代码适用范围 |
| [examples/nr_core.cpp](examples/nr_core.cpp) | 参数计算、序列、调制等教学函数 | 从小函数追踪公式 |
| [examples/nr_examples.cpp](examples/nr_examples.cpp) | 7 个实验入口 | 单独或一起运行 |

## 先跑起来

自编例程只需要 C++17 编译器，不依赖 MATLAB 或第三方库。官方 MATLAB 例程请按导读从 MathWorks 页面打开，需要相应版本与工具箱。

```bash
cd '/Users/Admin1/Desktop/4G:5G_physical_layer/nr_pusch_tutorial'
bash build_and_run.sh
```

脚本会编译并执行数值检查与全部实验。只运行例程 2：

```bash
./build/nr_examples --example 2 --out results
```

脚本把输出固定保存在教程的 `results/`，完整运行生成文本记录、资源网格/DM-RS、PAPR、信道估计和功率控制 CSV。也提供独立 CMake 工程，适合 CLion + Ninja；详见例程手册。

## 主实例

普通 CP、30 kHz SCS、单层单码字、关闭 transform precoding、连续 20 PRB、时隙内符号 2–13、DM-RS type 1、端口 0、单符号 DM-RS 位于 2 和 11、一个 CDM group without data、无 UCI/PT-RS/跳频/额外开销。MCS 使用适用的 64QAM 表中索引 12：16QAM，目标码率 490/1024。

从该配置得到 **2640 个数据 RE、10560 个编码比特、4992 bit 的 TB、BG1 和 Zc=240**。每个 0.5 ms 时隙都成功发送一个新 TB 时，TB 层速率为 **9.984 Mbit/s**；它不等于实际应用层吞吐率。

## 建议进度

1. 第 1 周：正文 1–3，术语表，例程 1；能解释 grant 如何变成资源网格。
2. 第 2 周：正文 4–5，例程 2；手算 TBS、CRC、BG、Zc、filler 和 G。
3. 第 3 周：正文 6–8，例程 3–5；把导频、DFT、空间预编码和接收机分清。
4. 第 4 周：正文 9–12，例程 6–7；理解 HARQ 状态、功率受限和测量口径。

每次学习均完成“画处理链、检查数组长度、复算一个数值、主动制造一个错误”四步。

## 代码的能力边界

这些 C++ 程序是可以独立运行的教学实验。TBS、LDPC 尺寸、Gold 序列等实现了明确限定的规则；OFDM、信道估计和软合并实验用于解释原理。**没有实现完整 NR LDPC 编解码、速率匹配/UCI 复用、DCI/RRC 协议栈或端到端 PUSCH BLER 仿真**。例程之间也不是一个完整收发机。每个实验的限制见例程手册；完整 NR 链路学习使用已列出的官方 MATLAB 示例，本次未在 MATLAB 中执行。

正文覆盖五份规范与 PUSCH 的核心关系，并给出扩展导航；没有逐条覆盖 Release 18 中全部下行、定位、侧链及增强特性。
