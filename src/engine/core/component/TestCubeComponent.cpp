//
// Created by ImSetal on 2026/2/16.
//

#include "TestCubeComponent.h"

#include <format>
#include <glm/gtc/type_ptr.hpp>

#include "TransformComponent.h"
#include "../Entity.h"
#include "../Log.h"
#include "../../../State.h"
#include "glad/glad.h"
#include "glm/glm.hpp"

namespace DE {
    const char * TestCubeComponent::GetComponentName() const {
        return "测试正方体网格组件";
    }

    bool TestCubeComponent::EditorStart(void *appstate) {
        return IComponent::EditorStart(appstate);
    }

    bool TestCubeComponent::Start(void *appstate) {
        return IComponent::Start(appstate);
    }

    bool TestCubeComponent::Event() {
        return IComponent::Event();
    }

    bool TestCubeComponent::LogicIterate(void *appstate) {
        auto* state = static_cast<AppState*>(appstate);
        auto* transform_component = GetOwner()->GetComponent<TransformComponent>();
        transform_component->rotation = glm::vec3(run_time*40, run_time*30, 0.0f);
        run_time += state->delta_time;
        return IComponent::LogicIterate(appstate);
    }

    bool TestCubeComponent::RenderIterate(void *appstate, RenderContext* render_context) {
        if (!GetOwner()) return IComponent::RenderIterate(appstate, render_context);
        auto* state = static_cast<AppState*>(appstate);
        auto* transform_component = GetOwner()->GetComponent<TransformComponent>();

        if (cubeVAO == 0) { // 准备渲染组件所需数据
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
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            //设置顶点法向属性指针
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
        }

        if (render_context) {
            glm::mat4 projection = glm::perspective(glm::radians(render_context->camera->fov),
                (float)*render_context->screenWidth / (float)*render_context->screenHeight, 0.1f, 100.0f);
            glm::mat4 view = render_context->camera->GetViewMatrix();
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, transform_component->position_world);
            model = glm::rotate(model, glm::radians(transform_component->rotation_world.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(transform_component->rotation_world.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(transform_component->rotation_world.z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, transform_component->scale_world);


            glUniformMatrix4fv(glGetUniformLocation(*render_context->program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(*render_context->program, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(*render_context->program, "model"), 1, GL_FALSE, glm::value_ptr(model));

            // 定向光
            if (render_context->dirLight) {
                glUniform3f(glGetUniformLocation(*render_context->program, "dirLight.direction"),
                            render_context->dirLight->direction.x,
                            render_context->dirLight->direction.y,
                            render_context->dirLight->direction.z);
                glUniform3f(glGetUniformLocation(*render_context->program, "dirLight.ambient"),
                            render_context->dirLight->ambient.x,
                            render_context->dirLight->ambient.y,
                            render_context->dirLight->ambient.z);
                glUniform3f(glGetUniformLocation(*render_context->program, "dirLight.diffuse"),
                            render_context->dirLight->diffuse.x,
                            render_context->dirLight->diffuse.y,
                            render_context->dirLight->diffuse.z);
                glUniform3f(glGetUniformLocation(*render_context->program, "dirLight.specular"),
                            render_context->dirLight->specular.x,
                            render_context->dirLight->specular.y,
                            render_context->dirLight->specular.z);
            }

            // 传入相机位置
            glUniform3fv(glGetUniformLocation(*render_context->program, "viewPos"), 1, glm::value_ptr(*render_context->camera->position));

            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        return IComponent::RenderIterate(appstate, render_context);
    }

    bool TestCubeComponent::End() {
        return IComponent::End();
    }

    bool TestCubeComponent::Quit() {
        return IComponent::Quit();
    }
} // DE