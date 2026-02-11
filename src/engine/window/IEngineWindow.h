//
// Created by ImSetal on 2026/2/8.
//

#ifndef DEARENGINE_IENGINEWINDOW_H
#define DEARENGINE_IENGINEWINDOW_H

/**
 * 引擎窗口抽象基类。
 * 所有可停靠的引擎窗口（控制台、场景树、GameObject 组件、场景编辑视口）继承此类，
 * 实现 Title() 和 Draw()，由主循环或 EngineWindowManager 统一调用。
 */
class IEngineWindow {
public:
    virtual ~IEngineWindow() = default;
    /** 窗口标题，用于 ImGui::Begin(title, ...) */
    virtual const char* Title() const = 0;
    /** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
    virtual void Draw() = 0;
    /** 是否显示该窗口，可由菜单栏 Checkbox 等绑定 */
    bool open = true;
};

#endif //DEARENGINE_IENGINEWINDOW_H