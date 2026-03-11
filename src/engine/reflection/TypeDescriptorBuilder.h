/**
 * @file TypeDescriptorBuilder.h
 * @brief RawTypeDescriptorBuilder 与 TypeDescriptorBuilder<T>，及 AddClass 入口
 *
 * 包含：RawTypeDescriptorBuilder（持有并构建 TypeDescriptor，析构时注册）、
 * TypeDescriptorBuilder<T>（流式封装）、AddClass<T>(name)。
 * RawTypeDescriptorBuilder 构造/析构实现在 TypeDescriptorBuilder.cpp。
 */
#pragma once

#include "TypeDescriptor.h"

#include <memory>
#include <string>

namespace DE {
    namespace Reflect {
        /**
         * 持有并构建一个 TypeDescriptor，析构时自动注册到 Registry。
         * 通过 AddMemberVar / AddMemberFunc 填充成员，不暴露类型 T；构造时创建 TypeDescriptor，
         * 析构时将其移交给 Registry::Register。TypeDescriptorBuilder<T> 对其做类型化封装并支持链式调用。
         */
        class RawTypeDescriptorBuilder {
        public:
            /// 创建并持有 TypeDescriptor，并设置其名称（实现在 TypeDescriptorBuilder.cpp）。
            explicit RawTypeDescriptorBuilder(const std::string &name);

            /// 析构时将 TypeDescriptor 交给 Registry 注册（实现在 TypeDescriptorBuilder.cpp）。
            ~RawTypeDescriptorBuilder();

            RawTypeDescriptorBuilder(const RawTypeDescriptorBuilder &) = delete;

            RawTypeDescriptorBuilder &operator=(const RawTypeDescriptorBuilder &) = delete;

            RawTypeDescriptorBuilder(RawTypeDescriptorBuilder &&) = default;

            RawTypeDescriptorBuilder &operator=(RawTypeDescriptorBuilder &&) = default;

            /// 添加一个成员变量：名称 name，成员指针 var，并加入 member_vars_。
            /// @tparam C 类类型；@tparam T 成员变量类型
            template<typename C, typename T>
            void AddMemberVar(const std::string &name, T C::*var) {
                MemberVariable mv{var};
                mv.name_ = name;
                desc_->member_vars_.push_back(std::move(mv));
            }

            /// 添加一个成员函数：名称 name，函数指针 func，并加入 member_funcs_。
            template<typename FUNC>
            void AddMemberFunc(const std::string &name, FUNC func) {
                MemberFunction mf{func};
                mf.name_ = name;
                desc_->member_funcs_.push_back(std::move(mf));
            }

            /// 设置“对象包装”函数，用于将 const void* / void* 转为 std::any（序列化/反序列化用）。
            template<typename T>
            void SetObjectType() {
                desc_->wrap_object_ = [](const void *p) {
                    return std::any(static_cast<const T *>(p));
                };
                desc_->wrap_mutable_object_ = [](void *p) {
                    return std::any(static_cast<T *>(p));
                };
            }

        private:
            std::unique_ptr<TypeDescriptor> desc_{nullptr};
        };

        /**
         * 类型 T 的流式描述符构建器，供 AddClass<T>(name) 返回。
         * 封装 RawTypeDescriptorBuilder，提供链式 AddMemberVar / AddMemberFunc，成员指针类型限定为 T 的成员，
         * 构建完成后在 Raw 析构时自动注册到 Registry。
         * @tparam T 要注册到反射系统的类类型
         */
        template<typename T>
        class TypeDescriptorBuilder {
        public:
            /// 用类型名 name 创建 builder，内部创建 RawTypeDescriptorBuilder(name)。
            explicit TypeDescriptorBuilder(const std::string &name) : raw_builder_(name) {
                raw_builder_.SetObjectType<T>();
            }

            /// 链式添加成员变量并返回 *this。
            /// @tparam T 当前注册的类类型（TypeDescriptorBuilder<T>）；@tparam V 成员变量类型
            template<typename V>
            TypeDescriptorBuilder &AddMemberVar(const std::string &name, V T::*var) {
                raw_builder_.AddMemberVar(name, var);
                return *this;
            }

            /// 链式添加成员函数并返回 *this。
            template<typename FUNC>
            TypeDescriptorBuilder &AddMemberFunc(const std::string &name, FUNC func) {
                raw_builder_.AddMemberFunc(name, func);
                return *this;
            }

        private:
            RawTypeDescriptorBuilder raw_builder_;
        };
    } // namespace Reflect
} // namespace DE
