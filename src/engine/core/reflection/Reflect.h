//
// Created by ImSetal on 2026/3/10.
//

#ifndef DEARENGINE_REFLECT_H
#define DEARENGINE_REFLECT_H
#include "TypeDescriptor.h"
#include "TypeDescriptorBuilder.h"

#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace DE {
    namespace Reflect {
        /// type_index -> 类型名（用于序列化时根据运行时类型得到注册名）
        inline std::unordered_map<std::type_index, std::string> g_typeIndexToName;

        inline void RegisterTypeIndex(std::type_index ti, std::string name) {
            g_typeIndexToName[ti] = std::move(name);
        }

        /// 若该类型曾通过 AddClass 注册，返回类型名，否则返回 nullopt。
        inline std::optional<std::string> GetTypeName(std::type_index ti) {
            auto it = g_typeIndexToName.find(ti);
            if (it == g_typeIndexToName.end())
                return std::nullopt;
            return it->second;
        }

        /// 创建类型 T 的 TypeDescriptorBuilder，用于链式注册成员变量与成员函数。
        template<typename T>
        TypeDescriptorBuilder<T> AddClass(const std::string &name) {
            RegisterTypeIndex(typeid(T), name);
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
