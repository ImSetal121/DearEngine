/**
 * @file MemberFunction.h
 * @brief 单个成员函数的反射描述
 *
 * 模板参数命名约定：C = 类类型，R = 返回类型，Args = 参数类型
 */
#pragma once

#include <any>
#include <functional>
#include <string>
#include <tuple>

namespace DE {
namespace Reflect {

/**
 * 描述某个类的单个成员函数。
 * 从成员函数指针构造，支持 const/非 const、有返回值/void；通过 Invoke 在给定对象上以
 * std::any 参数调用，返回值也包装为 std::any。供 TypeDescriptor 与按名调用成员函数使用。
 */
class MemberFunction {
 public:
  /// 默认构造，未绑定任何函数。
  MemberFunction() = default;

  /// 从非 const 有返回值成员函数指针构造，内部用 std::apply 调用。
  /// @tparam C 类类型；@tparam R 返回类型；@tparam Args 参数类型
  template <typename C, typename R, typename... Args>
  explicit MemberFunction(R (C::*func)(Args...)) {
    fn_ = [this, func](std::any obj_args) -> std::any {
      using tuple_t = std::tuple<C &, Args...>;
      auto *tp_ptr = std::any_cast<tuple_t *>(obj_args);
      return std::apply(func, *tp_ptr);
    };
  }

  /// 从非 const void 成员函数指针构造。
  /// @tparam C 类类型；@tparam Args 参数类型
  template <typename C, typename... Args>
  explicit MemberFunction(void (C::*func)(Args...)) {
    fn_ = [this, func](std::any obj_args) -> std::any {
      using tuple_t = std::tuple<C &, Args...>;
      auto *tp_ptr = std::any_cast<tuple_t *>(obj_args);
      std::apply(func, *tp_ptr);
      return std::any{};
    };
  }

  /// 从 const 有返回值成员函数指针构造，并标记 is_const_ = true。
  /// @tparam C 类类型；@tparam R 返回类型；@tparam Args 参数类型
  template <typename C, typename R, typename... Args>
  explicit MemberFunction(R (C::*func)(Args...) const) {
    fn_ = [this, func](std::any obj_args) -> std::any {
      using tuple_t = std::tuple<const C &, Args...>;
      auto *tp_ptr = std::any_cast<tuple_t *>(obj_args);
      return std::apply(func, *tp_ptr);
    };
    is_const_ = true;
  }

  /// 从 const void 成员函数指针构造，并标记 is_const_ = true。
  /// @tparam C 类类型；@tparam Args 参数类型
  template <typename C, typename... Args>
  explicit MemberFunction(void (C::*func)(Args...) const) {
    fn_ = [this, func](std::any obj_args) -> std::any {
      using tuple_t = std::tuple<const C &, Args...>;
      auto *tp_ptr = std::any_cast<tuple_t *>(obj_args);
      std::apply(func, *tp_ptr);
      return std::any{};
    };
    is_const_ = true;
  }

  /// 返回本成员函数的名称。
  const std::string &name() const {
    return name_;
  }

  /// 返回是否为 const 成员函数。
  bool is_const() const {
    return is_const_;
  }

  /// 在对象 c 上以参数 args 调用该成员函数，返回值包装为 std::any（void 则空 any）。
  /// @tparam C 对象所属类类型；@tparam Args 参数类型
  template <typename C, typename... Args>
  std::any Invoke(C &c, Args &&... args) {
    if (is_const_) {
      auto tp = std::make_tuple(std::reference_wrapper<const C>(c), args...);
      return fn_(&tp);
    }
    auto tp = std::make_tuple(std::reference_wrapper<C>(c), args...);
    return fn_(&tp);
  }

 private:
  friend class RawTypeDescriptorBuilder;

  std::string name_;
  bool is_const_{false};
  std::function<std::any(std::any)> fn_{nullptr};
};

}  // namespace Reflect
}  // namespace DE
