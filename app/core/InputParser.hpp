#pragma once

#include <logger/LogLevel.hpp>

#include <string>
#include <string_view>

namespace App {
    enum class EInputKind { Message, SetLevel, Quit, Help, Empty, Error };

    struct SUserInput {
        EInputKind kind{EInputKind::Empty};
        Logger::ELogLevel level{Logger::ELogLevel::Info};
        std::string message;
        std::string error;
    };

    SUserInput parseUserInput(std::string_view line, Logger::ELogLevel defaultLevel);
} // namespace App
