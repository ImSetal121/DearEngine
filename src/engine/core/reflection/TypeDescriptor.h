/**
 * @file TypeDescriptor.h
 * @brief 类型的运行时描述符，包含成员变量与成员函数列表及按名查找
 */
#pragma once

#include "MemberFunction.h"
#include "MemberVariable.h"

#include <functional>
#include <string>
#include <vector>

namespace DE {
    namespace Reflect {
        /**
         * 类型的运行时描述符。
         * 保存类型名称、成员变量列表（MemberVariable）、成员函数列表（MemberFunction），
         * 并提供按名称查找 GetMemberVar / GetMemberFunc。由 RawTypeDescriptorBuilder 构建，并注册到 Registry。
         */
        class TypeDescriptor {
        public:
            /// 返回类型名称。
            const std::string &name() const {
                return name_;
            }

            /// 返回该类型的所有成员变量描述列表。
            const std::vector<MemberVariable> &member_vars() const {
                return member_vars_;
            }

            /// 返回该类型的所有成员函数描述列表。
            const std::vector<MemberFunction> &member_funcs() const {
                return member_funcs_;
            }

            /// 按名称查找成员变量，未找到则返回默认构造的 MemberVariable。
            MemberVariable GetMemberVar(const std::string &name) const {
                for (const auto &mv: member_vars_) {
                    if (mv.name() == name) {
                        return mv;
                    }
                }
                return MemberVariable{};
            }

            /// 按名称查找成员函数，未找到则返回默认构造的 MemberFunction。
            MemberFunction GetMemberFunc(const std::string &name) const {
                for (const auto &mf: member_funcs_) {
                    if (mf.name() == name) {
                        return mf;
                    }
                }
                return MemberFunction{};
            }

            /// 将 const void* 包装为该类型对象的 std::any（供 GetValueAny 等使用）。
            std::any WrapObject(const void *obj) const {
                return wrap_object_ ? wrap_object_(obj) : std::any{};
            }

            /// 将 void* 包装为该类型对象的 std::any（供 SetValueAny 等反序列化使用）。
            std::any WrapMutableObject(void *obj) const {
                return wrap_mutable_object_ ? wrap_mutable_object_(obj) : std::any{};
            }

        private:
            friend class RawTypeDescriptorBuilder;

            std::string name_;
            std::function<std::any(const void *)> wrap_object_{nullptr};
            std::function<std::any(void *)> wrap_mutable_object_{nullptr};
            std::vector<MemberVariable> member_vars_;
            std::vector<MemberFunction> member_funcs_;
        };
    } // namespace Reflect
} // namespace DE
