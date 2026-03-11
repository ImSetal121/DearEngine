//
// Created by ImSetal on 2026/3/10.
//

#ifndef DEARENGINE_REFLECT_H
#define DEARENGINE_REFLECT_H
#include "TypeDescriptor.h"
#include "TypeDescriptorBuilder.h"

namespace DE {
    namespace Reflect {
        /// 创建类型 T 的 TypeDescriptorBuilder，用于链式注册成员变量与成员函数。
        template<typename T>
        TypeDescriptorBuilder<T> AddClass(const std::string &name) {
            TypeDescriptorBuilder<T> b{name};
            return b;
        }

        /// 按类型名查找并返回 TypeDescriptor 的引用。
        TypeDescriptor &GetByName(const std::string &name);

        /// 清空全局类型注册表。
        void ClearRegistry();
    }
}

#endif //DEARENGINE_REFLECT_H
