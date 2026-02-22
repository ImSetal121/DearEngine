# Dear Engine

基于 **SDL3**、**OpenGL**、**Dear ImGui** 的跨平台引擎与编辑器，实体-组件架构，支持场景编辑、运行预览与 3D 场景视口。技术栈：C++26、CMake；中文界面与文档。

## 构建与运行

需 CMake 3.16+、C++26、OpenGL。

```bash
git clone --recursive <repo-url>
cd DearEngine
mkdir build && cd build
cmake ..
cmake --build .
# 在项目根或 build 下运行
./<Configuration>/DearEngine
```

资源路径由编译期 `CONTENT_ROOT` 注入，不依赖运行时 cwd。

## 项目结构

- `src/main.cpp`：入口（SDL_AppInit/Event/Iterate/Quit）
- `src/application/`：运行预览应用（DA::Application）
- `src/engine/`：Engine、Entity/Scene/Component、编辑器窗口（Console、SceneTree、EntityComponent、SceneViewport、AssetManager）、assets、util

详细渲染与扩展见 **docs/场景视口渲染学习指导.md**。

## 许可证

见仓库 LICENSE；依赖（SDL3、ImGui、GLM、GLAD 等）遵循各自许可。
