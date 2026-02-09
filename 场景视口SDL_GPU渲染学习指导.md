# 在「场景视口」里画一个三角形 — 新手学习指导

---

## 新手先看这里（3 句话）

1. **目标**：让「场景视口」窗口里出现一个彩色三角形，而不是空白。
2. **做法**：你先让 GPU 把三角形画到「一张图」上，再让 ImGui 把这张图显示在场景视口里。
3. **顺序**：每一帧先画三角形到这张图，再画整个界面（这时界面里就会把这张图贴在场景视口里）。

下面用大白话和最小步骤讲清楚怎么做。

---

## 第一部分：用大白话理解「两张图」

可以这么想：

- **你看到的整个窗口** = 一张最终贴在屏幕上的大图。  
  你的主程序现在已经在做一件事：把 ImGui 画出来的所有界面（菜单、控制台、场景视口窗口框……）画到这张大图上。  
  这张大图在代码里叫「交换链」或「swapchain」，你不用自己造，SDL 会给你。

- **场景视口里那一块** = 我们想让它显示「另一张图」的内容。  
  这张图你要自己造：先让 GPU 在这张图上画三角形，再告诉 ImGui：「把这张图贴在场景视口里显示出来」。

所以：

- **第一张图**：你自己创建的、专门给场景视口用的「一张图」。GPU 先把三角形画在这张图上。
- **第二张图**：整个窗口用的那张大图。程序再把 ImGui（包括「在场景视口里显示第一张图」）画到这张大图上。

在代码里，「一张图」通常叫 **纹理（texture）**。  
所以：  
**你要做的就是：创建一张纹理 → 每帧把三角形画到这张纹理上 → 在场景视口里用 ImGui 把这张纹理当图片显示出来。**

---

## 第二部分：你需要会用的三件事（最少概念）

### 1. 纹理 = 一张可以画东西上去的「图」

- 交换链那张图：SDL 给你的，不能自己创建。
- 场景视口用的这张图：你要用 SDL 的接口**自己创建**一张（比如 256×256 大小）。  
  创建好后，你就有一个「纹理」了，后面两步都会用到它。

### 2. 「画到某张纹理上」= 一次「画」的动作

- 你的主程序里已经有「画 ImGui 到窗口」的这段逻辑：本质就是「画到交换链那张纹理上」。
- 示例项目 `example_sdl3_gpu_triangle` 里是「画三角形到交换链那张纹理上」。
- 我们要加的是：**再多一次「画」的动作**，但这次是「画三角形到我们自己的那张纹理上」。  
  画完之后，这张纹理里就有三角形了；然后再像现在这样画 ImGui，ImGui 会在场景视口里把这张纹理显示出来。

### 3. 在 ImGui 里显示这张纹理

- ImGui 有一个「显示图片」的接口：给一张纹理 + 宽高，它就会在窗口里把这张图贴出来。
- 在「场景视口」窗口里，你调用这个接口，把上面那张「画了三角形的纹理」传进去，场景视口里就会看到三角形。

总结：**创建一张纹理 → 每帧先画三角形到这张纹理 → 再画 ImGui（其中场景视口里用「显示图片」把这张纹理贴出来）。**

---

## 第三部分：一帧里应该先做什么、后做什么

每一帧的逻辑顺序要这样（不用背，知道「先画场景的图，再画整屏界面」就行）：

1. **先**：把三角形画到「场景视口用的那张纹理」上。
2. **再**：像现在一样，把 ImGui 画到窗口上（这时 ImGui 会把场景视口那块画成「你那张纹理」的内容）。

这样你才能在场景视口里看到三角形。如果顺序反了，就会出问题。

---

## 第四部分：自己动手的 3 个小目标（按顺序做）

下面每一步都是「先做到这个，再往下」。不用一次做完。每个小目标下面都写了**应该在哪里添加什么代码**，按文件、按位置对照着改即可。

---

### 小目标 1：让场景视口里出现「一张图」（先不画三角形也行）

- **你要做的**：  
  - 在程序启动时，用 SDL 的接口**创建一张小图**（纹理），比如 256×256。  
  - 在「场景视口」窗口的绘制代码里，用 ImGui 的「显示图片」接口，把这张纹理显示出来（先不用管图里有没有内容，哪怕全黑也行）。
- **做完之后**：运行程序，打开场景视口，你应该能看到一块区域（可能是黑的或有一块图），说明 ImGui 已经能显示你创建的这张纹理了。

下面是要添加的代码片段，按顺序复制到对应位置即可。

**① `src/main.cpp` — 在 `struct AppState` 里增加成员（放在其它引擎窗口指针下面）：**

```cpp
    // 场景视口用的纹理（小目标 1）
    SDL_GPUTexture* scene_viewport_texture = nullptr;
    int viewport_tex_w = 256;
    int viewport_tex_h = 256;
```

**② `src/main.cpp` — 在 `SDL_AppInit` 里，`state->scene_viewport_window = new SceneViewportWindow();` 之后、`*appstate = state;` 之前，添加：**

```cpp
        // 创建场景视口用的纹理（可渲染 + 可被 ImGui 采样）
        SDL_GPUTextureCreateInfo tex_info = {};
        tex_info.type = SDL_GPU_TEXTURETYPE_2D;
        tex_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        tex_info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
        tex_info.width = (Uint32)state->viewport_tex_w;
        tex_info.height = (Uint32)state->viewport_tex_h;
        tex_info.layer_count_or_depth = 1;
        tex_info.num_levels = 1;
        tex_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
        tex_info.props = 0;
        state->scene_viewport_texture = SDL_CreateGPUTexture(state->gpu_device, &tex_info);
        if (state->scene_viewport_texture)
            state->scene_viewport_window->SetViewportTexture(state->scene_viewport_texture, state->viewport_tex_w, state->viewport_tex_h);
```

**③ `src/engine/window/SceneViewportWindow.h` — 在类里增加（`Draw()` 声明上面）：**

```cpp
    void SetViewportTexture(void* texture, int width, int height);

private:
    void* viewport_texture_ = nullptr;
    int viewport_tex_w_ = 0;
    int viewport_tex_h_ = 0;
```

**④ `src/engine/window/SceneViewportWindow.cpp` — 增加 `#include <cstdint>`，实现 SetViewportTexture，并改 Draw：**

```cpp
#include <cstdint>

void SceneViewportWindow::SetViewportTexture(void* texture, int width, int height) {
    viewport_texture_ = texture;
    viewport_tex_w_ = width;
    viewport_tex_h_ = height;
}

void SceneViewportWindow::Draw() {
    if (!open) return;
    ImGui::Begin(Title());
    if (viewport_texture_ && viewport_tex_w_ > 0 && viewport_tex_h_ > 0)
        ImGui::Image((ImTextureID)(intptr_t)viewport_texture_, ImVec2((float)viewport_tex_w_, (float)viewport_tex_h_));
    ImGui::End();
}
```

---

### 小目标 2：准备好「画三角形」需要的东西（先不画也行）

- **你要做的**：  
  - 参考 `example_sdl3_gpu_triangle` 这个示例项目，它里面已经会「画一个三角形」了。  
  - 把里面「加载着色器、创建画三角形的管线」那部分看懂或抄过来，放到你的主程序里，在启动时执行一次。  
  - 注意：示例里是画到「交换链」；我们要画到「自己的纹理」，所以创建管线时，**输出格式要和你创建的那张纹理的格式一致**（比如都用 R8G8B8A8 这种常见格式）。
- **做完之后**：程序能正常启动、不报错就行，画面上可以暂时还看不到三角形（因为还没在每帧里真正画）。

**说明**：请把 `example_sdl3_gpu_triangle` 里的 `vert.spv`、`frag.spv`（或 Mac 上 `vert.msl`、`frag.msl`）复制到程序能读到的目录，下面代码里 `shader_dir` 填该目录（如 `"."` 或 `GetEngineAssetsPath().c_str()`）。

**⑤ `src/main.cpp` — 在 `struct AppState` 里再增加：**

```cpp
    SDL_GPUShader* scene_vertex_shader = nullptr;
    SDL_GPUShader* scene_fragment_shader = nullptr;
    SDL_GPUGraphicsPipeline* scene_triangle_pipeline = nullptr;
```

**⑥ `src/main.cpp` — 在 `GetEngineAssetsPath()` 下面增加两个辅助函数：**

```cpp
static SDL_GPUShader* LoadSceneShader(SDL_GPUDevice* device, const char* filename, SDL_Storage* storage, SDL_GPUShaderStage stage, SDL_GPUShaderFormat format) {
    Uint64 file_size;
    if (!SDL_GetStorageFileSize(storage, filename, &file_size)) return nullptr;
    std::vector<Uint8> data(file_size);
    if (!SDL_ReadStorageFile(storage, filename, data.data(), file_size)) return nullptr;
    SDL_GPUShaderCreateInfo ci = {};
    ci.code = data.data();
    ci.code_size = data.size();
    ci.entrypoint = "main0";
    ci.format = format;
    ci.num_samplers = 0;
    ci.num_storage_buffers = 0;
    ci.num_storage_textures = 0;
    ci.num_uniform_buffers = 0;
    ci.stage = stage;
    ci.props = 0;
    return SDL_CreateGPUShader(device, &ci);
}

static SDL_GPUGraphicsPipeline* CreateSceneTrianglePipeline(SDL_GPUDevice* device, SDL_GPUShader* vs, SDL_GPUShader* fs) {
    SDL_GPUGraphicsPipelineCreateInfo ci = {};
    ci.vertex_input_state.num_vertex_attributes = 0;
    ci.vertex_input_state.num_vertex_buffers = 0;
    ci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    ci.vertex_shader = vs;
    ci.fragment_shader = fs;
    ci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    ci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    ci.multisample_state.enable_mask = false;
    ci.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    ci.target_info.num_color_targets = 1;
    ci.target_info.has_depth_stencil_target = false;
    SDL_GPUColorTargetDescription desc = {};
    desc.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    desc.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    desc.blend_state.color_write_mask = SDL_GPU_COLORCOMPONENT_A | SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B;
    desc.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    desc.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    desc.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    desc.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    desc.blend_state.enable_blend = true;
    desc.blend_state.enable_color_write_mask = false;
    desc.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    ci.target_info.color_target_descriptions = &desc;
    return SDL_CreateGPUGraphicsPipeline(device, &ci);
}
```

**⑦ `src/main.cpp` — 在 `SDL_AppInit` 里，创建完视口纹理并 `SetViewportTexture` 之后、`*appstate = state;` 之前，添加：**

```cpp
        const char* shader_dir = ".";
        SDL_Storage* storage = SDL_OpenFileStorage(shader_dir);
        if (storage) {
            while (!SDL_StorageReady(storage)) SDL_Delay(1);
            const char* backend = SDL_GetGPUDeviceDriver(state->gpu_device);
            bool use_metal = backend && (SDL_strcasecmp(backend, "metal") == 0);
            if (use_metal) {
                state->scene_vertex_shader   = LoadSceneShader(state->gpu_device, "vert.msl", storage, SDL_GPU_SHADERSTAGE_VERTEX,   SDL_GPU_SHADERFORMAT_MSL);
                state->scene_fragment_shader = LoadSceneShader(state->gpu_device, "frag.msl", storage, SDL_GPU_SHADERSTAGE_FRAGMENT, SDL_GPU_SHADERFORMAT_MSL);
            } else {
                state->scene_vertex_shader   = LoadSceneShader(state->gpu_device, "vert.spv", storage, SDL_GPU_SHADERSTAGE_VERTEX,   SDL_GPU_SHADERFORMAT_SPIRV);
                state->scene_fragment_shader = LoadSceneShader(state->gpu_device, "frag.spv", storage, SDL_GPU_SHADERSTAGE_FRAGMENT, SDL_GPU_SHADERFORMAT_SPIRV);
            }
            SDL_CloseStorage(storage);
            if (state->scene_vertex_shader && state->scene_fragment_shader)
                state->scene_triangle_pipeline = CreateSceneTrianglePipeline(state->gpu_device, state->scene_vertex_shader, state->scene_fragment_shader);
        }
```

（若没有 `#include <vector>`，在文件顶部加一行。）

---

### 小目标 3：每帧先画三角形到你的纹理，再画 ImGui

- **你要做的**：  
  - 在「主循环」里，找到「取交换链、画 ImGui」的那段代码。  
  - 在**画 ImGui 之前**，加一段：以「场景视口用的那张纹理」为目标，清空、绑定三角形管线、画 3 个顶点、结束这次「画」。  
  - 具体要调哪些函数，对照 `example_sdl3_gpu_triangle/main.cpp` 里「画三角形」的那几行，只是把「画到交换链」改成「画到我这张纹理」。
- **做完之后**：场景视口里能看到彩色三角形。

**⑧ `src/main.cpp` — 在 `SDL_AppIterate` 里，在 `SDL_AcquireGPUCommandBuffer(state->gpu_device);` 之后、`SDL_WaitAndAcquireGPUSwapchainTexture(...)` 之前，插入：**

```cpp
    if (state->scene_viewport_texture && state->scene_triangle_pipeline && state->viewport_tex_w > 0 && state->viewport_tex_h > 0) {
        SDL_GPUColorTargetInfo viewport_target = {};
        viewport_target.texture = state->scene_viewport_texture;
        viewport_target.clear_color = SDL_FColor{ 0.1f, 0.1f, 0.1f, 1.0f };
        viewport_target.load_op = SDL_GPU_LOADOP_CLEAR;
        viewport_target.store_op = SDL_GPU_STOREOP_STORE;
        viewport_target.mip_level = 0;
        viewport_target.layer_or_depth_plane = 0;
        viewport_target.cycle = false;
        SDL_GPURenderPass* rp = SDL_BeginGPURenderPass(command_buffer, &viewport_target, 1, nullptr);
        SDL_BindGPUGraphicsPipeline(rp, state->scene_triangle_pipeline);
        SDL_GPUViewport vp = {};
        vp.x = 0;
        vp.y = 0;
        vp.w = state->viewport_tex_w;
        vp.h = state->viewport_tex_h;
        vp.min_depth = 0.0f;
        vp.max_depth = 1.0f;
        SDL_SetGPUViewport(rp, &vp);
        SDL_DrawGPUPrimitives(rp, 3, 1, 0, 0);
        SDL_EndGPURenderPass(rp);
    }
```

**⑨ 退出时释放 — 在 `SDL_AppQuit` 里，`SDL_WaitForGPUIdle` 之后、ImGui 关闭之前，添加：**

```cpp
    if (state->scene_viewport_texture) {
        SDL_ReleaseGPUTexture(state->gpu_device, state->scene_viewport_texture);
        state->scene_viewport_texture = nullptr;
    }
    if (state->scene_triangle_pipeline) {
        SDL_ReleaseGPUGraphicsPipeline(state->gpu_device, state->scene_triangle_pipeline);
        state->scene_triangle_pipeline = nullptr;
    }
    if (state->scene_vertex_shader) {
        SDL_ReleaseGPUShader(state->gpu_device, state->scene_vertex_shader);
        state->scene_vertex_shader = nullptr;
    }
    if (state->scene_fragment_shader) {
        SDL_ReleaseGPUShader(state->gpu_device, state->scene_fragment_shader);
        state->scene_fragment_shader = nullptr;
    }
```

---

## 第五部分：小结

- **小目标 1**：AppState 加视口纹理与宽高 → Init 里创建纹理并 `SetViewportTexture` → SceneViewportWindow 加 `SetViewportTexture` 与成员，Draw 里 `ImGui::Image`。
- **小目标 2**：AppState 加 shader 与管线指针 → 增加 `LoadSceneShader`、`CreateSceneTrianglePipeline` → Init 里用 `shader_dir` 加载 shader、创建管线（格式 `R8G8B8A8_UNORM`）。
- **小目标 3**：在 AcquireGPUCommandBuffer 之后、WaitAndAcquireGPUSwapchainTexture 之前插入「画三角形到视口纹理」的一段；退出时释放纹理、管线、shader。

---

## 第六部分：遇到问题时看哪

- **场景视口是黑的**：先确认小目标 1 做到了没有（纹理创建成功了吗？ImGui::Image 传的纹理和尺寸对吗？）。
- **报错和「格式」有关**：检查你创建纹理时用的格式，和画三角形时管线里设置的「输出格式」是否一致。
- **看不到三角形或花屏**：检查每帧是不是**先**画三角形到你的纹理，**再**画 ImGui；两段「画」要在同一帧、同一条命令缓冲里，顺序不能反。

---

## 最后：一句话总结

**先自己做一张「图」（纹理），每帧先把三角形画到这张图上，再让 ImGui 在场景视口里把这张图显示出来；画三角形的代码可以参考 example_sdl3_gpu_triangle，只是把「画到窗口」改成「画到我这张图」。**

按「小目标 1 → 2 → 3」的顺序做，做到哪一步卡住了，就针对那一步查文档或问人，会容易很多。
