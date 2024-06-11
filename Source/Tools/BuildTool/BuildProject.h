#pragma once
#include <filesystem>

extern void glslShaderEnumsGenerator(std::filesystem::path outFile, bool verbose);
extern void buildProjectDef(const std::filesystem::path project_file_path, std::filesystem::path buildDir);
