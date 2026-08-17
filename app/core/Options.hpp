#pragma once

#include <logger/LogLevel.hpp>

#include <iosfwd>
#include <string>

namespace App {
    enum class EOptionsKind { Run, Help, Error };

    struct SOptions {
        EOptionsKind kind{EOptionsKind::Error};
        std::string path;
        Logger::ELogLevel level{Logger::ELogLevel::Info};
        std::string error;
    };

    SOptions parseOptions(int argc, const char* const* argv);
    void printUsage(const char* program, std::ostream& stream);
    void printLevels(std::ostream& stream);
} // namespace App
