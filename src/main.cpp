#include <R.h>
#include <R_ext/Rdynload.h>
#include <Rinternals.h>
#include <Rembedded.h>

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>

#include "parse.h"
#include "typelang.h"

bool startsWith(const char* str, const char* prefix) {
	size_t lenPrefix = std::strlen(prefix);
	size_t lenStr = std::strlen(str);
	if (lenStr < lenPrefix) return false;
	return std::strncmp(str, prefix, lenPrefix) == 0;
}

void runParse(const char* filename) {
	std::cout << "Starting parsing process for file: " << filename << std::endl;

	SEXP roots = tokenizeRSource(filename);
	if (!Rf_inherits(roots, "data.frame")) {
		std::cerr << "Error before AST generation: result is not a data.frame." << std::endl;
		Rf_endEmbeddedR(0);
		return;
	}

	std::cout << "Tokenization successful. Generating AST..." << std::endl;

	std::vector<ParseNode*> astNodes = generateAST(roots);
	std::cout << "Root AST Nodes Generated: " << astNodes.size() << std::endl;

	TypeParser::functionContracts.clear();

	FILE* file = std::fopen(filename, "r");
	if (!file) {
		std::cerr << "Failed to open file: " << filename << "\n";
		Rf_endEmbeddedR(0);
		return;
	}

	const int maxLineLength = 1024;
	char buffer[maxLineLength];
	Type* contractType = nullptr;

	std::cout << "Reading the file line by line for contract annotations..." << std::endl;

	while (std::fgets(buffer, maxLineLength, file)) {
		size_t len = std::strlen(buffer);
		if (len > 0 && buffer[len - 1] == '\n') {
			buffer[len - 1] = '\0';
		}

		std::cout << "Processing line: " << buffer << std::endl;

		if (startsWith(buffer, "# @contract")) {
    const char* contractText = buffer + 11;
    while (*contractText && std::isspace(*contractText)) ++contractText;

    std::cout << "Raw Contract Line: " << contractText << std::endl;

    std::string line(contractText);

    // Split off function name
    std::istringstream iss(line);
    std::string functionName;
    iss >> functionName; // Read 'f' or 'g'

    std::string typeExpr;
    std::getline(iss, typeExpr); // Remaining text

    // Trim leading whitespace from typeExpr
    size_t start = typeExpr.find_first_not_of(" \t");
    if (start != std::string::npos) {
        typeExpr = typeExpr.substr(start);
    }

    std::cout << "Function Name: " << functionName << std::endl;
    std::cout << "Type Expression: " << typeExpr << std::endl;

    try {
        TypeParser parser(typeExpr);
        FunctionType* funcType = dynamic_cast<FunctionType*>(parser.parseType());
        if (!funcType) {
            std::cerr << "Failed to parse contract for function: " << functionName << std::endl;
            continue;
        }

        TypeParser::addFunctionContract(functionName, FunctionContract{
            .argTypes = funcType->arguments,
            .returnType = funcType->returnType
        });

        std::cout << "Registered contract for function " << functionName << std::endl;
    } catch (const std::runtime_error& e) {
        std::cerr << "Failed to parse contract: " << e.what() << std::endl;
    }
}


	}

	std::cout << "for root in nodeVector: " << std::endl;
	int index = 0;
	for (ParseNode*& root : astNodes) {
		std::cout << "Root Node #" << index << ": " << root->text << std::endl;
		index++;
	}
	std::cout << index << " roots in nodeVector." << std::endl;

	std::cout << "for node in flatAST: " << std::endl;
	std::vector<ParseNode*> flatAST = flattenAST(astNodes);
	index = 0;
	for (ParseNode*& node : flatAST) {
		std::cout << "Flat AST Node #" << index << ": " << node->text << std::endl;
		index++;
	}
	std::cout << index << " nodes in flatAST." << std::endl;

	std::cout << "Verifying function calls against contracts..." << std::endl;
	for (ParseNode* node : flatAST) {
		if (startsWith(node->text.c_str(), "exampleFunction")) {
			std::vector<Type*> actualArgs = {new ScalarType("int"), new ScalarType("double")};
			TypeParser::verifyFunctionCall("exampleFunction", actualArgs);
		}
	}

	std::fclose(file);
	std::cout << "File parsing and AST processing complete." << std::endl;
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
