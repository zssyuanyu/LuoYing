# 络樱 (LuoYing)

> **⚠️ 内测版 (Beta) 声明**
> 本项目处于早期开发阶段，功能尚不完善，可能存在 Bug、性能问题和不稳定的行为。
> 欢迎试用和反馈，但请勿用于生产环境或关键任务。

络樱是一个**完全离线、轻量级、本地运行的桌面 AI Agent**。它不需要联网，所有推理都在你的电脑上完成，用 C++ 和 Qt 编写，追求极致的资源占用。

---

## 特性

- **完全离线**：不连外网，所有对话和数据都留在本地
- **流式输出**：回复逐字显示，接近云端 AI 的交互体验
- **工具调用**：能读写文件、查看系统信息、执行安全的查询命令
- **多模型支持**：支持任意 GGUF 格式模型，可自由切换
- **自动管理**：启动时自动拉起推理服务，关闭时自动释放
- **轻量设计**：纯 C++ + Qt 编写，无 Python 运行时依赖

---

## 系统要求

| 项目 | 最低 | 推荐 |
|---|---|---|
| 操作系统 | Windows 10 64 位 | Windows 11 |
| 内存 | 8 GB | 16 GB |
| 磁盘 | 10 GB 可用 | 20 GB 可用 |
| 编译器 | MSVC 2022 (VS Build Tools) | 同左 |
| Qt | Qt 6.5+ (MSVC 版本) | Qt 6.11 |
| 显卡 | 无要求（CPU 推理） | 集成显卡即可 |

> **内存建议**：模型越大，对内存要求越高。8 GB 内存推荐 3B 模型，16 GB 推荐 7B 模型。

---

## 快速开始

### 方式一：下载发布版（推荐）

1. 去 [Releases 页面](https://github.com/zssyuanyu/LuoYing/releases) 下载最新的 `LuoYing.zip`（约 64 MB）
2. 解压到任意目录
3. 双击 `download_model.bat`，按菜单提示选择下载源：
   - **选项 1（推荐）**：HuggingFace 镜像（hf-mirror.com），单文件，支持断点续传
   - **选项 2**：ModelScope（官方源），分片下载后自动合并
4. 等待下载完成（约 4.68 GB，10-30 分钟）
5. 双击 `LuoYing.exe` 启动络樱

### 方式二：从源码编译

如果你熟悉 C++ 开发，可以自行编译：

```bash
git clone https://github.com/zssyuanyu/LuoYing.git
cd LuoYing
```

用 Qt Creator 打开 `CMakeLists.txt`，选择 MSVC 2022 64-bit 构建套件，编译即可。

**依赖**：
- Qt 6.5+（Widgets + Network 模块）
- MSVC 2022 或更高版本
- CMake 3.16+
- [llama.cpp](https://github.com/ggml-org/llama.cpp)（编译出 `llama-server.exe`）

---

## 模型下载

### 自动下载（推荐）

双击 `download_model.bat`，脚本会引导你选择下载源。

### 手动下载

如果自动脚本下载失败，可以手动下载模型。

**官方下载链接：**

| 来源 | 链接 | 说明 |
|---|---|---|
| **ModelScope（推荐）** | [qwen/Qwen2.5-7B-Instruct-gguf](https://www.modelscope.cn/models/qwen/Qwen2.5-7B-Instruct-gguf/files) | 国内官方源，稳定 |
| **Hugging Face** | [bartowski/Qwen2.5-7B-Instruct-GGUF](https://huggingface.co/bartowski/Qwen2.5-7B-Instruct-GGUF) | 国际源，可能需网络工具 |
| **Hugging Face 镜像** | [hf-mirror.com](https://hf-mirror.com/bartowski/Qwen2.5-7B-Instruct-GGUF) | 国内镜像，速度快 |

**需要的文件名：**

- 从 ModelScope 下载时，需要下载两个分片文件，再用下面的命令合并：
  ```
  copy /b qwen2.5-7b-instruct-q4_k_m-00001-of-00002.gguf + qwen2.5-7b-instruct-q4_k_m-00002-of-00002.gguf Qwen2.5-7B-Instruct-Q4_K_M.gguf
  ```
- 从 Hugging Face 或镜像下载时，直接下载 `Qwen2.5-7B-Instruct-Q4_K_M.gguf`（单文件，约 4.68 GB）。

**目录结构：**

```
LuoYing/
├── LuoYing.exe
├── download_model.bat
├── llama-server.exe
├── models/
│   └── Qwen2.5-7B-Instruct-Q4_K_M.gguf   ← 模型放这里
└── ...
```

放好后，双击 `LuoYing.exe` 即可启动。

---

## 使用其他模型

络樱支持**任意 GGUF 格式的模型**。只需把 `.gguf` 文件放进 `models\` 文件夹：

- **放一个模型**：络樱直接加载它，不弹窗。
- **放多个模型**：启动时弹出选择框，可以自由切换。
- **记住选择**：络樱会记住你上次选的那个，下次自动选中。

**推荐模型**（按硬件需求从低到高）：

| 模型 | 文件大小 | 建议内存 | 说明 |
|---|---|---|---|
| Qwen2.5-3B-Instruct-Q4_K_M | 2.1 GB | 8 GB | 轻量，速度快 |
| Qwen2.5-7B-Instruct-Q4_K_M | 4.68 GB | 16 GB | 推荐，平衡之选 |
| Llama-3.2-3B-Chinese-Elite-Q4_K_M | 2.0 GB | 8 GB | 中文优化 |
| Qwen2.5-14B-Instruct-Q4_K_M | 9 GB | 32 GB | 更强，需高配 |

把任意 `.gguf` 文件放进 `models\` 即可，络樱会自动识别。

---

## 已实现的工具

络樱目前支持以下工具，全部在本地执行：

| 工具 | 功能 | 安全限制 |
|---|---|---|
| `get_current_time` | 获取当前时间 | — |
| `read_file` | 读取文件 | 系统目录拒绝 |
| `list_directory` | 列出目录 | 系统目录拒绝 |
| `get_system_info` | 查询 CPU、内存、磁盘 | — |
| `write_file` | 写入文件 | 只能写工作目录内 |
| `create_directory` | 创建目录 | 只能建在工作目录内 |
| `execute_command` | 执行只读命令 | 黑名单 + 白名单双重过滤 |
| `get_file_info` | 文件详情 | 系统目录拒绝 |
| `search_files` | 搜索文件 | 系统目录拒绝 |
| `delete_file` | 删除文件 | 只能删工作目录内 |
| `copy_file` | 复制文件 | 目标必须在工作目录内 |
| `move_file` | 移动文件 | 源和目标都在工作目录内 |
| `open_file` | 默认程序打开 | 系统目录拒绝 |

---

## 使用示例

- `现在几点了？`
- `看看当前目录有哪些文件`
- `我电脑还剩多少内存？`
- `创建文件 notes.txt，内容是"今天学习了 C++"`
- `读一下 CMakeLists.txt`
- `帮我找找有哪些 .cpp 文件`
- `用记事本打开 CMakeLists.txt`

---

## 项目结构

```
LuoYing/
├── CMakeLists.txt
├── main.cpp
├── mainwindow.cpp
├── mainwindow.h
├── mainwindow.ui
├── README.md
├── LICENSE
└── .gitignore
```

---

## 模型来源与许可

络樱的推理能力基于 **Qwen2.5-7B-Instruct** 模型，该模型采用 **Apache License 2.0** 许可协议发布，版权归阿里巴巴云所有。

络樱是独立的开源项目，**与阿里巴巴或 Qwen 官方无任何关联**。

---

## 许可证

本项目采用 **Apache License 2.0** 许可协议，详见 [LICENSE](LICENSE)。

---

## 贡献与反馈

欢迎提交 Issue 反馈 Bug 或建议新功能。

**反馈时请提供：**
- 操作系统版本
- 使用的模型
- 复现步骤
- 错误截图或日志

---

## 免责声明

- 络樱处于 **内测阶段**，功能不完善，可能存在数据丢失或异常行为。
- 请在使用前备份重要文件。
- 工具调用有路径和命令限制，但仍建议只在非关键环境下运行。
- 使用本软件产生的任何后果由使用者自行承担。

---

**络樱** · 一个属于你的本地 AI Agent
