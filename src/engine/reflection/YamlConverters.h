//
// Created by ImSetal on 2026/3/9.
//

#ifndef DEARENGINE_YAMLCONVERTERS_H
#define DEARENGINE_YAMLCONVERTERS_H
#include <glm/vec3.hpp>

#include "../core/component/TransformSpace.h"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/emitter.h"
#include "yaml-cpp/emittermanip.h"

namespace YAML {

    // glm::vec3
    template<>
    struct convert<glm::vec3> {
        static Node encode(const glm::vec3& rhs) {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            return node;
        }

        static bool decode(const Node& node, glm::vec3& rhs) {
            if (!node.IsSequence() || node.size() != 3) {
                return false;
            }

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            return true;
        }
    };

    // DE::Space
    template<>
    struct convert<DE::TransformSpace> {
        static Node encode(const DE::TransformSpace& rhs) {
            Node node;
            switch (rhs) {
                case DE::ParentSpace: node = "ParentSpace"; break;
                case DE::WorldSpace:  node = "WorldSpace";  break;
                default:              node = "ParentSpace"; break;
            }
            return node;
        }

        static bool decode(const Node& node, DE::TransformSpace& rhs) {
            if (!node.IsScalar()) {
                return false;
            }

            const std::string s = node.as<std::string>();
            if (s == "ParentSpace") {
                rhs = DE::ParentSpace;
                return true;
            }
            if (s == "WorldSpace") {
                rhs = DE::WorldSpace;
                return true;
            }
            return false;
        }
    };

    // 自由函数：供 DeReflectionYaml 中 out << instance->*member 使用
    inline Emitter& operator<<(Emitter& out, const glm::vec3& v) {
        out << Flow << BeginSeq << v.x << v.y << v.z << EndSeq;
        return out;
    }

    inline Emitter& operator<<(Emitter& out, DE::TransformSpace s) {
        switch (s) {
            case DE::ParentSpace: out << "ParentSpace"; break;
            case DE::WorldSpace:  out << "WorldSpace";  break;
            default:              out << "ParentSpace"; break;
        }
        return out;
    }

} // namespace YAML


#endif //DEARENGINE_YAMLCONVERTERS_H