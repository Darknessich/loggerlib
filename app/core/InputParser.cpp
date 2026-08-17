#include "InputParser.hpp"

#include <utility>

namespace App {
    namespace {
        constexpr std::string_view kSpaces = " \t";

        std::string_view trim(std::string_view line) {
            const auto first = line.find_first_not_of(kSpaces);
            if (first == std::string_view::npos) return {};
            return line.substr(first, line.find_last_not_of(kSpaces) - first + 1);
        }

        std::pair<std::string_view, std::string_view> splitFirstWord(std::string_view line) {
            const auto end = line.find_first_of(kSpaces);
            return end == std::string_view::npos
                ? std::pair{line, std::string_view{}}
                : std::pair{line.substr(0, end), line.substr(end)};
        }

        SUserInput parseCommand(std::string_view line, Logger::ELogLevel defaultLevel) {
            const auto [name, tail] = splitFirstWord(line);

            if (name == "/quit") return {EInputKind::Quit, defaultLevel, {}, {}};
            if (name == "/help") return {EInputKind::Help, defaultLevel, {}, {}};
            if (name != "/level")
                return {
                    EInputKind::Error, defaultLevel, {},
                    "unknown command: " + std::string{name} + " (type /help)"
                };

            const std::string_view argument = trim(tail);
            if (argument.empty())
                return {EInputKind::Error, defaultLevel, {}, "usage: /level <name>"};

            const auto level = Logger::string2level(argument);
            if (!level)
                return {
                    EInputKind::Error, defaultLevel, {},
                    "unknown level: " + std::string{argument}
                };

            return {EInputKind::SetLevel, *level, {}, {}};
        }
    } // namespace

    SUserInput parseUserInput(std::string_view line, Logger::ELogLevel defaultLevel) {
        line = trim(line);
        if (line.empty()) return {EInputKind::Empty, defaultLevel, {}, {}};

        if (line.front() == '/')
            return line.size() > 1 && line[1] == '/'
                ? SUserInput{EInputKind::Message, defaultLevel, std::string{line.substr(1)}, {}}
                : parseCommand(line, defaultLevel);

        const auto [word, tail] = splitFirstWord(line);
        const auto level = Logger::string2level(word);
        if (!level) return {EInputKind::Message, defaultLevel, std::string{line}, {}};

        const std::string_view message = trim(tail);
        return message.empty()
            ? SUserInput{EInputKind::Error, defaultLevel, {}, "message text is missing"}
            : SUserInput{EInputKind::Message, *level, std::string{message}, {}};
    }
} // namespace App
