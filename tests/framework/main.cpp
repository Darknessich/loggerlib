#include <framework/TestFramework.hpp>

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    tf::SOptions options;
    if (!tf::parseOptions(argc, argv, options)) {
        tf::printUsage(argv[0], std::cerr);
        return EXIT_FAILURE;
    }

    if (options.helpRequested) {
        tf::printUsage(argv[0], std::cout);
        return EXIT_SUCCESS;
    }

    return tf::run(options);
}
