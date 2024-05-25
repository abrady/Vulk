#pragma once

#include <spdlog/spdlog.h>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>

class VulkLogger {
public:
    static std::shared_ptr<spdlog::logger> CreateLogger(std::string name) {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(name + "_logfile.log", 1024 * 1024 * 5, 30, true);
        spdlog::sinks_init_list sink_list = {file_sink, console_sink};
        auto p = std::make_shared<spdlog::logger>(name, sink_list.begin(), sink_list.end());
        auto level = spdlog::get_level();
        p->set_level(level);
        return p;
    }
    // Initialize a shared logger instance
    static std::shared_ptr<spdlog::logger>& GetLogger() {
        static std::shared_ptr<spdlog::logger> logger;
        static std::once_flag flag;
        std::call_once(flag, []() {
            logger = CreateLogger("Vulk");

            // Register it as the default logger
            spdlog::register_logger(logger);
            spdlog::set_default_logger(logger);

            // Set the logger's level (e.g., info, warning, error)
            logger->set_level(spdlog::level::info);
        });
        return logger;
    }
};

#define VULK_SET_LOG_LEVEL(level) VulkLogger::GetLogger()->set_level(level)
#define VULK_SET_TRACE_LOG_LEVEL() VULK_SET_LOG_LEVEL(spdlog::level::trace)
#define VULK_SET_DEBUG_LOG_LEVEL() VULK_SET_LOG_LEVEL(spdlog::level::debug)
#define VULK_SET_INFO_LOG_LEVEL() VULK_SET_LOG_LEVEL(spdlog::level::info)
#define VULK_SET_WARN_LOG_LEVEL() VULK_SET_LOG_LEVEL(spdlog::level::warn)
#define VULK_SET_ERROR_LOG_LEVEL() VULK_SET_LOG_LEVEL(spdlog::level::err)
#define VULK_SET_CRITICAL_LOG_LEVEL() VULK_SET_LOG_LEVEL(spdlog::level::critical)

#define VULK_TRACE(...) VulkLogger::GetLogger()->trace(__VA_ARGS__)
#define VULK_DEBUG(...) VulkLogger::GetLogger()->debug(__VA_ARGS__)
#define VULK_LOG(...) VulkLogger::GetLogger()->info(__VA_ARGS__)
#define VULK_WARN(...) VulkLogger::GetLogger()->warn(__VA_ARGS__)
#define VULK_ERR(...) VulkLogger::GetLogger()->error(__VA_ARGS__)
#define VULK_CRIT(...) VulkLogger::GetLogger()->critical(__VA_ARGS__)
