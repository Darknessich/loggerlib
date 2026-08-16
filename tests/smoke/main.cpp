#include <logger/LogLevel.hpp>
#include <logger/LogRecord.hpp>

#include <cstdlib>

int main() {
    if (Logger::level2string(Logger::ELogLevel::INFO) != "INFO") return EXIT_FAILURE;
    if (!Logger::string2level("info")) return EXIT_FAILURE;
    if (Logger::escapeMessage("a\nb") != "a\\nb") return EXIT_FAILURE;
    if (Logger::unescapeMessage("a\\nb") != "a\nb") return EXIT_FAILURE;
    if (Logger::formatRecord({}).empty()) return EXIT_FAILURE;
    if (Logger::parseRecord("garbage")) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
