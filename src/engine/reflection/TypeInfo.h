//
// Created by ImSetal on 2026/3/5.
//

#ifndef DEARENGINE_TYPEINFO_H
#define DEARENGINE_TYPEINFO_H
#include <cstring>
#include <string>
#include <vector>

namespace DE {

    struct FieldInfo {
        std::string name;
        std::string type_name;
        size_t offset;
        size_t size;  // 字段占用的字节数，用于 SetPtr 的 memcpy，支持任意类型
        void* GetPtr(void* obj) const {
            return (char*)obj + offset;
        }
        void SetPtr(void* obj, const void* value) const {
            if (value && size > 0)
                std::memcpy((char*)obj + offset, value, size);
        }
    };

    struct TypeInfo {
        std::string name;
        std::vector<FieldInfo> fields;
        void AddField(const std::string& n, size_t off, const std::string& type, size_t field_size) {
            fields.push_back({n, type, off, field_size});
        }
    };

}

#endif //DEARENGINE_TYPEINFO_H