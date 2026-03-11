/**
 * @file TypeDescriptorBuilder.cpp
 * @brief RawTypeDescriptorBuilder 构造/析构及 Registry 依赖实现
 */
#include "TypeDescriptorBuilder.h"
#include "Registry.h"

namespace DE {
    namespace Reflect {
        /// 创建 TypeDescriptor 并设置其名称。
        RawTypeDescriptorBuilder::RawTypeDescriptorBuilder(const std::string &name)
            : desc_(std::make_unique<TypeDescriptor>()) {
            desc_->name_ = name;
        }

        /// 析构时将描述符移交给 Registry 注册。
        RawTypeDescriptorBuilder::~RawTypeDescriptorBuilder() {
            Registry::instance().Register(std::move(desc_));
        }
    } // namespace Reflect
} // namespace DE
