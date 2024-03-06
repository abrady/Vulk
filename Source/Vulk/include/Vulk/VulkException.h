#pragma once

#include <iostream>
#include <stacktrace>
#include <stdexcept>

class VulkException : public std::exception {
    void init() {
        msg += "\nStacktrace:\n";
        for (const auto &frame : st) {
            msg += std::format("  {}\n", std::to_string(frame));
        }
    }

  public:
    VulkException(const char *message) : msg(message), st(std::stacktrace::current()) {
        init();
    }

    VulkException(std::string message) : msg(message), st(std::stacktrace::current()) {
        init();
    }

    const char *what() const noexcept override {
        return msg.c_str();
    }

    const std::stacktrace &get_stacktrace() const {
        return st;
    }

  private:
    std::string msg;
    std::stacktrace st;
};

#define VULK_THROW(msg) throw VulkException(msg)