# 4G/5G Physical Layer 学习练习

围绕 4G/5G 物理层开发所需的 C++ 与基础信号处理知识开展练习，当前内容包括资源管理、函数模板、Q15 定点数、IQ 信号能量、FIR 滤波和 IQ 文本文件读写。项目目前是独立练习程序与一个共用静态库，尚未实现完整的 4G/5G 物理层链路。

本文以根目录的 `*_practice.cpp` 为主，并结合 `main.cpp`、`include/phy.h`、`src/phy.cpp` 和 `CMakeLists.txt` 说明使用方式。

## 代码结构

```text
.
├── CMakeLists.txt          # C++20、可执行目标及 phy 静态库配置
├── main.cpp               # array/vector、IQ 能量与容量管理练习
├── raii_practice.cpp       # RAII、数据拷贝与独占所有权转移
├── templates_practice.cpp  # 浮点向量加法、标量乘法与异常处理
├── q15_practice.cpp        # Q15 转换、饱和乘法与定点能量计算
├── fir_practice.cpp        # FIR 单位冲激响应验证
├── debug_practice.cpp      # IQ 能量计算断点练习
├── iqfile_practice.cpp     # IQ 文件写入、读回与一致性检查
├── include/phy.h           # 共用信号处理接口
├── src/phy.cpp             # 共用接口实现
└── output/iq_samples.txt   # IQ 文本样本
```

## 构建与运行

需要支持 C++20 的编译器、CMake 4.3 或更高版本，以及 Ninja。CMake 版本要求来自当前 `CMakeLists.txt`。

在项目根目录执行：

```bash
cmake -S . -B build-practice -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-practice --parallel
```

当前目录名 `4G:5G_physical_layer` 含有冒号，使用 Unix Makefiles 构建时可能出现 `target pattern contains no '%'`，建议使用 Ninja。若 Ninja 未加入 PATH，可在配置命令中通过 `-DCMAKE_MAKE_PROGRAM=/实际路径/ninja` 指定；CLion 中也可使用 IDE 自带的 Ninja。

各练习拥有独立的 `main()`，分别运行对应目标：

```bash
./build-practice/raii_practice
./build-practice/templates_practice
./build-practice/q15_practice
./build-practice/fir_practice
./build-practice/debug_practice
./build-practice/iqfile_practice
```

也可以只构建一个目标，例如：

```bash
cmake --build build-practice --target fir_practice
```

在 CLion 中打开根目录 `CMakeLists.txt`，加载 CMake 项目后选择相应目标运行或调试。运行 IQ 文件练习时，注意配置中的工作目录决定相对输出路径。

## 练习内容

### 1. RAII 与所有权：`raii_practice`

使用 `std::make_unique<Frame>` 管理包含采样向量的对象，观察构造、析构及资源自动释放。

- 拷贝 `owner->samples` 会创建独立存储，修改副本后，原始首元素仍为 `1`，副本首元素为 `9`。
- `std::move(owner)` 将独占所有权交给 `next_owner`，原指针变为空，新对象中的采样数仍为 `4`。
- 作用域结束时自动调用析构函数，无需手动 `delete`。

### 2. 函数模板：`templates_practice`

实现向量逐元素加法 `add<T>` 与向量乘标量 `multiply<T>`，通过 `static_assert` 将类型限制为浮点类型。向量加法要求长度相等，否则抛出 `std::invalid_argument`。

当前示例打印：

```text
add float result = {4, 6}
add double result = {0.85, 1}
caught: length mismatch
a * b = {1.8, 0.6}
```

**当前限制：** 末尾自检使用浮点数精确相等比较，`3 * 0.6` 与字面量 `1.8` 可能具有不同的二进制表示。因此，即使打印结果如上，程序仍可能返回 `1`；本地 Apple Clang 验证中出现了这一情况。改进练习时可采用误差容限比较。

### 3. Q15 定点数：`q15_practice`

本练习使用有符号 16 位整数表示 Q15 数值：

```text
实际值 = 原始整数 / 32768
表示范围 = [-1, 32767/32768]
```

- `to_q15`：拒绝非有限输入，将数值限制在可表示范围内，再缩放、四舍五入。
- `q15_to_double`：将 Q15 原始整数还原为浮点值。
- `multiply_q15`：使用 32 位中间乘积，缩放后向零截断，并对结果做饱和处理。
- 对一个周期的 100 个正弦采样分别计算浮点能量与定点能量；定点平方和使用 64 位整数累加，最后除以 `32768²`。

关键结果包括 `Q15(0.5) = 16384`、`Q15(0.25) = 8192`，以及 `(-1) × (-1)` 饱和到 `32767`。浮点能量为约 `50`，定点结果接近该值。

注意：代码中打印标签 `relative error` 对应的实际计算是绝对误差 `abs(fixed_energy - energy)`。

### 4. FIR 滤波：`fir_practice`

调用 `phy::fir`，用单位冲激验证滤波器系数：

```text
输入：  [1, 0, 0, 0, 0]
系数：  [0.25, 0.5, 0.25]
输出：  [0.25, 0.5, 0.25, 0, 0]
```

实现按 `y[n] = Σ h[k] × x[n-k]` 计算因果 FIR，块前输入视为零。输出长度与输入相同，不保留卷积尾部，各次调用之间不保存状态。空系数会抛出异常；系数非空时，空输入返回空输出。

### 5. 断点调试：`debug_practice`

使用 IQ 样本 `{3, 4}` 和 `{1, 0}` 调用 `phy::energy`，预期打印 `result = 26`。

可以在调用处或 `src/phy.cpp` 的能量累加语句处设置断点，逐步观察 `sample` 和 `sum`：

```text
初始值：0
第一个样本：0 + 3² + 4² = 25
第二个样本：25 + 1² + 0² = 26
```

### 6. IQ 文件读写：`iqfile_practice`

将三个 IQ 样本写入文本文件，再读回并比较，预期打印 `samples = 3` 和 `round trip equal = true`。

不传参数时，程序在**当前工作目录**下创建 `output/`，写入 `output/iq_samples.txt`。也可指定文件路径：

```bash
./build-practice/iqfile_practice /tmp/iq_practice_samples.txt
```

**指定路径同样会先写入并覆盖文件，再读取验证，并非只读取已有文件。** 自定义路径的父目录需要提前存在。

文件采用每行一对 `I Q` 浮点数的文本格式，例如：

```text
3 4
0.100000001 -0.200000003
1 0
```

写入使用足以往返还原 `float` 的精度。读取允许空行和行首、行尾空白，拒绝缺失字段、非有限数值和额外字段；格式错误信息包含行号。程序捕获异常后打印 `IQ file error: ...` 并返回 `1`。

## 共用 `phy` 静态库

`fir_practice`、`debug_practice` 和 `iqfile_practice` 链接 `phy`。CMake 通过 `PUBLIC` 头文件目录将 `include/` 传递给使用该库的目标。

| 接口 | 作用 |
| --- | --- |
| `phy::IQ` | `std::complex<float>`，实部表示 I，虚部表示 Q |
| `phy::energy(samples)` | 累加各 IQ 样本的 `I² + Q²`，返回 `double`；空输入返回 `0` |
| `phy::fir(input, taps)` | 对实数采样执行 FIR 滤波 |
| `phy::write_iq(path, samples)` | 检查样本并写入 IQ 文本文件 |
| `phy::read_iq(path)` | 读取并校验 IQ 文本文件，返回采样向量 |

`main.cpp` 对应额外目标 `4G_5G_physical_layer`，演示从 `std::array` 构造 `std::vector`、`reserve` 与 `push_back`，以及 IQ 能量计算。运行后可观察到 `reserve(8)` 不改变元素数量，追加样本后能量为 `30`，空向量能量为 `0`。

## 验证方式

当前各程序在 `main()` 中包含简单自检，通常返回 `0` 表示通过，返回 `1` 表示检查失败；尚未注册 CTest 测试。`templates_practice` 的浮点精确比较问题见上文。

本地使用 Apple Clang 21、CMake 4.4.3 和 Ninja 完成构建。运行结果：`raii_practice`、`q15_practice`、`fir_practice`、`debug_practice`、`iqfile_practice` 及主程序均返回 `0`；`templates_practice` 打印预期数值，但返回 `1`。
