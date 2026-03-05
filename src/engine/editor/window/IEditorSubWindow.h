//
// Created by ImSetal on 2026/2/8.
//

#ifndef DEARENGINE_IENGINEWINDOW_H
#define DEARENGINE_IENGINEWINDOW_H
#include "SDL3/SDL_init.h"

namespace DE {
    /**
     * 引擎窗口抽象基类。
     * 所有可停靠的引擎窗口（控制台、场景树、GameObject 组件、场景编辑视口）继承此类，
     * 实现 Title() 和 Draw()，由主循环或 EngineWindowManager 统一调用。
     */
    class IEditorSubWindow {
    public:
        virtual ~IEditorSubWindow() = default;
        /** 窗口标题，用于 ImGui::Begin(title, ...) */
        virtual const char* Title() const = 0;
        /** 启动 */
        virtual bool Init(void *appstate) {return true;};
        /** 事件 */
        virtual bool Event() {return true;};
        /** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
        virtual bool LogicIterate(void *appstate) {return true;};
        /** 每帧渲染，内部应包含处理视口纹理绘制等 */
        virtual bool RenderIterate(void *appstate) {return true;};
        /** 结束 */
        virtual bool Quit() {return true;};
        /** 是否显示该窗口，可由菜单栏 Checkbox 等绑定 */
        bool open = true;
    };
}

#endif //DEARENGINE_IENGINEWINDOW_H