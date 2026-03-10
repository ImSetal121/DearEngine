/**
 * @file MemberVariable.h
 * @brief 单个成员变量的反射描述
 *
 * 模板参数命名约定：C = 类类型，T = 成员变量类型
 */
#pragma once

#include <any>
#include <functional>
#include <string>

namespace DE {
namespace Reflect {

/**
 * 描述某个类的单个成员变量。
 * 持有一个成员指针的“抽象视图”：通过 name()、GetValue、SetValue 按名称读写任意对象上该成员，
 * 内部用 std::any + lambda 封装 getter/setter，供 TypeDescriptor 与反射调用使用。
 */
class MemberVariable {
 public:
  /// 默认构造，getter/setter 为空。
  MemberVariable() = default;

  /// 从成员指针 var 构造：绑定 getter（读 obj->*var）与 setter（写 obj->*var）。
  /// @tparam C 类类型（拥有该成员的类）；@tparam T 成员变量类型
  template <typename C, typename T>
  MemberVariable(T C::*var) {
    getter_ = [var](std::any obj) -> std::any {
      return std::any_cast<const C *>(obj)->*var;
    };
    setter_ = [var](std::any obj, std::any val) {
      // Syntax: https://stackoverflow.com/a/670744/12003165
      // `obj.*member_var`
      auto *self = std::any_cast<C *>(obj);
      self->*var = std::any_cast<T>(val);
    };
  }

  /// 返回本成员变量的名称。
  const std::string &name() const {
    return name_;
  }

  /// 从对象 c 读取该成员的值，以类型 T 返回。
  /// @tparam T 成员值类型；@tparam C 对象所属类类型
  template <typename T, typename C>
  T GetValue(const C &c) const {
    return std::any_cast<T>(getter_(&c));
  }

  /// 将值 val 写入对象 c 的该成员。
  /// @tparam C 对象所属类类型；@tparam T 成员值类型
  template <typename C, typename T>
  void SetValue(C &c, T val) {
    setter_(&c, val);
  }

 private:
  friend class RawTypeDescriptorBuilder;

  std::string name_;
  std::function<std::any(std::any)> getter_{nullptr};
  std::function<void(std::any, std::any)> setter_{nullptr};
};

}  // namespace Reflect
}  // namespace DE
