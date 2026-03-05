//
// Created by ImSetal on 2026/3/5.
//

#ifndef DEARENGINE_TYPEINFO_H
#define DEARENGINE_TYPEINFO_H
#include <string>

namespace DE {

    struct FieldInfo {
        std::string name;
        std::string type_name;
        size_t offset;
    };

    struct TypeInfo {
        std::string name;
        std::vector<FieldInfo> fields;
    };

}

#endif //DEARENGINE_TYPEINFO_H