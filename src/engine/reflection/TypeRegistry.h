//
// Created by ImSetal on 2026/3/6.
//

#ifndef DEARENGINE_TYPEREGISTRY_H
#define DEARENGINE_TYPEREGISTRY_H
#include <unordered_map>

#include "TypeInfo.h"

namespace DE {
    class TypeRegistry {
    public:
        template<typename T>
        static TypeInfo* Get() { return T::GetStaticTypeInfo(); } // 获取类型信息
        static TypeInfo* GetTypeByName(const char* name) { // 按名字查找
            auto it = GetTypes().find(name);
            return it != GetTypes().end() ? it->second : nullptr;
        }
        static void Register(TypeInfo* info) { // 注册类型
            GetTypes()[info->name] = info;
        }
    private:
        static std::unordered_map<std::string, TypeInfo*>& GetTypes() {
            static std::unordered_map<std::string, TypeInfo*> types;
            return types;
        }
    };
}

#endif //DEARENGINE_TYPEREGISTRY_H