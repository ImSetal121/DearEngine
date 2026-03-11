/**
 * @file Registry.cpp
 * @brief Registry 的 Find / Register / Clear 实现
 */
#include "Registry.h"

namespace DE {
    namespace Reflect {
        /// 按名称在 type_descs_ 中查找并返回 TypeDescriptor 指针。
        TypeDescriptor *Registry::Find(const std::string &name) {
            return type_descs_.find(name)->second.get();
        }

        /// 将描述符以类型名为键存入 type_descs_。
        void Registry::Register(std::unique_ptr<TypeDescriptor> desc) {
            auto name = desc->name();
            type_descs_[name] = std::move(desc);
        }

        /// 清空 type_descs_（通过 swap 与临时 map 交换）。
        void Registry::Clear() {
            decltype(type_descs_) tmp;
            tmp.swap(type_descs_);
        }

        /// 对外接口：按类型名查找并返回 TypeDescriptor 引用。
        TypeDescriptor &GetByName(const std::string &name) {
            return *Registry::instance().Find(name);
        }

        /// 对外接口：清空全局类型注册表。
        void ClearRegistry() {
            Registry::instance().Clear();
        }
    } // namespace Reflect
} // namespace DE
