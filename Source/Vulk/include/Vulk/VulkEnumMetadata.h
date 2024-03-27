#pragma once

#include "VulkShaderEnums_generated.h"
#include "VulkResourceMetadata_generated.h"
#include <unordered_map> // for std::unordered_map
#include <mutex> // for std::once_flag

template <typename T> struct EnumNameGetter;

template <> struct EnumNameGetter<VulkShaderLocation> {
    static const char *const *getNames() {
        return EnumNamesVulkShaderLocation();
    }
    static VulkShaderLocation getMin() {
        return VulkShaderLocation_MIN;
    }
};

template <> struct EnumNameGetter<VulkShaderBindings> {
    static const char *const *getNames() {
        return EnumNamesVulkShaderBindings();
    }
    static VulkShaderBindings getMin() {
        return VulkShaderBindings_MIN;
    }
};

template <> struct EnumNameGetter<VulkShaderUBOBinding> {
    static const char *const *getNames() {
        return EnumNamesVulkShaderUBOBinding();
    }
    static VulkShaderUBOBinding getMin() {
        return VulkShaderUBOBinding_MIN;
    }
};

template <> struct EnumNameGetter<VulkShaderDebugUBO> {
    static const char *const *getNames() {
        return EnumNamesVulkShaderDebugUBO();
    }
    static VulkShaderDebugUBO getMin() {
        return VulkShaderDebugUBO_MIN;
    }
};

template <> struct EnumNameGetter<VulkShaderSSBOBinding> {
    static const char *const *getNames() {
        return EnumNamesVulkShaderSSBOBinding();
    }
    static VulkShaderSSBOBinding getMin() {
        return VulkShaderSSBOBinding_MIN;
    }
};

template <> struct EnumNameGetter<VulkShaderTextureBinding> {
    static const char *const *getNames() {
        return EnumNamesVulkShaderTextureBinding();
    }
    static VulkShaderTextureBinding getMin() {
        return VulkShaderTextureBinding_MIN;
    }
};

template <> struct EnumNameGetter<VulkPrimitiveTopology> {
    static const char *const *getNames() {
        return EnumNamesVulkPrimitiveTopology();
    }
    static VulkPrimitiveTopology getMin() {
        return VulkPrimitiveTopology_MIN;
    }
};

template <> struct EnumNameGetter<MeshDefType> {
    static const char *const *getNames() {
        return EnumNamesMeshDefType();
    }
    static MeshDefType getMin() {
        return MeshDefType_MIN;
    }
};

template <> struct EnumNameGetter<GeoMeshDefType> {
    static const char *const *getNames() {
        return EnumNamesGeoMeshDefType();
    }
    static GeoMeshDefType getMin() {
        return GeoMeshDefType_MIN;
    }
};

template <> struct EnumNameGetter<VulkCompareOp> {
    static const char *const *getNames() {
        return EnumNamesVulkCompareOp();
    }
    static VulkCompareOp getMin() {
        return VulkCompareOp_MIN;
    }
};

template <> struct EnumNameGetter<VulkCullModeFlags> {
    static const char *const *getNames() {
        return EnumNamesVulkCullModeFlags();
    }
    static VulkCullModeFlags getMin() {
        return VulkCullModeFlags_MIN;
    }
};

// the strings are packed in the array starting from the min value
template <typename EnumType> struct EnumLookup {
    static EnumType getEnumFromStr(std::string value) {
        static std::unordered_map<std::string, EnumType> enumMap;
        static std::once_flag flag;
        std::call_once(flag, [&]() {
            const char *const *vals = EnumNameGetter<EnumType>::getNames();
            EnumType min = EnumNameGetter<EnumType>::getMin();
            for (int i = 0; vals[i]; i++) {
                EnumType enumValue = static_cast<EnumType>(min + i);
                if (*vals[i])
                    enumMap[vals[i]] = enumValue;
            }
        });
        return enumMap.at(value);
    }
    static std::string getStrFromEnum(EnumType type) {
        return EnumNameGetter<EnumType>::getNames()[static_cast<int>(type) - EnumNameGetter<EnumType>::getMin()];
    }
};