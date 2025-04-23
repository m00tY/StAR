#include <iostream>
#include <cstring>
#include <sys/stat.h>
#include "ast.h"

void runParse(char* filename) {
    SEXP roots = tokenizeRSource(filename); 

    if (!Rf_inherits(roots, "data.frame")) {
        std::cerr << "Error before AST generation: result is not a data.frame." << std::endl;
        return;
    }

    std::vector<ParseNode*> astNodes = generateAST(roots);

    for (ParseNode* node : astNodes) {
        if (node) {
            debugAST(node, 0);
        } else {
            std::cerr << "Failed to generate AST node." << std::endl;
        }
    }
}

bool fileExists(const char* path) {
    struct stat buffer;
    return stat(path, &buffer) == 0;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <flag> <filename>" << std::endl;
        return 1;
    }

    const char* flag = argv[1];
    const char* filename = argv[2];

    std::cout << "Arg count: " << argc << std::endl;
    std::cout << "Flag: " << flag << ", File: " << filename << std::endl;

    if (strcmp(flag, "-p") != 0) {
        std::cerr << "Error: Invalid flag." << std::endl;
        return 1;
    }

    if (!fileExists(filename)) {
        std::cerr << "Error: File does not exist: " << filename << std::endl;
        return 1;
    }

    if (std::getenv("R_HOME") == nullptr) {
        setenv("R_HOME", "/usr/lib64/R", 1);  
	}

    runParse(const_cast<char*>(filename));

    return 0;
}
