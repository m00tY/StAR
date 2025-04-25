#include <Rinternals.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>

#include "ast.h"
#include "contractparser.h"

void collectComments(ParseNode* node, std::vector<ParseNode*>& out) {
	if (!node) return;
	if (node->token == "COMMENT") {
		out.push_back(node);
	}
	for (ParseNode* child : node->children) {
		collectComments(child, out);
	}
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

	for (ParseNode* node : astNodes) {
		if (node) {
			collectComments(node, commentNodes); 
		}
	}


	for (ParseNode* comment : commentNodes) {
		std::string text = comment->token;
		const std::string prefix = "# @contract";

		// Check if the text starts with the prefix, using find() for compatibility
		if (text.find(prefix) == 0) {
			std::string contractStr = text.substr(prefix.size());

			try {
				// Use unique_ptr for automatic memory management
				std::unique_ptr<Contract> contract = ContractParser(contractStr).parseContract();

				// Output the parsed contract
				std::cout << "Parsed Contract: ";
				contract->print(std::cout);
				std::cout << std::endl;
			} catch (const std::exception& ex) {
				// Handle parsing errors
				std::cerr << "Error parsing contract: " << ex.what() << std::endl;
			}
		}
	}

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
		setenv("R_HOME", "/usr/lib64/R", 1);  // Adjust path for your R install
	}

	runParse(filename);
	return 0;
}


