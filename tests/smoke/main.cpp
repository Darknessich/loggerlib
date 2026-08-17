#include <logger/LogLevel.hpp>
#include <logger/LogRecord.hpp>
#include <logger/LoggerFactory.hpp>

#include <cstdlib>
#include <system_error>

int main() {
    if (Logger::level2string(Logger::ELogLevel::Info) != "INFO") return EXIT_FAILURE;
    if (!Logger::string2level("info")) return EXIT_FAILURE;
    if (Logger::escapeMessage("a\nb") != "a\\nb") return EXIT_FAILURE;
    if (Logger::unescapeMessage("a\\nb") != "a\nb") return EXIT_FAILURE;
    if (Logger::formatRecord({}).empty()) return EXIT_FAILURE;
    if (Logger::parseRecord("garbage")) return EXIT_FAILURE;

    std::error_code ec;
    if (Logger::createFileLogger("", Logger::ELogLevel::Info, ec)) return EXIT_FAILURE;
    if (!ec) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
