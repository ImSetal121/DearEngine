//
// Created by ImSetal on 2026/3/9.
//

#ifndef DEARENGINE_DEREFLECTIONYAML_H
#define DEARENGINE_DEREFLECTIONYAML_H

#include <functional>
#include <fstream>

#include "yaml-cpp/emitter.h"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/parse.h"

namespace DE {

    // =========================
    // 反射基础结构
    // =========================

    struct FieldInfo {
        const char* name = nullptr;
        std::function<void(YAML::Emitter&, const void*)> serialize;
        std::function<void(const YAML::Node&, void*)> deserialize;
    };

    struct TypeInfo {
        const char* name = nullptr;
        std::vector<FieldInfo> fields;
    };

    // =========================
    // 字段工厂
    // =========================

    template<typename Class, typename Member>
    FieldInfo MakeField(const char* field_name, Member Class::*member) {
        FieldInfo info;
        info.name = field_name;

        info.serialize = [field_name, member](YAML::Emitter& out, const void* obj) {
            const auto* instance = static_cast<const Class*>(obj);
            out << YAML::Key << field_name;
            out << YAML::Value << instance->*member;
        };

        info.deserialize = [field_name, member](const YAML::Node& node, void* obj) {
            auto* instance = static_cast<Class*>(obj);
            const YAML::Node value_node = node[field_name];
            if (!value_node || value_node.IsNull()) {
                return;
            }
            instance->*member = value_node.as<Member>();
        };

        return info;
    }

    // 支持把字段列表初始化成 vector
    inline void CollectFields(std::vector<FieldInfo>&) {}

    template<typename First, typename... Rest>
    void CollectFields(std::vector<FieldInfo>& out, First&& first, Rest&&... rest) {
        out.emplace_back(std::forward<First>(first));
        CollectFields(out, std::forward<Rest>(rest)...);
    }

    template<typename... Fields>
    std::vector<FieldInfo> BuildFields(Fields&&... fields) {
        std::vector<FieldInfo> out;
        out.reserve(sizeof...(Fields));
        CollectFields(out, std::forward<Fields>(fields)...);
        return out;
    }

    // =========================
    // YAML 自动序列化
    // =========================

    namespace yaml {

    template<typename T>
    std::string Serialize(const T& obj) {
        YAML::Emitter out;
        out << YAML::BeginMap;

        const TypeInfo& type = T::StaticType();
        for (const auto& field : type.fields) {
            field.serialize(out, &obj);
        }

        out << YAML::EndMap;
        return {out.c_str()};
    }

    template<typename T>
    void Deserialize(const YAML::Node& node, T& obj) {
        const TypeInfo& type = T::StaticType();
        for (const auto& field : type.fields) {
            field.deserialize(node, &obj);
        }
    }

    template<typename T>
    void Deserialize(std::string_view yaml_text, T& obj) {
        YAML::Node node = YAML::Load(std::string(yaml_text));
        Deserialize(node, obj);
    }

    template<typename T>
    bool SaveToFile(const std::string& path, const T& obj) {
        try {
            YAML::Emitter out;
            out << YAML::BeginMap;

            const TypeInfo& type = T::StaticType();
            out << YAML::Key << type.name;
            out << YAML::BeginMap;
            for (const auto& field : type.fields) {
                field.serialize(out, &obj);
            }
            out << YAML::EndMap;

            out << YAML::EndMap;

            std::ofstream fout(path);
            if (!fout.is_open()) {
                return false;
            }
            fout << out.c_str();
            return true;
        } catch (...) {
            return false;
        }
    }

    template<typename T>
    bool LoadFromFile(const std::string& path, T& obj) {
        try {
            YAML::Node root = YAML::LoadFile(path);
            const TypeInfo& type = T::StaticType();

            YAML::Node node = root[type.name];
            if (!node) {
                // 兼容“裸 map”格式
                node = root;
            }

            Deserialize(node, obj);
            return true;
        } catch (...) {
            return false;
        }
    }

    } // namespace yaml
    } // namespace de

    // =========================
    // 宏接口
    // =========================

    #define DE_FIELD(name) ::DE::MakeField<Self>(#name, &Self::name)

    #define DE_REFLECT(Type, ...)                                  \
    public:                                                        \
        using Self = Type;                                         \
        static const ::DE::TypeInfo& StaticType() {                \
            static const ::DE::TypeInfo type_info = {              \
                #Type,                                             \
                ::DE::BuildFields(__VA_ARGS__)                     \
            };                                                     \
            return type_info;                                      \
        }

#endif //DEARENGINE_DEREFLECTIONYAML_H