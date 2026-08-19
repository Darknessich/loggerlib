#include <utils/TempFile.hpp>

#include <logger/LogLevel.hpp>
#include <logger/LogRecord.hpp>
#include <logger/LoggerFactory.hpp>

#include <cstdlib>
#include <string>
#include <system_error>

namespace {
    bool writesThroughTheSharedLibrary() {
        const std::string message = "through the shared library";
        const utils::TempFile file{"logger_shared_smoke.log"};

        std::error_code ec;
        const auto logger = Logger::createFileLogger(file.path(), Logger::ELogLevel::Info, ec);
        if (!logger || ec) return false;
        if (!logger->log(Logger::ELogLevel::Warn, message)) return false;

        const auto lines = file.readLines();
        if (lines.size() != 1) return false;

        const auto parsed = Logger::parseRecord(lines.front());
        return parsed && parsed->level == Logger::ELogLevel::Warn && parsed->message == message;
    }
} // namespace

int main() {
    if (Logger::level2string(Logger::ELogLevel::Info) != "INFO") return EXIT_FAILURE;
    if (!Logger::string2level("info")) return EXIT_FAILURE;
    if (!Logger::isValidLevel(Logger::ELogLevel::Info)) return EXIT_FAILURE;
    if (Logger::escapeMessage("a\nb") != "a\\nb") return EXIT_FAILURE;
    if (Logger::unescapeMessage("a\\nb") != "a\nb") return EXIT_FAILURE;
    if (Logger::formatRecord({}).empty()) return EXIT_FAILURE;
    if (Logger::parseRecord("garbage")) return EXIT_FAILURE;

    std::error_code ec;
    if (Logger::createFileLogger("", Logger::ELogLevel::Info, ec)) return EXIT_FAILURE;
    if (!ec) return EXIT_FAILURE;

    if (!writesThroughTheSharedLibrary()) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
