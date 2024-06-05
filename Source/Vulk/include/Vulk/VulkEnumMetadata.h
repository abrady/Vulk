#pragma once

#include "VulkResourceMetadata_generated.h"
#include "VulkShaderEnums_generated.h"
#include "VulkShaderEnums_types.h"
#include <mutex>         // for std::once_flag
#include <unordered_map> // for std::unordered_map

template <typename T>
struct EnumNameGetter;

template <typename T, std::size_t N>
constexpr std::size_t flatbufArraySize(const T (&)[N]) noexcept {
    return N;
}

// template <>
// struct EnumNameGetter<vulk::VulkShaderLocation::type> {
//     static const char* getName(vulk::VulkShaderLocation::type e) {
//         return EnumNameVulkShaderLocation(e);
//     }
//     static std::vector<vulk::VulkShaderLocation::type> getValues() {
//         const auto& vals = EnumValuesVulkShaderLocation();
//         int n = sizeof(vals) / sizeof(vals[0]);
//         return std::vector<vulk::VulkShaderLocation::type>(vals, vals + n);
//     }
// };

// template <>
// struct EnumNameGetter<vulk::VulkShaderBinding> {
//     static const char* getName(vulk::VulkShaderBinding e) {
//         return EnumNameVulkShaderBinding(e);
//     }
//     static std::vector<vulk::VulkShaderBinding> getValues() {
//         const auto& vals = EnumValuesVulkShaderBinding();
//         int n = sizeof(vals) / sizeof(vals[0]);
//         return std::vector<vulk::VulkShaderBinding>(vals, vals + n);
//     }
// };

// template <>
// struct EnumNameGetter<vulk::VulkShaderDebugUBO> {
//     static const char* getName(vulk::VulkShaderDebugUBO e) {
//         return EnumNameVulkShaderDebugUBO(e);
//     }
//     static std::vector<vulk::VulkShaderDebugUBO> getValues() {
//         const auto& vals = EnumValuesVulkShaderDebugUBO();
//         int n = sizeof(vals) / sizeof(vals[0]);
//         return std::vector<vulk::VulkShaderDebugUBO>(vals, vals + n);
//     }
// };

// template <>
// struct EnumNameGetter<VulkPrimitiveTopology> {
//     static const char* getName(VulkPrimitiveTopology e) {
//         return EnumNameVulkPrimitiveTopology(e);
//     }
//     static std::vector<VulkPrimitiveTopology> getValues() {
//         const auto& vals = EnumValuesVulkPrimitiveTopology();
//         int n = sizeof(vals) / sizeof(vals[0]);
//         return std::vector<VulkPrimitiveTopology>(vals, vals + n);
//     }
// };

// template <>
// struct EnumNameGetter<VulkPolygonMode> {
//     static const char* getName(VulkPolygonMode e) {
//         return EnumNameVulkPolygonMode(e);
//     }
//     static std::vector<VulkPolygonMode> getValues() {
//         const auto& vals = EnumValuesVulkPolygonMode();
//         int n = sizeof(vals) / sizeof(vals[0]);
//         return std::vector<VulkPolygonMode>(vals, vals + n);
//     }
// };

// template <>
// struct EnumNameGetter<vulk::MeshDefType::type> {
//     static const char* getName(vulk::MeshDefType::type e) {
//         return EnumNameMeshDefType(e);
//     }
//     static std::vector<vulk::MeshDefType::type> getValues() {
//         const auto& vals = EnumValuesMeshDefType();
//         int n = sizeof(vals) / sizeof(vals[0]);
//         return std::vector<vulk::MeshDefType::type>(vals, vals + n);
//     }
// };

// template <>
// struct EnumNameGetter<GeoMeshDefType> {
//     static const char* getName(GeoMeshDefType e) {
//         return EnumNameGeoMeshDefType(e);
//     }
//     static std::vector<GeoMeshDefType> getValues() {
//         const auto& vals = EnumValuesGeoMeshDefType();
//         int n = sizeof(vals) / sizeof(vals[0]);
//         return std::vector<GeoMeshDefType>(vals, vals + n);
//     }
// };

// template <>
// struct EnumNameGetter<VulkCompareOp> {
//     static const char* getName(VulkCompareOp e) {
//         return EnumNameVulkCompareOp(e);
//     }
//     static std::vector<VulkCompareOp> getValues() {
//         const auto& vals = EnumValuesVulkCompareOp();
//         int n = sizeof(vals) / sizeof(vals[0]);
//         return std::vector<VulkCompareOp>(vals, vals + n);
//     }
// };

// template <>
// struct EnumNameGetter<VulkCullModeFlags> {
//     static const char* getName(VulkCullModeFlags e) {
//         return EnumNameVulkCullModeFlags(e);
//     }
//     static std::vector<VulkCullModeFlags> getValues() {
//         const auto& vals = EnumValuesVulkCullModeFlags();
//         int n = sizeof(vals) / sizeof(vals[0]);
//         return std::vector<VulkCullModeFlags>(vals, vals + n);
//     }
// };

// template <>
// struct EnumNameGetter<vulk::VulkShaderStage> {
//     static const char* getName(vulk::VulkShaderStage e) {
//         return EnumNameVulkShaderStage(e);
//     }
//     static std::vector<vulk::VulkShaderStage> getValues() {
//         const auto& vals = EnumValuesVulkShaderStage();
//         int n = sizeof(vals) / sizeof(vals[0]);
//         return std::vector<vulk::VulkShaderStage>(vals, vals + n);
//     }
// };

// the strings are packed in the array starting from the min value
// template <typename EnumType>
// struct EnumLookup {
//     static EnumType getEnumFromStr(std::string value) {
//         static std::unordered_map<std::string, EnumType> enumMap;
//         static std::once_flag flag;
//         std::call_once(flag, [&]() {
//             const auto& vals = EnumNameGetter<EnumType>::getValues();
//             for (auto val : vals) {
//                 std::string name = EnumNameGetter<EnumType>::getName(val);
//                 enumMap[name] = val;
//             }
//         });
//         return enumMap.at(value);
//     }
//     static std::string getStrFromEnum(EnumType type) {
//         return EnumNameGetter<EnumType>::getName(type);
//     }
// };