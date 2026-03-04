# 渲染流程教学指南：使用 Main Camera 绘制场景到 Application 窗口

## 概述

本文档将教你如何在 Application 运行时，创建独立的窗口并使用场景的 `main_camera` 将场景内容绘制到该窗口。

---

## 第一部分：理解你的架构

### 1.1 当前架构分析

你的项目有两个窗口系统：

| 系统 | 窗口 | OpenGL上下文 | 用途 |
|------|------|--------------|------|
| EngineEditor | `state->engine_window` | `state->gl_context` | 编辑器窗口（ImGui界面） |
| Application | `application->window_` | ❌ 缺失！ | 运行时游戏窗口 |

**核心问题**：`Application` 创建了 SDL 窗口，但没有创建 OpenGL 上下文，因此无法进行 OpenGL 渲染。

### 1.2 渲染流程概览

```
┌─────────────────────────────────────────────────────────────────┐
│                    完整渲染流程                                  │
└─────────────────────────────────────────────────────────────────┘
                               │
    ┌──────────────────────────┴──────────────────────────┐
    │                                                     │
    ▼                                                     ▼
┌─────────────────┐                              ┌─────────────────┐
│   编辑器窗口     │                              │  游戏窗口        │
│ engine_window   │                              │  window_        │
│  + gl_context   │                              │  + gl_context   │
└─────────────────┘                              └─────────────────┘
    │                                                     │
    ▼                                                     ▼
┌─────────────────┐                              ┌─────────────────┐
│ ImGui 渲染      │                              │ 场景渲染        │
│ (编辑器界面)    │                              │ (3D场景)        │
└─────────────────┘                              └─────────────────┘
```

---

## 第二部分：需要完成的核心步骤

### 步骤 1：为 Application 创建 OpenGL 上下文

**在 `Application.h` 中添加：**

```cpp
class Application {
    // ...
private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;  // ← 添加这个！
};
```

**在 `Application::Start` 中创建上下文：**

```cpp
bool Application::Start(void *appstate, DE::Scene *scene) {
    // ... 创建窗口的代码 ...

    // 创建 OpenGL 上下文（必须在创建窗口后）
    gl_context_ = SDL_GL_CreateContext(window_);
    if (gl_context_ == nullptr) {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return false;
    }

    // 设置当前上下文（让后续 OpenGL 调用作用于这个窗口）
    SDL_GL_MakeCurrent(window_, gl_context_);

    // 重要：如果使用 glad，需要再次加载 OpenGL 函数
    // （因为新上下文可能需要重新加载）
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

    // ... 其余代码 ...
}
```

### 步骤 2：在 RenderIterate 中设置渲染状态

**完整的 `Application::RenderIterate`：**

```cpp
bool Application::RenderIterate(void *appstate) {
    // 1. 切换到 Application 的 OpenGL 上下文
    SDL_GL_MakeCurrent(window_, gl_context_);

    // 2. 获取窗口尺寸
    int w, h;
    SDL_GetWindowSize(window_, &w, &h);

    // 3. 设置视口和清屏
    glViewport(0, 0, w, h);
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);  // 深蓝色背景
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 4. 启用深度测试（3D渲染必须）
    glEnable(GL_DEPTH_TEST);

    // 5. 创建 RenderContext
    if (current_playing_scene && current_playing_scene->main_camera) {
        DE::RenderContext render_context;
        render_context.camera = current_playing_scene->main_camera;
        render_context.screenWidth = &w;
        render_context.screenHeight = &h;
        // render_context.program = ...; // 需要设置着色器程序！

        // 6. 遍历渲染
        for (auto& entity : current_playing_scene->root) {
            RenderIterateEntity(entity.get(), appstate, &render_context);
        }
    }

    // 7. 交换缓冲区（显示渲染结果）
    SDL_GL_SwapWindow(window_);

    return true;
}
```

### 步骤 3：创建并使用着色器程序

你的 `RenderContext` 有一个 `program` 字段，组件用它来设置矩阵。

**你需要一个简单的着色器。创建着色器的典型流程：**

```cpp
// 顶点着色器源码
const char* vertexShaderSource = R"(
#version 410 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

// 片段着色器源码
const char* fragmentShaderSource = R"(
#version 410 core
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.5, 0.2, 1.0);  // 橙色
}
)";

// 编译链接着色器的函数
unsigned int CreateShaderProgram() {
    // 编译顶点着色器
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // 编译片段着色器
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // 链接程序
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // 清理着色器对象
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}
```

### 步骤 4：在 Application 中管理着色器

**在 `Application.h` 中：**

```cpp
class Application {
    // ...
private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;
    unsigned int shader_program_ = 0;  // ← 添加着色器程序
};
```

**在 `Application::Start` 中创建着色器：**

```cpp
bool Application::Start(void *appstate, DE::Scene *scene) {
    // ... 创建窗口和上下文 ...

    // 创建着色器程序
    shader_program_ = CreateShaderProgram();

    // ... 其余代码 ...
}
```

**在 `RenderIterate` 中使用着色器：**

```cpp
bool Application::RenderIterate(void *appstate) {
    // ... 前面的代码 ...

    // 使用着色器程序
    glUseProgram(shader_program_);
    render_context.program = &shader_program_;

    // 遍历渲染
    for (auto& entity : current_playing_scene->root) {
        RenderIterateEntity(entity.get(), appstate, &render_context);
    }

    // ... 后面的代码 ...
}
```

### 步骤 5：在 End 中清理资源

```cpp
bool Application::End(void *appstate) {
    // 清理场景
    if (current_playing_scene) {
        for (auto& entity : current_playing_scene->root) {
            EndEntity(entity.get(), appstate);
        }
    }

    // 清理 OpenGL 资源
    if (shader_program_) {
        glDeleteProgram(shader_program_);
        shader_program_ = 0;
    }

    // 清理 OpenGL 上下文
    if (gl_context_) {
        SDL_GL_DestroyContext(gl_context_);
        gl_context_ = nullptr;
    }

    // 清理窗口
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    return true;
}
```

---

## 第三部分：完整流程图

```
┌────────────────────────────────────────────────────────────────────┐
│                    Application 生命周期                             │
└────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│ Start()                                                             │
│   ├── SDL_CreateWindow()        // 创建窗口                         │
│   ├── SDL_GL_CreateContext()    // 创建 OpenGL 上下文 ★关键!        │
│   ├── gladLoadGLLoader()        // 加载 OpenGL 函数                  │
│   ├── CreateShaderProgram()     // 创建着色器程序                    │
│   └── 遍历组件调用 Start()                                           │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│ RenderIterate() [每帧调用]                                           │
│   ├── SDL_GL_MakeCurrent()      // 切换到游戏窗口上下文 ★关键!       │
│   ├── glViewport()              // 设置视口                          │
│   ├── glClear()                 // 清屏                              │
│   ├── glEnable(GL_DEPTH_TEST)   // 启用深度测试                       │
│   ├── glUseProgram()            // 使用着色器                         │
│   ├── 创建 RenderContext        // 设置相机、屏幕尺寸、着色器          │
│   ├── 遍历组件调用 RenderIterate()  // 每个组件自己绘制                 │
│   └── SDL_GL_SwapWindow()       // 交换缓冲区，显示画面 ★关键!        │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│ End()                                                               │
│   ├── 遍历组件调用 End()                                             │
│   ├── glDeleteProgram()         // 删除着色器                         │
│   ├── SDL_GL_DestroyContext()   // 销毁 OpenGL 上下文                 │
│   └── SDL_DestroyWindow()       // 销毁窗口                          │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 第四部分：组件渲染逻辑

你的 `TestCubeComponent::RenderIterate` 已经实现了渲染逻辑，它：

1. 检查 `render_context` 是否有效
2. 从 `render_context` 获取相机，计算投影矩阵和视图矩阵
3. 从 `TransformComponent` 获取位置和旋转，计算模型矩阵
4. 将三个矩阵传递给着色器
5. 绑定 VAO 并绘制三角形

```cpp
// TestCubeComponent::RenderIterate 核心逻辑
if (render_context) {
    // 计算投影矩阵（透视投影）
    glm::mat4 projection = glm::perspective(
        glm::radians(render_context->camera->Fov),
        (float)*render_context->screenWidth / (float)*render_context->screenHeight,
        0.1f, 100.0f
    );

    // 获取视图矩阵（从相机视角）
    glm::mat4 view = render_context->camera->GetViewMatrix();

    // 计算模型矩阵（物体的位置、旋转、缩放）
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, transform_component->position);
    // ... 旋转 ...

    // 传递给着色器
    glUniformMatrix4fv(..., "projection", ..., glm::value_ptr(projection));
    glUniformMatrix4fv(..., "view", ..., glm::value_ptr(view));
    glUniformMatrix4fv(..., "model", ..., glm::value_ptr(model));

    // 绘制
    glDrawArrays(GL_TRIANGLES, 0, 36);
}
```

---

## 第五部分：关键概念详解

### 5.1 为什么需要 OpenGL 上下文？

OpenGL 是一个状态机，所有 OpenGL 命令都需要一个"当前上下文"才能工作。

```
┌─────────────────────────────────────────────────────────────┐
│                    OpenGL 上下文                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ 包含：                                               │   │
│  │   - 所有 OpenGL 状态（深度测试、混合模式等）          │   │
│  │   - 纹理、缓冲区、着色器程序等资源                    │   │
│  │   - 绑定到特定窗口                                   │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

没有上下文，`glClear`, `glDrawArrays` 等函数什么都不会做！

### 5.2 SDL_GL_MakeCurrent 的作用

你的程序有两个窗口（编辑器 + 游戏），每个窗口有自己的上下文。

```cpp
// 在编辑器窗口渲染时
SDL_GL_MakeCurrent(state->engine_window, state->gl_context);
// ... 编辑器的 OpenGL 调用 ...

// 在游戏窗口渲染时
SDL_GL_MakeCurrent(application->window_, application->gl_context_);
// ... 游戏的 OpenGL 调用 ...
```

**必须在每次渲染前切换到正确的上下文！**

### 5.3 双缓冲与交换

```
帧缓冲区工作原理：

┌──────────────┐     ┌──────────────┐
│   前缓冲区    │ ──→ │   显示屏幕    │  用户看到的画面
└──────────────┘     └──────────────┘
       ↑
       │ SDL_GL_SwapWindow() 交换
       ↓
┌──────────────┐
│   后缓冲区    │ ←── OpenGL 渲染的目标
└──────────────┘
```

**不调用 `SDL_GL_SwapWindow()`，画面不会更新！**

### 5.4 深度测试

3D 场景中，物体有前后关系。深度测试确保前面的物体遮挡后面的物体。

```cpp
glEnable(GL_DEPTH_TEST);     // 启用深度测试
glClear(GL_DEPTH_BUFFER_BIT); // 每帧清除深度缓冲
```

---

## 第六部分：常见问题

### Q1: 窗口是黑屏/白屏，什么都没有？

检查清单：
1. ✅ 是否创建了 OpenGL 上下文？
2. ✅ 是否调用了 `SDL_GL_MakeCurrent()`？
3. ✅ 是否调用了 `glClear()`？
4. ✅ 是否调用了 `SDL_GL_SwapWindow()`？
5. ✅ 着色器是否编译成功？
6. ✅ 相机位置是否能看到物体？

### Q2: 如何调试着色器？

```cpp
// 检查编译错误
int success;
char infoLog[512];
glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
if (!success) {
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    printf("着色器编译错误: %s\n", infoLog);
}
```

### Q3: 相机应该放在哪里才能看到物体？

你的测试立方体在原点 (0, 0, 0)，相机需要在合适的位置：

```cpp
// 相机在 (-5, 0, 0)，看向 +X 方向
camera_entity->GetComponent<TransformComponent>()->position = glm::vec3(-5.0f, 0.0f, 0.0f);
// 确保相机的 rotation 让它看向原点
```

---

## 第七部分：修改清单

### Application.h 需要添加：

```cpp
private:
    SDL_GLContext gl_context_ = nullptr;      // OpenGL 上下文
    unsigned int shader_program_ = 0;          // 着色器程序
```

### Application.cpp 需要修改：

| 方法 | 需要做的事 |
|------|-----------|
| `Start()` | 创建 OpenGL 上下文、加载 glad、创建着色器 |
| `RenderIterate()` | 切换上下文、设置视口、清屏、创建 RenderContext、交换缓冲区 |
| `End()` | 删除着色器、销毁 OpenGL 上下文 |

---

## 第八部分：下一步学习

1. **封装着色器类**：创建 `Shader` 类管理编译和链接
2. **添加更多渲染组件**：网格渲染器、粒子系统等
3. **实现光照**：在着色器中添加光照计算
4. **相机控制**：让玩家可以用键盘/鼠标控制相机