//
// Created by ImSetal on 2026/3/6.
//

#ifndef DEARENGINE_REFLECTIONMACROS_H
#define DEARENGINE_REFLECTIONMACROS_H

#define REFLECT_CLASS(ClassName) \
    static TypeInfo* GetStaticTypeInfo() { \
        static TypeInfo* info = []() { \
            auto* ti = new TypeInfo{#ClassName, {}};
#define REFLECT_FIELD(ClassName, field) \
            ti->AddField(#field, offsetof(ClassName, field), typeid(decltype(((ClassName*)0)->field)).name(), sizeof(decltype(((ClassName*)0)->field)));
#define REFLECT_END() \
            TypeRegistry::Register(ti); return ti; }(); return info; } \

#endif //DEARENGINE_REFLECTIONMACROS_H