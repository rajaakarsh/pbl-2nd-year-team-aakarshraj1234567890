#include "ActionManager.hpp"
#include "Image.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void printUsage(const std::string& prog) {
    std::cerr
        << "Usage:\n"
        << "  Batch Queue Mode:   " << prog
            << " -q <flags> <input.bmp> <output.bmp>\n"
        << "  Single Filter Mode: " << prog
            << " -g|-s|-r|-b|-t|-f|-h <input.bmp> <output.bmp>\n"
        << "\n"
        << "Filter flags:\n"
        << "  g  Grayscale          s  Sepia\n"
        << "  r  Reflect            b  Blur (3x3 box)\n"
        << "  t  Quadtree compress  f  Graph BFS segment\n"
        << "  h  Print histogram    u  Undo last filter  (batch only)\n"
        << "\n"
        << "Examples:\n"
        << "  " << prog << " -q gsrb  sample.bmp output.bmp\n"
        << "  " << prog << " -g       sample.bmp output.bmp\n";
}

} 

int main(int argc, char* argv[]) {
  
    if (argc != 4 && argc != 5) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    try {
        ActionManager manager;
        std::string inputFile;
        std::string outputFile;

        if (argc == 5) {
           
            const std::string mode = argv[1];
            if (mode != "-q") {
                std::cerr << "Error: expected '-q' for batch mode, got '"
                          << mode << "'\n";
                printUsage(argv[0]);
                return EXIT_FAILURE;
            }
            manager.enqueueFilters(argv[2]);
            inputFile  = argv[3];
            outputFile = argv[4];

        } else {
           
            const std::string flag = argv[1];
            if (flag.size() != 2 || flag[0] != '-') {
                std::cerr << "Error: invalid flag '" << flag << "'\n";
                printUsage(argv[0]);
                return EXIT_FAILURE;
            }

            const char filterChar = flag[1];
            const std::string validFlags = "gsrbtfh";
            if (validFlags.find(filterChar) == std::string::npos) {
                std::cerr << "Error: unknown flag '" << flag << "'\n";
                printUsage(argv[0]);
                return EXIT_FAILURE;
            }

            manager.enqueueFilters(std::string(1, filterChar));
            inputFile  = argv[2];
            outputFile = argv[3];
        }

      
        Image image;
        image.read(inputFile);

        manager.executePipeline(image);

        image.write(outputFile);

        std::cout << "Successfully wrote " << outputFile
                  << "  (" << manager.historyCount()
                  << " undo snapshot(s) stored)\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
