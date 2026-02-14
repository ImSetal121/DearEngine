# DearEngine 渲染后端切换至 OpenGL 学习指导

本文档面向「为学习目的将 DearEngine 从 SDL_GPU3 切换到 OpenGL」的读者，按步骤说明需要改动的模块和概念，便于你亲手完成切换并理解每一处修改的原因。

---

## 一、整体架构对比

### 1.1 当前架构（SDL_GPU3）

- **窗口**：`SDL_CreateWindow` 创建窗口，不要求 OpenGL。
- **渲染设备**：`SDL_CreateGPUDevice()` 创建抽象 GPU 设备，支持 SPIR-V / DXIL / Metal 等。
- **交换链**：`SDL_ClaimWindowForGPUDevice` 将窗口交给 GPU 设备，由 SDL_GPU 管理交换链。
- **ImGui 渲染**：`imgui_impl_sdlgpu3` 在 command buffer 上录制 ImGui 绘制，再在 render pass 里执行。
- **场景视口**：`SDL_GPUTexture` 作为渲染目标，用 `SDL_GPURenderPass` + pipeline 画三角形。

### 1.2 目标架构（OpenGL）

- **窗口**：创建窗口时加上 `SDL_WINDOW_OPENGL`，并先设置 `SDL_GL_*` 属性。
- **上下文**：`SDL_GL_CreateContext(window)` 得到 OpenGL 上下文，`SDL_GL_MakeCurrent` 绑定到当前线程。
- **呈现**：每帧 `glClear` → 自己的绘制 → `ImGui_ImplOpenGL3_RenderDrawData` → `SDL_GL_SwapWindow`。
- **ImGui 渲染**：`imgui_impl_opengl3` 直接调用 `gl*` 绘制 ImGui。
- **场景视口**：用 **FBO（帧缓冲对象）** 绑定一张 **GL 纹理** 作为渲染目标，用 **GL 着色器程序** 画三角形；在 ImGui 里用 `ImGui::Image((ImTextureID)(intptr_t)texture_id)` 显示该纹理。

### 1.3 概念对应关系（便于学习）

| SDL_GPU3 | OpenGL |
|----------|--------|
| SDL_GPUDevice | 无直接对应（由窗口 + GL 上下文承担） |
| SDL_GPUCommandBuffer / RenderPass | 直接 gl* 调用序列 |
| SDL_GPUTexture | GLuint 纹理 + glTexImage2D / glFramebufferTexture2D |
| SDL_GPUShader (SPIR-V/MSL) | glCreateShader + glShaderSource + glCompileShader |
| SDL_GPUGraphicsPipeline | glCreateProgram + glAttachShader + glLinkProgram + glUseProgram |
| 交换链呈现 | SDL_GL_SwapWindow |

---

## 二、需要修改的文件清单

| 文件 | 修改内容概要 |
|------|----------------|
| `src/CMakeLists.txt` | 换 ImGui 后端为 opengl3，链接 OpenGL |
| `src/Appstate.h` | 去掉 SDL_GPU 相关，增加 GL 上下文与场景 FBO/纹理/程序 |
| `src/main.cpp` | 用 SDL_GL 创建窗口与上下文，ImGui 用 OpenGL3 后端，主循环用 gl 呈现 |
| `src/engine/Engine.h` | RenderIterate 去掉 command_buffer 参数 |
| `src/engine/Engine.cpp` | Init/Quit/RenderIterate 改为 OpenGL 实现 |
| `src/engine/util/AboutGPU.*` 或新建 | 提供「从文件读 GLSL 并创建 program」的 OpenGL 工具 |
| `src/engine/window/SceneViewportWindow.cpp` | 将纹理以 ImTextureID 形式传给 ImGui（GL 纹理 id） |
| `src/application/Application.h` | RenderIterate 去掉 command_buffer 参数（若存在） |
| 着色器 | 使用 OpenGL 可用的 GLSL（见下文） |

---

## 三、分步修改说明

### 3.1 CMake：链接 OpenGL，更换 ImGui 后端

**文件：`src/CMakeLists.txt`**

1. **查找并链接 OpenGL**

在 `add_executable` 之后、`target_link_libraries` 处修改或添加：

```cmake
find_package(OpenGL REQUIRED)

target_link_libraries(DearEngine PRIVATE
        SDL3::SDL3
        ${OPENGL_LIBRARIES}
)
target_include_directories(DearEngine PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${OPENGL_INCLUDE_DIR}
)
```

2. **ImGui 后端**

- 从 `add_executable` 的源文件列表中**删除**：  
  `${IMGUI_DIR}/backends/imgui_impl_sdlgpu3.cpp`
- **添加**：  
  `${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp`

保留：`imgui_impl_sdl3.cpp`（负责窗口与输入）。

**学习要点**：  
- OpenGL 是系统/平台提供的 API，通过 `find_package(OpenGL)` 和 `OpenGL::OpenGL` 可跨平台链接。  
- ImGui 的「平台后端」负责输入和窗口，「渲染后端」负责把 ImGui 的顶点数据画到屏幕；这里平台仍是 SDL3，渲染从 SDL_GPU3 换成 OpenGL3。

---

### 3.2 AppState：用 OpenGL 状态替代 SDL_GPU 状态

**文件：`src/Appstate.h`**

1. **删除**所有 SDL_GPU 相关成员与头文件，例如：
   - `SDL_GPUDevice* gpu_device`
   - `SDL_GPUTexture* scene_viewport_texture`
   - `SDL_GPUShader* scene_vertex_shader` / `scene_fragment_shader`
   - `SDL_GPUGraphicsPipeline* scene_triangle_pipeline`

2. **增加** OpenGL 相关（需在能见到 GL 类型的翻译单元中包含 OpenGL 头，或使用前向声明 + 实现文件内包含）：

```cpp
#include "SDL3/SDL_video.h"  // SDL_GLContext

// 若希望头文件里直接使用 GL 类型，可包含（否则在 .cpp 里包含）：
// #include <SDL3/SDL_opengl.h>

struct AppState {
    SDL_Window* engine_window = nullptr;
    SDL_GLContext gl_context = nullptr;   // OpenGL 上下文

    bool application_is_running = false;
    Uint64 current_time_ns = 0;
    Uint64 delta_time_ns = 0;
    double current_time = 0;
    double delta_time = 0;

    // 场景视口：OpenGL FBO + 纹理
    int scene_viewport_texture_width = 1280;
    int scene_viewport_texture_height = 720;
    unsigned int scene_viewport_fbo = 0;        // GL FBO
    unsigned int scene_viewport_texture = 0;    // GL 纹理 id，供 ImGui::Image 使用
    unsigned int scene_program = 0;             // 场景用着色器程序（如画三角形）
    unsigned int scene_vao = 0;                 // 可选：顶点数组对象
    unsigned int scene_vbo = 0;                 // 可选：顶点缓冲

    ConsoleWindow* console_window = nullptr;
    SceneTreeWindow* scene_tree_window = nullptr;
    EntityComponentWindow* entity_component_window = nullptr;
    SceneViewportWindow* scene_viewport_window = nullptr;
};
```

若你不想在头文件里写 `unsigned int`（与 GLuint 等价），可以用 `void*` 或 `uint32_t`，在 Engine 的 .cpp 里再转成 GL 类型；上面写法最直观，便于学习。

**学习要点**：  
- OpenGL 是状态机，没有「Device」对象；「当前上下文」由 `SDL_GL_MakeCurrent` 绑定。  
- 离屏渲染到纹理 = FBO + 绑定的纹理附件；`scene_viewport_texture` 既是渲染目标，也是 ImGui 要显示的贴图。

---

### 3.3 main.cpp：从 SDL_GPU 初始化改为 OpenGL 初始化与主循环

**文件：`src/main.cpp`**

#### 3.3.1 头文件

- **删除**：`#include "imgui_impl_sdlgpu3.h"` 以及所有 `SDL3/SDL_gpu.h`（若存在）。
- **添加**：  
  `#include "imgui_impl_opengl3.h"`  
  `#include <SDL3/SDL_opengl.h>`（若在 main 里直接调用 gl 接口）。

#### 3.3.2 SDL_AppInit 中「创建窗口」之前

在 `SDL_CreateWindow` **之前**设置 OpenGL 属性并决定 GLSL 版本（与 ImGui OpenGL3 后端一致）：

```cpp
// 根据平台选择 GL 版本（与 imgui examples/example_sdl3_opengl3 一致）
#if defined(__APPLE__)
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
```

#### 3.3.3 创建窗口时加上 SDL_WINDOW_OPENGL

```cpp
SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
state->engine_window = SDL_CreateWindow("Dear Engine", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
// ... 错误检查 ...
SDL_SetWindowPosition(state->engine_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
SDL_ShowWindow(state->engine_window);
```

**删除**整段：`SDL_CreateGPUDevice`、`SDL_ClaimWindowForGPUDevice`、`SDL_SetGPUSwapchainParameters`。

#### 3.3.4 创建 OpenGL 上下文并设为当前

```cpp
state->gl_context = SDL_GL_CreateContext(state->engine_window);
if (state->gl_context == nullptr) {
    printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
}
SDL_GL_MakeCurrent(state->engine_window, state->gl_context);
SDL_GL_SetSwapInterval(1);  // VSync
```

#### 3.3.5 ImGui 渲染后端

- **删除**：`ImGui_ImplSDLGPU3_Init(...)` 及所有 `ImGui_ImplSDLGPU3_*` 调用。
- **添加**：  
  `ImGui_ImplOpenGL3_Init(glsl_version);`  
  （`glsl_version` 使用上面根据平台选的字符串。）

将原来的 `ImGui_ImplSDL3_InitForSDLGPU(state->engine_window)` 改为：  
`ImGui_ImplSDL3_InitForOpenGL(state->engine_window, state->gl_context);`  
这样 SDL3 平台后端会正确支持多视口下的 OpenGL 上下文切换。

#### 3.3.6 SDL_AppIterate（主循环）

- **删除**：  
  - `SDL_AcquireGPUCommandBuffer`  
  - `SDL_WaitAndAcquireGPUSwapchainTexture`  
  - `ImGui_ImplSDLGPU3_PrepareDrawData`  
  - `SDL_BeginGPURenderPass` / `ImGui_ImplSDLGPU3_RenderDrawData` / `SDL_EndGPURenderPass`  
  - `SDL_SubmitGPUCommandBuffer`

- **NewFrame 部分**改为：

```cpp
ImGui_ImplOpenGL3_NewFrame();
ImGui_ImplSDL3_NewFrame();
ImGui::NewFrame();
```

- **渲染部分**改为（顺序不要颠倒）：

```cpp
DE::Engine::LogicIterate(appstate);

ImGui::Render();
ImDrawData* draw_data = ImGui::GetDrawData();
const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

if (!is_minimized) {
    // 1) 将主窗口的 framebuffer 设为当前，并清屏（与 example_sdl3_opengl3 一致，用 ImGui 的 DisplaySize）
    glViewport(0, 0, (int)draw_data->DisplaySize.x, (int)draw_data->DisplaySize.y);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 2) 引擎渲染（内部会绑定场景 FBO 画三角形，再解绑）
    DE::Engine::RenderIterate(appstate);

    // 3) 把 ImGui 画到当前默认 framebuffer（包含场景视口窗口里的纹理）
    ImGui_ImplOpenGL3_RenderDrawData(draw_data);
}

if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    SDL_Window* backup_window = SDL_GL_GetCurrentWindow();
    SDL_GLContext backup_context = SDL_GL_GetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    SDL_GL_MakeCurrent(backup_window, backup_context);
}

SDL_GL_SwapWindow(state->engine_window);
```

**学习要点**：  
- OpenGL 没有「command buffer」和「render pass」；每帧就是按顺序执行 gl 调用。  
- 先清屏、再执行引擎的 RenderIterate（画到 FBO）、最后把 ImGui 画到默认 0 号 framebuffer，这样场景视口窗口里显示的才是本帧刚画好的纹理。

#### 3.3.7 SDL_AppQuit

- **删除**：  
  `SDL_WaitForGPUIdle`、`ImGui_ImplSDLGPU3_Shutdown`、`SDL_ReleaseWindowFromGPUDevice`、`SDL_DestroyGPUDevice`。
- **添加**：  
  `ImGui_ImplOpenGL3_Shutdown();`  
  `SDL_GL_DestroyContext(state->gl_context);`  
  其余顺序建议：ImGui 先 Shutdown，再 `DE::Engine::Quit`（里面对 GL 资源做 delete），最后 Destroy 上下文和窗口。

---

### 3.4 Engine：Init / Quit / RenderIterate 改为 OpenGL

**文件：`src/engine/Engine.h`**

- `RenderIterate` 签名改为只保留 `appstate`：  
  `static bool RenderIterate(void* appstate);`  
  不再接受 `SDL_GPUCommandBuffer*`。

**文件：`src/engine/Engine.cpp`**

- **头文件**：删除 `SDL3/SDL_gpu.h`、`SDL3/SDL_storage.h` 以及 `util/AboutGPU.h`（若不再用其 SDL_GPU 接口）；添加 `#include <SDL3/SDL_opengl.h>`。若你把「加载 GLSL 并创建 program」放在别的文件（如 `OpenGLUtil.h/cpp`），则包含该头文件。

- **Init 中「场景视口 + 三角形」部分**整体替换为 OpenGL 逻辑，建议顺序如下（伪代码，具体 GL 常量请按你使用的 GL 版本书写）：

  1. **创建 FBO 和纹理**  
     - `glGenFramebuffers(1, &state->scene_viewport_fbo)`  
     - `glGenTextures(1, &state->scene_viewport_texture)`  
     - `glBindTexture(GL_TEXTURE_2D, state->scene_viewport_texture)`  
     - `glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr)`  
     - 设置 `GL_TEXTURE_MIN_FILTER` / `GL_TEXTURE_MAG_FILTER`（如 `GL_LINEAR`）  
     - `glBindFramebuffer(GL_FRAMEBUFFER, state->scene_viewport_fbo)`  
     - `glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state->scene_viewport_texture, 0)`  
     - 检查 `glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE`  
     - `state->scene_viewport_window->SetViewportTexture((void*)(intptr_t)state->scene_viewport_texture, width, height);`

  2. **加载 GLSL 并创建 program**  
     使用你在「3.6 节」实现的工具函数，从 `GetEngineAssetsPath() + "shader/..."` 读取 **OpenGL 版** 的 .vert/.frag 源码（见 3.5 节），得到 `state->scene_program`。

  3. **可选：创建 VAO/VBO 画三角形**  
     若你用「顶点数组 + 顶点属性」的方式，可在这里生成 `scene_vao`、`scene_vbo`，并上传顶点数据（例如一个三角形的 3 个顶点）；若你用「无 VBO、在顶点着色器里用 gl_VertexID 生成顶点」的方式，则可不创建 VBO，只保留一个空 VAO（部分 GL 要求至少绑定一个 VAO）。

- **RenderIterate(appstate)** 中原来所有 SDL_GPU 调用替换为：

  1. `glBindFramebuffer(GL_FRAMEBUFFER, state->scene_viewport_fbo)`  
  2. `glViewport(0, 0, state->scene_viewport_texture_width, state->scene_viewport_texture_height)`  
  3. `glClearColor(0.1f, 0.1f, 0.1f, 1.0f); glClear(GL_COLOR_BUFFER_BIT)`  
  4. `glUseProgram(state->scene_program)`  
  5. 若使用 VAO：`glBindVertexArray(state->scene_vao); glDrawArrays(GL_TRIANGLES, 0, 3);`  
  6. `glBindFramebuffer(GL_FRAMEBUFFER, 0)`  
  7. 返回 `true`。

- **Quit**：删除所有 SDL_GPU 的 Release/Destroy；改为删除 GL 资源：  
  `glDeleteProgram(state->scene_program)`、`glDeleteFramebuffers(1, &state->scene_viewport_fbo)`、`glDeleteTextures(1, &state->scene_viewport_texture)`，以及 VAO/VBO 的 delete（若创建了）。

**学习要点**：  
- FBO 绑定一张纹理后，后续的 `glClear`、`glDraw*` 都会作用到该纹理上；解绑 FBO 后，主窗口的默认 framebuffer 0 才会被 ImGui 使用。  
- OpenGL 的「管线」就是「当前绑定的 program + 当前 VAO/VBO + 当前 FBO」等状态组合。

---

### 3.5 着色器：为 OpenGL 准备 GLSL

当前工程里的 `default_vert.vert` / `default_frag.frag` 是 **#version 450** 且使用 **gl_VertexIndex**（Vulkan 风格），OpenGL 桌面版应使用 **#version 330**（或 macOS 上 150）和 **gl_VertexID**。

**方案 A：新建 OpenGL 专用 shader 文件（推荐学习）**

例如在 `src/engine/assets/shader/` 下增加：

- `opengl_default_vert.vert`
- `opengl_default_frag.frag`

**顶点着色器示例（OpenGL 3.3，与 #version 330 对应）：**

```glsl
#version 330 core

out vec3 fragColor;

const vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

const vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
    fragColor = colors[gl_VertexID];
}
```

**片段着色器示例：**

```glsl
#version 330 core

in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

在 Engine::Init 里加载 **opengl_default_vert.vert** 和 **opengl_default_frag.frag** 的源码并创建 program。  
若在 macOS 上使用 **#version 150**，需把 `out`/`in` 改成 `varying`/`attribute` 等 150 的写法，并对应修改入口；建议先完成 330 版本再尝试 150。

**方案 B：沿用现有文件名，在加载时按后端选择**

保留现有 default_vert/default_frag 的 **文件名**，但为 OpenGL 分支准备一套内容为上面 330 的版本（可复制到 opengl 专用文件再由代码读该文件），避免和 SPIR-V 编译管线混淆。

**学习要点**：  
- Vulkan/SPIR-V 用 `gl_VertexIndex`，OpenGL 用 `gl_VertexID`。  
- OpenGL 3.3 的 GLSL 用 `in`/`out`，且顶点着色器里没有 `location` 时需保证与片段着色器 `in` 的变量名一致（或通过 layout(location=...) 匹配）。

---

### 3.6 从文件加载 GLSL 并创建 OpenGL Program

可以单独建 `src/engine/util/OpenGLUtil.h` 和 `OpenGLUtil.cpp`，提供例如：

```cpp
// 从文件路径读取 GLSL 源码，编译并链接着色器，返回 program id；失败返回 0
unsigned int CreateProgramFromFiles(const std::string& vert_path, const std::string& frag_path);
```

实现步骤（建议自己写一遍以巩固）：

1. 读 vert_path / frag_path 的文本到 `std::string`。
2. `glCreateShader(GL_VERTEX_SHADER)`，`glShaderSource`，`glCompileShader`；用 `glGetShaderiv(..., GL_COMPILE_STATUS)` 和 `glGetShaderInfoLog` 检查编译是否成功。
3. 对 fragment shader 做同样步骤。
4. `glCreateProgram()`，`glAttachShader` 两个 shader，`glLinkProgram`，检查 `GL_LINK_STATUS` 和 `glGetProgramInfoLog`。
5. `glDeleteShader` 两个 shader（链接到 program 后即可删）。
6. 返回 program id。

Engine::Init 里用 `GetEngineAssetsPath() + "shader/opengl_default_vert.vert"` 等路径调用该函数，将返回值赋给 `state->scene_program`。

**学习要点**：  
- OpenGL 的 shader 是「源码 → 编译 → 链接」；与 SDL_GPU 的「预编译 SPIR-V/MSL 二进制」不同。  
- 编译/链接失败时一定要看 InfoLog，否则很难排错。

---

### 3.7 场景视口窗口：把 GL 纹理 id 传给 ImGui

**文件：`src/engine/window/SceneViewportWindow.cpp`**

`SetViewportTexture` 已接受 `void* texture`，在 OpenGL 后端下，该指针应表示「GL 纹理 id」：  
在 Engine::Init 里调用时传 `(void*)(intptr_t)state->scene_viewport_texture`。

在 `Draw()` 里显示时，ImGui OpenGL3 后端约定 `ImTextureID` 即为该 GL 纹理 id 的指针形式，因此：

```cpp
ImGui::Image((ImTextureID)(intptr_t)viewport_texture_, ImVec2(viewport_width_, viewport_height_));
```

只要 `viewport_texture_` 存的是 Engine 传进来的 `(void*)(intptr_t)gl_texture_id`，即可正确显示。  
**学习要点**：不同渲染后端对 `ImTextureID` 的约定不同；OpenGL 下就是「GL 纹理 id 转成指针」。

---

### 3.8 Application：去掉 command_buffer 参数

若 `DA::Application::RenderIterate` 的签名是 `(void* appstate, SDL_GPUCommandBuffer* command_buffer)`，改为只保留 `(void* appstate)`；若运行期暂时不画 3D，实现可以为空或直接返回 true。  
`Engine::RenderIterate` 的调用处在 main 里已改为无参（仅 appstate），无需再传 command_buffer。

---

## 四、学习自检清单

完成修改后，可依此自检：

- [ ] CMake 能正确找到 OpenGL 并链接，且只编译 imgui_impl_opengl3（不再编译 imgui_impl_sdlgpu3）。
- [ ] 窗口创建带 `SDL_WINDOW_OPENGL`，且在 CreateWindow 前设置了 GL 属性。
- [ ] 有且仅有 `SDL_GL_CreateContext` / `SDL_GL_MakeCurrent`，没有 `SDL_CreateGPUDevice` / `SDL_ClaimWindowForGPUDevice`。
- [ ] ImGui 使用 `ImGui_ImplOpenGL3_Init/NewFrame/RenderDrawData/Shutdown`，主循环里用 `glClear` + `RenderIterate` + `ImGui_ImplOpenGL3_RenderDrawData` + `SDL_GL_SwapWindow`。
- [ ] 场景视口使用 FBO + 纹理，Engine::RenderIterate 里绑定该 FBO 并画三角形，再解绑。
- [ ] 场景用的 GLSL 是 OpenGL 可用的版本（如 330），且使用 `gl_VertexID` 而非 `gl_VertexIndex`。
- [ ] 场景视口窗口中 `ImGui::Image` 使用的是 `(ImTextureID)(intptr_t)gl_texture_id`。
- [ ] Quit 时正确释放 GL 资源并 `SDL_GL_DestroyContext`。

---

## 五、参考

- ImGui 官方示例：`submodule/imgui/examples/example_sdl3_opengl3/main.cpp`（SDL3 + OpenGL3 的完整流程）。
- SDL3 文档：`SDL_GL_*` 系列接口。
- 学习 OpenGL（中文）：[https://learnopengl-cn.github.io/](https://learnopengl-cn.github.io/)（FBO、纹理、着色器基础）。

按上述步骤完成后，DearEngine 的渲染将完全由 OpenGL 驱动，便于你在此基础上继续学习 GL 管线、着色器与离屏渲染。

---

## 附录 A：CreateProgramFromFiles 实现参考

以下为「从文件读 GLSL 并创建 program」的极简实现思路，便于你手写或对照学习。

```cpp
#include <fstream>
#include <sstream>
#include <string>
#include <SDL3/SDL_opengl.h>

static std::string read_file(const std::string& path) {
    std::ifstream f(path);
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

static unsigned int compile_shader(unsigned int type, const std::string& source) {
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    int success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(id, sizeof(log), nullptr, log);
        // 打印 log 并返回 0 或 abort
        glDeleteShader(id);
        return 0;
    }
    return id;
}

unsigned int CreateProgramFromFiles(const std::string& vert_path, const std::string& frag_path) {
    std::string vert_src = read_file(vert_path);
    std::string frag_src = read_file(frag_path);
    unsigned int vs = compile_shader(GL_VERTEX_SHADER, vert_src);
    unsigned int fs = compile_shader(GL_FRAGMENT_SHADER, frag_src);
    if (!vs || !fs) return 0;
    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        // 打印 log，glDeleteProgram(program); return 0;
        glDeleteProgram(program);
        return 0;
    }
    return program;
}
```

将上述函数放在 `OpenGLUtil.cpp` 中，并在头文件中声明 `CreateProgramFromFiles` 即可在 Engine::Init 里调用。

---

## 附录 B：无 VBO 画三角形（gl_VertexID）

若使用 3.5 节中的顶点着色器（在 shader 里用 `positions[gl_VertexID]` 和 `colors[gl_VertexID]`），则不需要上传顶点数据。OpenGL 3.3 Core 要求至少绑定一个 VAO，可以只创建一个空 VAO，每帧绘制前绑定即可：

```cpp
// Init 中
glGenVertexArrays(1, &state->scene_vao);

// RenderIterate 中
glBindVertexArray(state->scene_vao);
glDrawArrays(GL_TRIANGLES, 0, 3);
glBindVertexArray(0);
```

这样即可画出与当前 default_vert/default_frag 效果一致的彩色三角形（红绿蓝三顶点）。
