//
// Created by ImSetal on 2026/2/8.
//

#ifndef DEARENGINE_SCENEVIEWPORTWINDOW_H
#define DEARENGINE_SCENEVIEWPORTWINDOW_H
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "IEditorSubWindow.h"
#include "../../core/render_context/ICamera.h"
#include "../../core/render_context/RenderContext.h"

namespace DE {
    class SceneViewportSubWindow : public IEditorSubWindow {
    public:
        // 引擎场景视口绘制
        unsigned int scene_viewport_fbo = 0;        // GL FBO
        unsigned int scene_viewport_texture = 0;    // GL 纹理 id，供 ImGui::Image 使用
        unsigned int scene_viewport_rbo = 0;        // 深度 Renderbuffer
        unsigned int scene_vao = 0;                 // 场景用 VAO（Core 下 glDrawArrays 必须绑定 VAO）
        int scene_viewport_texture_width = 1280;
        int scene_viewport_texture_height = 720;
        // 视口相机
        ICamera* camera = nullptr;
        glm::vec3 camera_position = glm::vec3(-5.0f, 0.0f, 0.0f);
        glm::vec3 camera_rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        float camera_sensitivity = 1.0f;
        float camera_move_speed = 10.0f;
        // 渲染上下文
        RenderContext* render_context = nullptr;

        SceneViewportSubWindow();
        /** 窗口标题，用于 ImGui::Begin(title, ...) */
        const char* Title() const override;
        /** 启动 */
        bool Init(void *appstate) override;
        /** 事件 */
        bool Event() override;
        /** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
        bool LogicIterate(void *appstate) override;
        /** 每帧渲染，内部应包含处理视口纹理绘制等 */
        bool RenderIterate(void *appstate) override;
        /** 结束 */
        bool Quit() override;
        /** 设置视口绘制纹理指针 */
        void SetViewportTexture(void* texture, int width, int height);

    private:
        void* viewport_texture_ = nullptr;
        int viewport_width_ = 0;
        int viewport_height_ = 0;

        /** 在已绑定 scene_viewport_fbo 的前提下，创建或更新颜色纹理与深度 RBO 为指定尺寸；会先删旧再建新。更新 scene_viewport_texture/rbo/width/height 并 SetViewportTexture。返回 FBO 是否完整。 */
        bool EnsureFboAttachments(int width, int height);
    };
}

#endif //DEARENGINE_SCENEVIEWPORTWINDOW_H