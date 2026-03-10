/**
 * @file reflect.cpp
 * @brief 反射库核心实现
 *
 * 本文件实现：
 * - RawTypeDescriptorBuilder：构造时创建 TypeDescriptor 并设名称，析构时交给 Registry 注册
 * - Registry::Find(name)：按名称查找并返回 TypeDescriptor 指针
 * - Registry::Register：将 TypeDescriptor 存入 type_descs_ 映射
 * - Registry::Clear：清空所有已注册类型描述
 * - GetByName(name)：对外接口，返回按名称查到的 TypeDescriptor 引用
 * - ClearRegistry()：对外接口，清空全局类型注册表
 */
#include "reflect.hpp"

#include <iostream>

namespace reflect {
namespace details {

/// 创建 TypeDescriptor 并设置其名称。
RawTypeDescriptorBuilder::RawTypeDescriptorBuilder(const std::string &name)
    : desc_(std::make_unique<TypeDescriptor>()) {
  desc_->name_ = name;
}

/// 析构时将描述符移交给 Registry 注册。
RawTypeDescriptorBuilder::~RawTypeDescriptorBuilder() {
  Registry::instance().Register(std::move(desc_));
}

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

}  // namespace details

namespace {
using namespace details;
}  // namespace

/// 对外接口：按类型名查找并返回 TypeDescriptor 引用。
details::TypeDescriptor &GetByName(const std::string &name) {
  return *Registry::instance().Find(name);
}

/// 对外接口：清空全局类型注册表。
void ClearRegistry() {
  Registry::instance().Clear();
}

}  // namespace reflect
