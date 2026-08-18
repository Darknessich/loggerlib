#include <framework/TestFramework.hpp>

#include <cstdlib>
#include <exception>
#include <iostream>

namespace tf {
    namespace {
        std::string fullName(const STestCase& testCase) {
            return std::string{testCase.suite} + '.' + std::string{testCase.name};
        }

        bool matches(const STestCase& testCase, const SOptions& options, const std::string& name) {
            if (!options.suite.empty() && options.suite != testCase.suite) return false;
            if (!options.filter.empty() && name.find(options.filter) == std::string::npos)
                return false;
            return true;
        }
    } // namespace

    bool Context::check(bool condition, std::string_view expression, const char* file, int line) {
        if (!condition) reportFailure(expression, file, line);
        return condition;
    }

    void Context::reportFailure(std::string_view what, const char* file, int line) {
        ++m_failures;
        std::cout << "         " << file << ':' << line << ": " << what << '\n';
    }

    Registry& Registry::instance() {
        static Registry registry;
        return registry;
    }

    void Registry::add(const STestCase& testCase) {
        m_cases.push_back(testCase);
    }

    void printUsage(const char* program, std::ostream& stream) {
        stream << "Usage: " << program
               << " [--help|-h] [--suite <name>] [--filter <substring>] [--list]\n";
    }

    bool parseOptions(int argc, char** argv, SOptions& out) {
        for (int i = 1; i < argc; ++i) {
            const std::string_view argument = argv[i];

            if (argument == "--help" || argument == "-h") {
                out.helpRequested = true;
            } else if (argument == "--list") {
                out.listOnly = true;
            } else if (argument == "--suite" || argument == "--filter") {
                if (i + 1 >= argc) {
                    std::cerr << "missing value for " << argument << '\n';
                    return false;
                }
                ((argument == "--suite") ? out.suite : out.filter) = argv[++i];
            } else {
                std::cerr << "unknown argument: " << argument << '\n';
                return false;
            }
        }
        return true;
    }

    int run(const SOptions& options) {
        std::size_t executed = 0;
        std::size_t failed = 0;

        for (const auto& testCase : Registry::instance().cases()) {
            const std::string name = fullName(testCase);
            if (!matches(testCase, options, name)) continue;

            if (options.listOnly) {
                std::cout << name << '\n';
                ++executed;
                continue;
            }

            std::cout << "[ RUN  ] " << name << '\n';

            Context context;
            try {
                testCase.function(context);
            } catch (const std::exception& error) {
                context.reportFailure(
                    std::string{"unexpected exception: "} + error.what(), __FILE__, __LINE__
                );
            } catch (...) {
                context.reportFailure("unexpected non-standard exception", __FILE__, __LINE__);
            }

            ++executed;
            if (context.failureCount() == 0) {
                std::cout << "[  OK  ] " << name << '\n';
            } else {
                ++failed;
                std::cout << "[ FAIL ] " << name << " (" << context.failureCount()
                          << " failed checks)\n";
            }
        }

        if (executed == 0) {
            std::cerr << "no tests matched the given filter\n";
            return EXIT_FAILURE;
        }

        if (options.listOnly) return EXIT_SUCCESS;

        std::cout << '\n'
                  << (executed - failed) << " passed, " << failed << " failed, " << executed
                  << " total\n";
        return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }
} // namespace tf
