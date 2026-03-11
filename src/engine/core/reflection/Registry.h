/**
 * @file Registry.h
 * @brief 全局类型注册表（单例），按类型名存储 TypeDescriptor；对外提供 GetByName、ClearRegistry
 */
#pragma once

#include "TypeDescriptor.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace DE {
    namespace Reflect {
        /**
         * 全局类型注册表（单例）。
         * 以类型名字符串为键存储 TypeDescriptor；提供 Find(name)、Register、Clear。
         * 反射按名称查找类型时通过 GetByName -> Registry::Find 获取 TypeDescriptor。
         */
        class Registry {
        public:
            /// 返回单例引用。
            static Registry &instance() {
                static Registry inst;
                return inst;
            }

            /// 按类型名查找并返回 TypeDescriptor 指针，未找到返回 nullptr（实现在 .cpp）。
            TypeDescriptor *Find(const std::string &name);

            /// 是否存在指定类型名。
            bool Contains(const std::string &name) const;

            /// 将描述符注册到 type_descs_，以类型名为键（实现在 .cpp）。
            void Register(std::unique_ptr<TypeDescriptor> desc);

            /// 清空所有已注册类型描述（实现在 .cpp）。
            void Clear();

        private:
            std::unordered_map<std::string, std::unique_ptr<TypeDescriptor> > type_descs_;
        };

        /// 按类型名查找并返回 TypeDescriptor 的引用。
        TypeDescriptor &GetByName(const std::string &name);

        /// 清空全局类型注册表。
        void ClearRegistry();
    } // namespace Reflect
} // namespace DE
