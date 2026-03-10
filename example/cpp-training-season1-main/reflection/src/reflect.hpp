/**
 * @file reflect.hpp
 * @brief C++ 简易运行时反射库头文件
 *
 * 参考：rttr (https://github.com/rttrorg/rttr)、
 *      Preshing 博客 (https://preshing.com/20180116/a-primitive-reflection-system-in-cpp-part-1/)
 *
 * 本文件提供：
 * - MemberVariable：成员变量描述，支持 GetValue/SetValue（内部用 std::any + 成员指针）
 * - MemberFunction：成员函数描述，支持 Invoke，处理 const/非 const、有返回值/void
 * - TypeDescriptor：类型描述，包含名称、成员变量列表、成员函数列表及按名查找接口
 * - RawTypeDescriptorBuilder / TypeDescriptorBuilder：流式构建 TypeDescriptor（AddMemberVar/AddMemberFunc）
 * - Registry：单例，按类型名字符串存储 TypeDescriptor
 * - 对外 API：AddClass<T>(name)、GetByName(name)、ClearRegistry()
 */
#pragma once

#include <any>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
namespace reflect {
namespace details {

class MemberVariable {
 public:
  /// 默认构造，getter/setter 为空。
  MemberVariable() = default;

  /// 从成员指针 var 构造：绑定 getter（读 obj->*var）与 setter（写 obj->*var）。
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
  template <typename T, typename C>
  T GetValue(const C &c) const {
    return std::any_cast<T>(getter_(&c));
  }

  /// 将值 val 写入对象 c 的该成员。
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

class MemberFunction {
 public:
  /// 默认构造，未绑定任何函数。
  MemberFunction() = default;

  /// 从非 const 有返回值成员函数指针构造，内部用 std::apply 调用。
  template <typename C, typename R, typename... Args>
  explicit MemberFunction(R (C::*func)(Args...)) {
    fn_ = [this, func](std::any obj_args) -> std::any {
      using tuple_t = std::tuple<C &, Args...>;
      // How to debug compile-time types...
      // static_assert(std::is_same<tuple_t, void>::value, "Hoi!");
      auto *tp_ptr = std::any_cast<tuple_t *>(obj_args);
      return std::apply(func, *tp_ptr);
    };
  }

  /// 从非 const void 成员函数指针构造。
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
  template <typename C, typename R, typename... Args>
  explicit MemberFunction(R (C::*func)(Args...) const) {
    fn_ = [this, func](std::any obj_args) -> std::any {
      using tuple_t = std::tuple<const C &, Args...>;
      // How to debug compile-time types...
      // static_assert(std::is_same<tuple_t, void>::value, "Hoi!");
      auto *tp_ptr = std::any_cast<tuple_t *>(obj_args);
      return std::apply(func, *tp_ptr);
    };
    is_const_ = true;
  }

  /// 从 const void 成员函数指针构造，并标记 is_const_ = true。
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
    for (const auto &mv : member_vars_) {
      if (mv.name() == name) {
        return mv;
      }
    }
    return MemberVariable{};
  }

  /// 按名称查找成员函数，未找到则返回默认构造的 MemberFunction。
  MemberFunction GetMemberFunc(const std::string &name) const {
    for (const auto &mf : member_funcs_) {
      if (mf.name() == name) {
        return mf;
      }
    }
    return MemberFunction{};
  }

 private:
  friend class RawTypeDescriptorBuilder;

  std::string name_;
  std::vector<MemberVariable> member_vars_;
  std::vector<MemberFunction> member_funcs_;
};

class RawTypeDescriptorBuilder {
 public:
  /// 创建并持有 TypeDescriptor，并设置其名称（实现在 .cpp）。
  explicit RawTypeDescriptorBuilder(const std::string &name);

  /// 析构时将 TypeDescriptor 交给 Registry 注册（实现在 .cpp）。
  ~RawTypeDescriptorBuilder();
  RawTypeDescriptorBuilder(const RawTypeDescriptorBuilder &) = delete;
  RawTypeDescriptorBuilder &operator=(const RawTypeDescriptorBuilder &) =
      delete;
  RawTypeDescriptorBuilder(RawTypeDescriptorBuilder &&) = default;
  RawTypeDescriptorBuilder &operator=(RawTypeDescriptorBuilder &&) = default;

  /// 添加一个成员变量：名称 name，成员指针 var，并加入 member_vars_。
  template <typename C, typename T>
  void AddMemberVar(const std::string &name, T C::*var) {
    MemberVariable mv{var};
    mv.name_ = name;
    desc_->member_vars_.push_back(std::move(mv));
  }

  /// 添加一个成员函数：名称 name，函数指针 func，并加入 member_funcs_。
  template <typename FUNC>
  void AddMemberFunc(const std::string &name, FUNC func) {
    MemberFunction mf{func};
    mf.name_ = name;
    desc_->member_funcs_.push_back(std::move(mf));
  }

 private:
  std::unique_ptr<TypeDescriptor> desc_{nullptr};
};

template <typename T>
class TypeDescriptorBuilder {
 public:
  /// 用类型名 name 创建 builder，内部创建 RawTypeDescriptorBuilder(name)。
  explicit TypeDescriptorBuilder(const std::string &name) : raw_builder_(name) {
  }

  /// 链式添加成员变量并返回 *this。
  template <typename V>
  TypeDescriptorBuilder &AddMemberVar(const std::string &name, V T::*var) {
    raw_builder_.AddMemberVar(name, var);
    return *this;
  }

  /// 链式添加成员函数并返回 *this。
  template <typename FUNC>
  TypeDescriptorBuilder &AddMemberFunc(const std::string &name, FUNC func) {
    raw_builder_.AddMemberFunc(name, func);
    return *this;
  }

 private:
  RawTypeDescriptorBuilder raw_builder_;
};

class Registry {
 public:
  /// 返回单例引用。
  static Registry &instance() {
    static Registry inst;
    return inst;
  }

  /// 按类型名查找并返回 TypeDescriptor 指针（实现在 .cpp）。
  TypeDescriptor *Find(const std::string &name);

  /// 将描述符注册到 type_descs_，以类型名为键（实现在 .cpp）。
  void Register(std::unique_ptr<TypeDescriptor> desc);

  /// 清空所有已注册类型描述（实现在 .cpp）。
  void Clear();

 private:
  std::unordered_map<std::string, std::unique_ptr<TypeDescriptor>> type_descs_;
};

}  // namespace details

/// 创建类型 T 的 TypeDescriptorBuilder，用于链式注册成员变量与成员函数。
template <typename T>
details::TypeDescriptorBuilder<T> AddClass(const std::string &name) {
  details::TypeDescriptorBuilder<T> b{name};
  return b;
}

/// 按类型名查找并返回 TypeDescriptor 的引用。
details::TypeDescriptor &GetByName(const std::string &name);

/// 清空全局类型注册表。
void ClearRegistry();

}  // namespace reflect
