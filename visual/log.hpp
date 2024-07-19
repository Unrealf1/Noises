#pragma once

#include <utility>

#ifdef __EMSCRIPTEN__

#include <format>
#include <emscripten/console.h>

#else

#include <spdlog/spdlog.h>

#endif

#ifdef __EMSCRIPTEN__

namespace internal {
    void log(const char* prefix, const char* fmt, auto&&... args) {
        auto message = std::vformat(fmt, std::make_format_args(args...));
        auto withPrefix = std::format("{}: {}", prefix, message);
        emscripten_console_log(withPrefix.c_str());
    }
}

template<typename... Ts>
void error(const char* fmt, Ts&&... args) { internal::log("[E]", fmt, std::forward<Ts>(args)...); }

template<typename... Ts>
void warn(const char* fmt, Ts&&... args) { internal::log("[W]", fmt, std::forward<Ts>(args)...); }

template<typename... Ts>
void info(const char* fmt, Ts&&... args) { internal::log("[I]", fmt, std::forward<Ts>(args)...); }

#else // no emscripten

#define error(...) spdlog::error(__VA_ARGS__)
#define warn(...) spdlog::warn(__VA_ARGS__)
#define info(...) spdlog::info(__VA_ARGS__)

#endif
