//
// Created by ImSetal on 2026/2/16.
//

#include "TestCubeComponent.h"

#include "glad/glad.h"
#include "glm/ext/matrix_clip_space.inl"

namespace DE {
    const char * TestCubeComponent::GetComponentName() const {
        return "测试正方体网格组件";
    }

    bool TestCubeComponent::Init(void *appstate) {
        //创建VBO对象
        glGenBuffers(1, &VBO);
        //绑定VBO对象
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        ////把顶点数据复制到缓冲中供OpenGL使用
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
        //创建cubeVAO对象
        glGenVertexArrays(1, &cubeVAO);
        //绑定cubeVAO对象
        glBindVertexArray(cubeVAO);
        //设置顶点位置属性指针
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        return IComponent::Init(appstate);
    }

    bool TestCubeComponent::Event() {
        return IComponent::Event();
    }

    bool TestCubeComponent::LogicIterate(void *appstate) {
        return IComponent::LogicIterate(appstate);
    }

    bool TestCubeComponent::RenderIterate(void *appstate, RenderContext* render_context) {
        if (render_context) {
            // glm::mat4 projection = glm::perspective(glm::radians(render_context->camera->Fov), (float)screenWidth / screenHeight, 0.1f, 100.0f); //创建fov为相机fov，长宽比与窗口长宽比一致的透视投影矩阵
            // glm::mat4 view = camera.GetViewMatrix();
            // glm::mat4 model;
        }
        return IComponent::RenderIterate(appstate, render_context);
    }

    bool TestCubeComponent::Quit() {
        return IComponent::Quit();
    }
} // DE