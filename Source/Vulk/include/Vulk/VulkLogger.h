#pragma once

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

class VulkLogger {
  public:
    // Initialize a shared logger instance
    static std::shared_ptr<spdlog::logger> &GetLogger() {
        static std::shared_ptr<spdlog::logger> logger;
        static std::once_flag flag;
        std::call_once(flag, []() {
            // Create a color console sink
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            // Create a file sink
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logfile.log", true);

            // Combine the sinks into a list
            spdlog::sinks_init_list sink_list = {file_sink, console_sink};

            // Create a logger with both sinks
            logger = std::make_shared<spdlog::logger>("BuildTool", sink_list.begin(), sink_list.end());

            // Register it as the default logger
            spdlog::register_logger(logger);
            spdlog::set_default_logger(logger);

            // Set the logger's level (e.g., info, warning, error)
            logger->set_level(spdlog::level::info);
        });
        return logger;
    }
};

#define VULK_TRACE(...) VulkLogger::GetLogger()->trace(__VA_ARGS__)
#define VULK_DEBUG(...) VulkLogger::GetLogger()->debug(__VA_ARGS__)
#define VULK_LOG(...) VulkLogger::GetLogger()->info(__VA_ARGS__)
#define VULK_WARN(...) VulkLogger::GetLogger()->warn(__VA_ARGS__)
#define VULK_ERR(...) VulkLogger::GetLogger()->error(__VA_ARGS__)
#define VULK_CRIT(...) VulkLogger::GetLogger()->critical(__VA_ARGS__)
