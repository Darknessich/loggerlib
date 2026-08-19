#include <framework/TestFramework.hpp>

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    const char* const program = argc > 0 ? argv[0] : "logger_tests";

    tf::SOptions options;
    if (!tf::parseOptions(argc, argv, options)) {
        tf::printUsage(program, std::cerr);
        return EXIT_FAILURE;
    }

    if (options.helpRequested) {
        tf::printUsage(program, std::cout);
        return EXIT_SUCCESS;
    }

    return tf::run(options);
}
