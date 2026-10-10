# PUSCH 边写边学：第 19–31 章配套练习

主教程：[5G PUSCH 从零开始](../5G_PUSCH_从零开始.md#ch19)。本目录与 `cpp/` 并列放置，构建参考检查需要 `cpp` 的头文件和源文件；不依赖下载 srsRAN，也不需要射频设备。

## 使用方式

在项目根目录执行：

```bash
cmake -S learning_path -B build-learning -DCMAKE_BUILD_TYPE=Debug
cmake --build build-learning --parallel
./build-learning/nr_labs L01
```

编辑 `learning_path/exercises/student.cpp`。初始版本可以编译，运行会显示 TODO 并返回非零退出码。每完成一个函数，运行对应实验；`all` 运行全部实验。测试是起点，需按章节增加边界、随机、独立向量和错误注入检查。

```bash
ctest --test-dir build-learning -R '^L01$' --output-on-failure
./build-learning/nr_labs all
```

## 十个练习

| ID | 章节 | 函数 | 学习重点 |
| --- | --- | --- | --- |
| L01 | 20 | data_re | 资源格坐标、DM-RS 预留、映射顺序 |
| L02 | 21 | gold | 两个31阶序列、初值位序、Nc=1600 |
| L03 | 21 | qam16 | 星座位标签、平均功率 |
| L04 | 22 | dmrs_init | DM-RS 初始化、宽整数、模2^31 |
| L05 | 23 | flat_ls | 复数共轭、导频 LS、分母能量 |
| L06 | 24 | crc16 | GF(2)、位序、CRC 独立已知向量 |
| L07 | 25 | check_node | normalized min-sum、重复最小值、零 |
| L08 | 26 | rm_positions | RV、母码坐标、比特交织 |
| L09 | 26 | combine | 重复观测相加、软缓冲坐标 |
| L10 | 27 | qpsk_llr | 软值符号、复噪声方差 |

## 参考解

使用独立构建目录，避免覆盖自己的学生版本配置：

```bash
cmake -S learning_path -B build-learning-answer -DNR_USE_SOLUTIONS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-learning-answer --parallel
ctest --test-dir build-learning-answer --output-on-failure
```

`solutions.cpp` 中部分函数调用既有 `pusch_lab` 参考实现；新小算子直接给出实现。它们不是 srsRAN 代码副本。测试包含手算值、已知 CRC 向量、Gold 的独立压紧寄存器表示及边界用例；没有完成全配置一致性验证。学生实现应扩大覆盖，尤其全部 QAM 标签、非法比特、长度、NaN 和边界。

## 源码阅读

- [源码导读与固定链接](源码导读.md)
- [来源及文件 SHA-256](sources.json)
- [本次验证记录](验证记录.md)

规范基线：TS 38.211–38.215 V18.5.0。源码基线：srsRAN Project `release_25_10`，commit `d2f4b70dda8e2c557d5b05a0ac5f92dbddda19bc`。二者的版本号不代表功能范围相同，以具体条款及源码能力为准。

此目录提供入门到核心算子的练习，完整 NR LDPC、UCI、编码 PUSCH 闭环与目标 CPU 优化仍是主教程中的后续工程任务。本次未构建或运行完整 srsRAN 项目。
