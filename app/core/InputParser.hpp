#pragma once

#include <logger/LogLevel.hpp>

#include <string>
#include <string_view>
#include <variant>

namespace App {
    struct SMessage {
        Logger::ELogLevel level{Logger::ELogLevel::Info};
        std::string text;
    };
    struct SLevelCommand {
        Logger::ELogLevel level{Logger::ELogLevel::Info};
    };
    struct SError {
        std::string text;
    };
    struct SQuitCommand {};
    struct SHelpCommand {};
    struct SEmptyLine {};

    using TUserInput =
        std::variant<SMessage, SLevelCommand, SQuitCommand, SHelpCommand, SEmptyLine, SError>;

    // Expects one line without its terminator
    TUserInput parseUserInput(std::string_view line, Logger::ELogLevel defaultLevel);
} // namespace App
