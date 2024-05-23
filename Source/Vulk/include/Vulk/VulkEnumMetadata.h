#pragma once

#include "VulkResourceMetadata_generated.h"
#include "VulkShaderEnums_generated.h"
#include <mutex>         // for std::once_flag
#include <unordered_map> // for std::unordered_map

template <typename T> struct EnumNameGetter;

template <typename T, std::size_t N> constexpr std::size_t flatbufArraySize(const T (&)[N]) noexcept {
    return N;
}

template <> struct EnumNameGetter<VulkShaderLocation> {
    static const char *const getName(VulkShaderLocation e) {
        return EnumNameVulkShaderLocation(e);
    }
    static std::vector<VulkShaderLocation> getValues() {
        const auto &vals = EnumValuesVulkShaderLocation();
        int n = sizeof(vals) / sizeof(vals[0]);
        return std::vector<VulkShaderLocation>(vals, vals + n);
    }
};

template <> struct EnumNameGetter<VulkShaderBinding> {
    static const char *const getName(VulkShaderBinding e) {
        return EnumNameVulkShaderBinding(e);
    }
    static std::vector<VulkShaderBinding> getValues() {
        const auto &vals = EnumValuesVulkShaderBinding();
        int n = sizeof(vals) / sizeof(vals[0]);
        return std::vector<VulkShaderBinding>(vals, vals + n);
    }
};

template <> struct EnumNameGetter<VulkShaderUBOBinding> {
    static const char *const getName(VulkShaderUBOBinding e) {
        return EnumNameVulkShaderUBOBinding(e);
    }
    static std::vector<VulkShaderUBOBinding> getValues() {
        const auto &vals = EnumValuesVulkShaderUBOBinding();
        int n = sizeof(vals) / sizeof(vals[0]);
        return std::vector<VulkShaderUBOBinding>(vals, vals + n);
    }
};

template <> struct EnumNameGetter<VulkShaderDebugUBO> {
    static const char *const getName(VulkShaderDebugUBO e) {
        return EnumNameVulkShaderDebugUBO(e);
    }
    static std::vector<VulkShaderDebugUBO> getValues() {
        const auto &vals = EnumValuesVulkShaderDebugUBO();
        int n = sizeof(vals) / sizeof(vals[0]);
        return std::vector<VulkShaderDebugUBO>(vals, vals + n);
    }
};

template <> struct EnumNameGetter<VulkShaderSSBOBinding> {
    static const char *const getName(VulkShaderSSBOBinding e) {
        return EnumNameVulkShaderSSBOBinding(e);
    }
    static std::vector<VulkShaderSSBOBinding> getValues() {
        const auto &vals = EnumValuesVulkShaderSSBOBinding();
        int n = sizeof(vals) / sizeof(vals[0]);
        return std::vector<VulkShaderSSBOBinding>(vals, vals + n);
    }
};

template <> struct EnumNameGetter<VulkShaderTextureBinding> {
    static const char *const getName(VulkShaderTextureBinding e) {
        return EnumNameVulkShaderTextureBinding(e);
    }
    static std::vector<VulkShaderTextureBinding> getValues() {
        const auto &vals = EnumValuesVulkShaderTextureBinding();
        int n = sizeof(vals) / sizeof(vals[0]);
        return std::vector<VulkShaderTextureBinding>(vals, vals + n);
    }
};

template <> struct EnumNameGetter<VulkPrimitiveTopology> {
    static const char *const getName(VulkPrimitiveTopology e) {
        return EnumNameVulkPrimitiveTopology(e);
    }
    static std::vector<VulkPrimitiveTopology> getValues() {
        const auto &vals = EnumValuesVulkPrimitiveTopology();
        int n = sizeof(vals) / sizeof(vals[0]);
        return std::vector<VulkPrimitiveTopology>(vals, vals + n);
    }
};

template <> struct EnumNameGetter<VulkPolygonMode> {
    static const char *const getName(VulkPolygonMode e) {
        return EnumNameVulkPolygonMode(e);
    }
    static std::vector<VulkPolygonMode> getValues() {
        const auto &vals = EnumValuesVulkPolygonMode();
        int n = sizeof(vals) / sizeof(vals[0]);
        return std::vector<VulkPolygonMode>(vals, vals + n);
    }
};

template <> struct EnumNameGetter<MeshDefType> {
    static const char *const getName(MeshDefType e) {
        return EnumNameMeshDefType(e);
    }
    static std::vector<MeshDefType> getValues() {
        const auto &vals = EnumValuesMeshDefType();
        int n = sizeof(vals) / sizeof(vals[0]);
        return std::vector<MeshDefType>(vals, vals + n);
    }
};

template <> struct EnumNameGetter<GeoMeshDefType> {
    static const char *const getName(GeoMeshDefType e) {
        return EnumNameGeoMeshDefType(e);
    }
    static std::vector<GeoMeshDefType> getValues() {
        const auto &vals = EnumValuesGeoMeshDefType();
        int n = sizeof(vals) / sizeof(vals[0]);
        return std::vector<GeoMeshDefType>(vals, vals + n);
    }
};

template <> struct EnumNameGetter<VulkCompareOp> {
    static const char *const getName(VulkCompareOp e) {
        return EnumNameVulkCompareOp(e);
    }
    static std::vector<VulkCompareOp> getValues() {
        const auto &vals = EnumValuesVulkCompareOp();
        int n = sizeof(vals) / sizeof(vals[0]);
        return std::vector<VulkCompareOp>(vals, vals + n);
    }
};

template <> struct EnumNameGetter<VulkCullModeFlags> {
    static const char *const getName(VulkCullModeFlags e) {
        return EnumNameVulkCullModeFlags(e);
    }
    static std::vector<VulkCullModeFlags> getValues() {
        const auto &vals = EnumValuesVulkCullModeFlags();
        int n = sizeof(vals) / sizeof(vals[0]);
        return std::vector<VulkCullModeFlags>(vals, vals + n);
    }
};

// the strings are packed in the array starting from the min value
template <typename EnumType> struct EnumLookup {
    static EnumType getEnumFromStr(std::string value) {
        static std::unordered_map<std::string, EnumType> enumMap;
        static std::once_flag flag;
        std::call_once(flag, [&]() {
            const auto &vals = EnumNameGetter<EnumType>::getValues();
            for (auto val : vals) {
                std::string name = EnumNameGetter<EnumType>::getName(val);
                enumMap[name] = val;
            }
        });
        return enumMap.at(value);
    }
    static std::string getStrFromEnum(EnumType type) {
        return EnumNameGetter<EnumType>::getName(type);
    }
};