\# 络樱 (LuoYing)



> \\\*\\\*⚠️ 内测版 (Beta) 声明\\\*\\\*

> 本项目处于早期开发阶段，功能尚不完善，可能存在 Bug、性能问题和不稳定的行为。

> 欢迎试用和反馈，但请勿用于生产环境或关键任务。



络樱是一个\*\*完全离线、轻量级、本地运行的桌面 AI Agent\*\*。它不需要联网，所有推理都在你的电脑上完成，用 C++ 和 Qt 编写，追求极致的资源占用。



\## 特性



\- \*\*完全离线\*\*：不连外网，所有对话和数据都留在本地。

\- \*\*工具调用\*\*：能读写文件、查看系统信息、执行安全的查询命令。

\- \*\*流式输出\*\*：回复逐字显示，接近云端 AI 的交互体验。

\- \*\*自动管理\*\*：启动时自动拉起推理服务，关闭时自动释放。

\- \*\*轻量设计\*\*：纯 C++ + Qt 编写，无 Python 运行时依赖。

\- \*\*深色界面\*\*：类似 DeepSeek 桌面端的现代观感。



\## 系统要求



| 项目 | 最低 | 推荐 |

|---|---|---|

| 操作系统 | Windows 10 64 位 | Windows 11 |

| 内存 | 8 GB | 16 GB |

| 磁盘 | 10 GB 可用 | 20 GB 可用 |

| 编译器 | MSVC 2022 (VS Build Tools) | 同左 |

| Qt | Qt 6.5+ (MSVC 版本) | Qt 6.11 |

| 显卡 | 无要求（CPU 推理） | 集成显卡即可 |



\## 已实现的工具



络樱目前支持以下工具，全部在本地执行：



| 工具 | 功能 | 安全限制 |

|---|---|---|

| `get\\\_current\\\_time` | 获取当前时间 | — |

| `read\\\_file` | 读取工作目录内的文件 | 只能读工作目录内 |

| `list\\\_directory` | 列出工作目录内容 | 只能列工作目录内 |

| `get\\\_system\\\_info` | 查询 CPU、内存、磁盘 | — |

| `write\\\_file` | 写入文件 | 只能写工作目录内 |

| `create\\\_directory` | 创建目录 | 只能建在工作目录内 |

| `execute\\\_command` | 执行只读类命令 | 黑名单 + 白名单双重过滤 |



\## 快速开始



\### 1. 下载模型



络樱需要 \*\*Qwen2.5-7B-Instruct-Q4\_K\_M\*\* 模型。从以下任一渠道下载：



\- \*\*ModelScope（国内推荐）\*\*：https://www.modelscope.cn/models/qwen/Qwen2.5-7B-Instruct-gguf

\- \*\*Hugging Face\*\*：https://huggingface.co/bartowski/Qwen2.5-7B-Instruct-GGUF



将下载的 `Qwen2.5-7B-Instruct-Q4\\\_K\\\_M.gguf`（约 4.68 GB）放到 `models/` 目录下。



\### 2. 编译 llama.cpp



络樱的推理后端是 \[llama.cpp](https://github.com/ggml-org/llama.cpp)。



```bash

git clone https://github.com/ggml-org/llama.cpp

cd llama.cpp

cmake -B build -DGGML\\\_VULKAN=ON   # 或去掉 Vulkan 用纯 CPU

cmake --build build --config Release


