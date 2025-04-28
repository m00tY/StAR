#include <Rinternals.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>

#include "sourceparser.h"
#include "contractparser.h"

bool startsWith(const char* str, const char* prefix) {
	size_t len_prefix = std::strlen(prefix);
    size_t len_str = std::strlen(str);
    if (len_str < len_prefix) return false;
    return std::strncmp(str, prefix, len_prefix) == 0;
}


void runParse(const char* filename) {
	SEXP roots = tokenizeRSource(filename);

	if (!Rf_inherits(roots, "data.frame")) {
		std::cerr << "Error before AST generation: result is not a data.frame." << std::endl;
		return;
	}

	std::vector<ParseNode*> astNodes = generateAST(roots);
	std::cout << "Root AST Nodes Generated: " << astNodes.size() << std::endl;

	std::vector<ParseNode*> commentNodes;

	for (ParseNode *& node: astNodes) {
		debugAST(node);
	}



	FILE* file = std::fopen(filename, "r");

    if (!file) {
        std::cerr << "Failed to open file: " << filename << "\n";
    }

    const int MaxLineLength = 1024;
    char buffer[MaxLineLength];

    while (std::fgets(buffer, MaxLineLength, file)) {
        // Remove trailing newline if it exists
        size_t len = std::strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        if (!startsWith(buffer, "# @contract")) {
            continue;  // Skip non-contract lines
        }

        // Strip "# @contract " prefix (length = 11)
        const char* contract_text = buffer + 11;
        while (*contract_text && std::isspace(*contract_text)) {
            contract_text++;
        }

        try {
            ContractLexer lexer(contract_text);
            ContractParser parser(lexer);
            Contract contract = parser.parseContract();

            std::cout << "Parsed contract:\n";
            std::cout << "Arguments:\n";
            for (const auto& arg : contract.argumentTypes) {
                std::cout << "  - " << arg.name << "\n";
            }
            std::cout << "Return Type:\n";
            std::cout << "  - " << contract.returnType.name << "\n\n";
        } catch (const std::exception& ex) {
            std::cerr << "Parse error: " << ex.what() << "\n\n";
        }
    }

    std::fclose(file);

	
}

bool fileExists(const char* path) {
	struct stat buffer;
	return stat(path, &buffer) == 0;
}


int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " -p <filename>" << std::endl;
		return 1;
	}

	const char* flag = argv[1];
	const char* filename = argv[2];

	if (strcmp(flag, "-p") != 0) {
		std::cerr << "Error: Unknown flag '" << flag << "'" << std::endl;
		return 1;
	}

	if (!fileExists(filename)) {
		std::cerr << "Error: File not found: " << filename << std::endl;
		return 1;
	}

	if (std::getenv("R_HOME") == nullptr) {
		setenv("R_HOME", "/usr/lib64/R", 1);
	}

	runParse(filename);
	return 0;
}


