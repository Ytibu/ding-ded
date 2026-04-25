
# Glyph-editor

## 项目简介
Glyph-editor 是一款基于 OpenGL/SDL3 的图形化代码编辑器，支持丰富的渲染特效，旨在让用户无需关注底层 OpenGL 细节即可开发编辑器功能。

**主要特性：**
- 以c++面向对象和GLSL实现，结构清晰，易于扩展
- 基于 SDL3/glew/FreeType，跨平台（当前主要支持 Windows）
- 高性能文本渲染，支持 Victor Mono 等编程字体
- 代码结构模块化，便于学习和二次开发

> **开发环境**  
> - 平台：Windows 11 x64  
> - GPU：NVIDIA GeForce RTX 4060  


## 编译与运行

```sh
# 编译
make -B

# 运行
./te.exe
```

> **注意：**
> - 需将 SDL3、glew32、freetype 等 DLL 放置于可执行文件同目录或系统 PATH 下。
> - 如遇依赖库缺失，请根据下方依赖说明下载安装。

---

## 依赖说明

- **SDL3**（窗口/输入/事件）  
	https://github.com/libsdl-org/SDL
- **OpenGL/glew**（渲染）  
	https://glew.sourceforge.net
- **FreeType**（字体渲染）  
	https://freetype.org/download.html
- **Victor Mono**（编程字体，推荐）  
	https://rubjo.github.io/victor-mono

> **Windows 平台依赖 DLL**：
> - SDL3.dll
> - glew32.dll
> - freetype.dll

---

## 功能模块简介

- **文本编辑核心**：支持多行文本、插入、删除、光标移动等基本编辑操作
- **OpenGL 渲染**：自定义着色器，支持高效文本与光标渲染
- **字体与排版**：集成 FreeType，支持矢量字体渲染
- **文件读写**：支持文件的加载与保存
- **可扩展性**：C++ 版本采用面向对象设计，便于功能扩展

---

## 致谢与声明

本项目灵感与核心思路源自 [tsoding](https://github.com/tsoding) 于 2021 年使用 SDL2 开发的 [ded 项目](https://github.com/tsoding/ded)。

**与原项目主要区别：**
- 底层框架：SDL2 → SDL3
- 编程范式：C 风格 → C++ 面向对象封装
- 代码结构：完全重构，层次差异大
- 运行效果：与原项目几乎一致

特此声明并感谢原作者的贡献。