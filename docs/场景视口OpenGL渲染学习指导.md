# 在「场景视口」里画一个三角形 — OpenGL 渲染学习指导

---

## 新手先看这里（3 句话）

1. **目标**：让「场景视口」窗口里出现一个彩色三角形，而不是空白。
2. **做法**：用 OpenGL 的 FBO（帧缓冲）把三角形画到一张纹理上，再让 ImGui 把这张纹理显示在场景视口里。
3. **顺序**：每一帧先绑定 FBO、画三角形到纹理，再画整个界面（ImGui 在场景视口里显示这张纹理）。

下面用大白话和最小步骤讲清楚怎么做。

---

## 第一部分：用大白话理解「两张图」

可以这么想：

- **你看到的整个窗口** = 一张最终贴在屏幕上的大图。  
  主程序把 ImGui 画出来的所有界面（菜单、控制台、场景视口窗口框……）画到这张大图上。  
  这张大图对应 OpenGL 的「默认帧缓冲」（framebuffer 0），由 SDL/窗口系统提供。

- **场景视口里那一块** = 我们想让它显示「另一张图」的内容。  
  这张图你要自己造：用 **FBO + 纹理**，先让 GPU 在这张纹理上画三角形，再告诉 ImGui：「把这张纹理贴在场景视口里显示出来」。

所以：

- **第一张图**：你自己创建的 **FBO + 颜色附件纹理**。GPU 先把三角形画在这张纹理上。
- **第二张图**：整个窗口用的默认帧缓冲。程序再把 ImGui（包括「在场景视口里显示第一张纹理」）画到这张图上。

在 OpenGL 里，「一张图」对应 **纹理（texture）**，画到纹理上要用 **帧缓冲对象（FBO）** 把纹理绑成渲染目标。  
**你要做的就是：创建 FBO + 纹理 → 每帧绑定 FBO、把三角形画到纹理上 → 在场景视口里用 ImGui::Image 把纹理显示出来。**

---

## 第二部分：你需要会用的三件事（最少概念）

### 1. 纹理 + FBO = 一张可以画东西上去的「图」

- 默认帧缓冲：SDL/系统给的，对应整个窗口。
- 场景视口用的这张图：你要用 OpenGL 的 **glGenTextures**、**glGenFramebuffers** 自己创建一张纹理，并把它绑到 FBO 的颜色附件上（**glFramebufferTexture2D**）。  
  创建好后，你就有一个「可以当渲染目标的纹理」了，后面每帧都会先画到它上面。

### 2. 「画到某张纹理上」= 绑定 FBO 后的一次绘制

- 主程序里「画 ImGui 到窗口」：本质是画到默认帧缓冲（framebuffer 0）。
- 我们要加的是：**每帧先** `glBindFramebuffer(GL_FRAMEBUFFER, state->scene_viewport_fbo)`，设置 viewport、清屏，再用 **glUseProgram** + **glBindVertexArray** + **glDrawArrays** 把三角形画到 FBO 绑定的纹理上；**然后** `glBindFramebuffer(GL_FRAMEBUFFER, 0)` 恢复默认，再画 ImGui。  
  画完之后，这张纹理里就有三角形；ImGui 在场景视口里把这张纹理贴出来。

### 3. 在 ImGui 里显示这张纹理

- ImGui 的 **ImGui::Image(ImTextureID, size)**：给一个纹理 ID（OpenGL 下就是 `(void*)(intptr_t)gl_texture_id`）和显示尺寸，就会在窗口里把这张图贴出来。
- 在「场景视口」窗口的 Draw 里，把上面那张「画了三角形的纹理」的 GL 纹理 ID 传进去，场景视口里就会看到三角形。

总结：**创建 FBO + 纹理 → 每帧先绑定 FBO、画三角形到纹理，再绑定回 0、画 ImGui（场景视口里用 ImGui::Image 显示该纹理）。**

---

## 第三部分：一帧里应该先做什么、后做什么

每一帧的逻辑顺序要这样（不用背，记住「先画场景纹理，再画整屏界面」就行）：

1. **先**：`glBindFramebuffer` 到场景视口 FBO → `glViewport`、`glClear` → `glUseProgram`、`glBindVertexArray`、`glDrawArrays` 画三角形 → `glBindFramebuffer(0)`。
2. **再**：像现在一样，把 ImGui 画到默认帧缓冲（这时 ImGui 会在场景视口里用 `ImGui::Image` 显示你那张纹理）。

这样场景视口里才能看到三角形。顺序反了就会出问题。

---

## 第四部分：自己动手的 3 个小目标（按顺序做）

下面每一步都是「先做到这个，再往下」。每个小目标下面都写了**应该在哪里添加什么代码**，按文件、按位置对照着改即可。

---

### 小目标 1：让场景视口里出现「一张图」（先不画三角形也行）

- **你要做的**：  
  - 在程序启动时（如 `Engine::Init`），用 OpenGL 的 **glGenFramebuffers**、**glGenTextures** 创建 FBO 和一张纹理（如 1280×720），把纹理绑到 FBO 的颜色附件（**glFramebufferTexture2D**），并检查 **glCheckFramebufferStatus** 为 **GL_FRAMEBUFFER_COMPLETE**。  
  - 把纹理 ID 和宽高传给「场景视口」窗口（**SetViewportTexture((void*)(intptr_t)texture_id, width, height)**）。  
  - 在场景视口窗口的 Draw 里，用 **ImGui::Image(viewport_texture_, ImVec2(...))** 显示这张纹理（先不用管图里有没有内容，哪怕全灰也行）。
- **做完之后**：运行程序，打开场景视口，你应该能看到一块区域（可能是灰的），说明 ImGui 已经能显示你创建的这张纹理了。

**注意**：  
- 使用 **GLAD** 加载 OpenGL 函数时，要在 **SDL_GL_MakeCurrent** 之后、任何 GL 调用之前执行 **gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)**（一般在 main 的 Init 里）。  
- **State** 里需要保存：`unsigned int scene_viewport_fbo`、`unsigned int scene_viewport_texture`、`int scene_viewport_texture_width/height`。

**① `State.h`（或 AppState）里增加成员：**

```cpp
    unsigned int scene_viewport_fbo = 0;        // GL FBO
    unsigned int scene_viewport_texture = 0;    // GL 纹理 id，供 ImGui::Image 使用
    int scene_viewport_texture_width = 1280;
    int scene_viewport_texture_height = 720;
```

**② 在 Init 里创建 FBO + 纹理（示例片段，放在创建窗口、GL 上下文、GLAD 初始化之后）：**

```cpp
    GLuint fbo = 0, tex = 0;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, state->scene_viewport_texture_width, state->scene_viewport_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteTextures(1, &tex);
        glDeleteFramebuffers(1, &fbo);
        return false;  // 或打印错误
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    state->scene_viewport_fbo = fbo;
    state->scene_viewport_texture = tex;
    state->scene_viewport_window->SetViewportTexture((void*)(intptr_t)state->scene_viewport_texture, state->scene_viewport_texture_width, state->scene_viewport_texture_height);
```

**③ `SceneViewportWindow`**：已有 **SetViewportTexture(void* texture, int width, int height)** 和 **Draw()** 里 **ImGui::Image(viewport_texture_, ImVec2(viewport_width_, viewport_height_))** 即可（OpenGL 下传的是 `(void*)(intptr_t)gl_texture_id`）。

---

### 小目标 2：准备好「画三角形」需要的东西（着色器 + VAO）

- **你要做的**：  
  - 准备顶点/片段着色器（如 **default_vert.vert**、**default_frag.frag**）。OpenGL 桌面用 **gl_VertexID**（不要用 Vulkan 的 gl_VertexIndex）；若源文件里写的是 gl_VertexIndex，加载源码后替换成 gl_VertexID 再 **glShaderSource**。  
  - 用 **glCreateShader**、**glCompileShader**、**glCreateProgram**、**glLinkProgram** 得到 **scene_program**。  
  - 在 **OpenGL 3.2+ Core Profile**（如 macOS）下，**必须**有一个 VAO 绑定才能 **glDrawArrays**；用 **glGenVertexArrays(1, &state->scene_vao)** 创建，每帧绘制前 **glBindVertexArray(state->scene_vao)**。
- **做完之后**：程序能正常启动、不报错；画面上可以暂时还看不到三角形（因为小目标 3 才真正每帧画）。

**④ State 里再增加：**

```cpp
    unsigned int scene_program = 0;   // 场景用着色器程序（画三角形）
    unsigned int scene_vao = 0;      // Core 下 glDrawArrays 必须绑定 VAO
```

**⑤ 加载着色器并创建 program**（可从文件或 SDL_Storage 读源码；若源码里是 gl_VertexIndex，替换为 gl_VertexID 再编译）。示例逻辑：

- 读入顶点/片段源码字符串。
- `glCreateShader(GL_VERTEX_SHADER)` / `GL_FRAGMENT_SHADER` → `glShaderSource` → `glCompileShader`（检查 GL_COMPILE_STATUS）。
- `glCreateProgram` → `glAttachShader` 两个 → `glLinkProgram`（检查 GL_LINK_STATUS）→ `glDeleteShader` 两个。
- 保存到 `state->scene_program`。
- `glGenVertexArrays(1, &state->scene_vao)`。

着色器示例（顶点用 gl_VertexID，无 VBO）：

```glsl
// 顶点着色器 (#version 330 或 410)
out vec3 vColor;
const vec2 positions[3] = vec2[](vec2(0.0,-0.5), vec2(0.5,0.5), vec2(-0.5,0.5));
const vec3 colors[3] = vec3[](vec3(1,0,0), vec3(0,1,0), vec3(0,0,1));
void main() {
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
    vColor = colors[gl_VertexID];
}
// 片段着色器
in vec3 vColor;
out vec4 outColor;
void main() { outColor = vec4(vColor, 1.0); }
```

---

### 小目标 3：每帧先画三角形到 FBO 纹理，再画 ImGui

- **你要做的**：  
  - 在主循环里，找到「清屏、画 ImGui」的那段。  
  - 在**画 ImGui 之前**，加一段：**绑定场景视口 FBO** → **glViewport** 为视口纹理尺寸 → **glClear** → **glUseProgram(scene_program)** → **glBindVertexArray(scene_vao)** → **glDrawArrays(GL_TRIANGLES, 0, 3)** → **glBindFramebuffer(GL_FRAMEBUFFER, 0)**。  
  - 然后再清默认帧缓冲、画 ImGui（含场景视口里 ImGui::Image 显示该纹理）。
- **做完之后**：场景视口里能看到彩色三角形。

**⑥ 在每帧渲染里（如 Engine::RenderIterate），在画 ImGui 之前插入：**

```cpp
    if (state->scene_viewport_fbo && state->scene_program && state->scene_viewport_texture_width > 0 && state->scene_viewport_texture_height > 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, state->scene_viewport_fbo);
        glViewport(0, 0, state->scene_viewport_texture_width, state->scene_viewport_texture_height);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(state->scene_program);
        glBindVertexArray(state->scene_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
```

**⑦ 退出时释放（如 Engine::Quit 或 SDL_AppQuit）：**

```cpp
    if (state->scene_viewport_texture) {
        glDeleteTextures(1, &state->scene_viewport_texture);
        state->scene_viewport_texture = 0;
    }
    if (state->scene_viewport_fbo) {
        glDeleteFramebuffers(1, &state->scene_viewport_fbo);
        state->scene_viewport_fbo = 0;
    }
    if (state->scene_program) {
        glDeleteProgram(state->scene_program);
        state->scene_program = 0;
    }
    if (state->scene_vao) {
        glDeleteVertexArrays(1, &state->scene_vao);
        state->scene_vao = 0;
    }
```

---

## 第五部分：小结

- **小目标 1**：State 加 FBO、纹理、宽高 → Init 里创建 FBO+纹理、检查 COMPLETE、SetViewportTexture → SceneViewportWindow 用 ImGui::Image 显示纹理。
- **小目标 2**：State 加 scene_program、scene_vao → 加载顶点/片段着色器（gl_VertexID）、创建 program、glGenVertexArrays。
- **小目标 3**：每帧先绑定 FBO、viewport、clear、program、VAO、glDrawArrays(3)、再绑定回 0；退出时释放纹理、FBO、program、VAO。

---

## 第六部分：遇到问题时看哪

- **场景视口是黑的/灰的**：先确认小目标 1 做到了没有（FBO 是否 COMPLETE？SetViewportTexture 和 ImGui::Image 传的纹理 ID、尺寸对吗？）。
- **仍然没有三角形**：在 Core Profile 下必须绑定 VAO 再 glDrawArrays；确认 state->scene_vao 已创建并在绘制前 glBindVertexArray(state->scene_vao)。
- **着色器报错「gl_VertexIDx」**：把源码里的 gl_VertexIndex 替换成 gl_VertexID 时，替换长度必须是 **14**（strlen("gl_VertexIndex")），否则会留下尾字母 x 变成未声明标识符 gl_VertexIDx。
- **格式/链接错误**：检查顶点与片段着色器 out/in 变量名、location 是否一致；Mac 上若用 #version 410 不可用可改为 #version 150。

---

## 最后：一句话总结

**用 OpenGL 做一张「图」（FBO + 纹理），每帧先把三角形画到这张图上（绑定 FBO、绑定 VAO、glDrawArrays），再让 ImGui 在场景视口里把这张图显示出来；Core 下一定要有 VAO，着色器用 gl_VertexID。**

按「小目标 1 → 2 → 3」的顺序做，做到哪一步卡住了，就针对那一步查文档或排查，会容易很多。
