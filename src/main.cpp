#include <R.h>
#include <R_ext/Rdynload.h>
#include <Rinternals.h>
#include <Rembedded.h>
#include <R_ext/Parse.h>

#undef length

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <cerrno>
#include <unistd.h> 
#include <sys/stat.h>
#include <algorithm>
#include <cctype>

#include "parse.h"
#include "typelang.h"
#include "gensource.h"

bool startsWith(const std::string& str, const char* prefix) {
	size_t lenPrefix = std::strlen(prefix);
	if (str.size() < lenPrefix) return false;
	return std::strncmp(str.c_str(), prefix, lenPrefix) == 0;
}

std::vector<ParseNode*> parseCheckExprFromString(const std::string& code) {
	SEXP expr, status;
	PROTECT(expr = Rf_mkString(code.c_str()));

	ParseStatus parseStatus;
	SEXP parsed = PROTECT(R_ParseVector(expr, -1, &parseStatus, R_NilValue));

	if (parseStatus != PARSE_OK) {
		UNPROTECT(2);
		throw std::runtime_error("Failed to parse injected type check expression.");
	}

	SEXP firstExpr = VECTOR_ELT(parsed, 0);
	std::vector<ParseNode*> nodes = generateAST(firstExpr);
	UNPROTECT(2);
	return nodes;
}

SEXP tokenizeRString(const char* code) {
    SEXP expr = PROTECT(Rf_mkString(code));
    SEXP srcfile = PROTECT(R_ParseVector(expr, -1, NULL, R_NilValue));
    UNPROTECT(2);
    return srcfile;
}

std::vector<ParseNode*> generateASTFromSource(const std::string& code) {
    SEXP tokens = tokenizeRString(code.c_str());
    std::vector<ParseNode*> ast = generateAST(tokens);
    return ast;
}

void run(const char* filename, const char* outputPath) {
    std::cout << "Starting parsing process for file: " << filename << std::endl;

    SEXP tokens = tokenizeRSource(filename);
    if (!Rf_inherits(tokens, "data.frame")) {
        std::cerr << "Error: tokenization did not return a data.frame." << std::endl;
        return;
    }

    std::vector<ParseNode*> rootNodes = generateAST(tokens);
    std::vector<ParseNode*> flatAST = flattenAST(rootNodes);

    TypeParser::functionContracts.clear();

    // --- Extract @contract annotations from file comments
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        if (!startsWith(line, "# @contract")) continue;

        // Remove the "# @contract" prefix
        std::string contractLine = line.substr(11);
        contractLine.erase(0, contractLine.find_first_not_of(" \t"));

        std::istringstream iss(contractLine);
        std::string functionName;
        iss >> functionName;

        std::string typeExpr;
        std::getline(iss, typeExpr);
        typeExpr.erase(0, typeExpr.find_first_not_of(" \t"));

        try {
            TypeParser parser(typeExpr);
            FunctionType* funcType = dynamic_cast<FunctionType*>(parser.parseType());
            if (funcType) {
                TypeParser::addFunctionContract(functionName, FunctionContract{
                    .argTypes = funcType->arguments,
                    .returnType = funcType->returnType
                });
            }
        } catch (const std::exception& e) {
            std::cerr << "Contract parse error: " << e.what() << std::endl;
        }
    }

    // --- Inject runtime type checks inside function bodies (WIP)
    if (flatAST.size() < 5) {
        std::cerr << "Error: flatAST does not have enough elements to process function definitions." << std::endl;
        return;
    }

    for (size_t i = 0; i + 4 < flatAST.size(); ++i) {
        if (!flatAST[i]) {
            std::cerr << "Error: flatAST[" << i << "] is null." << std::endl;
            continue;
        }
        // Check for function definition pattern
        if (flatAST[i + 1]->text == "<-" &&
            flatAST[i + 2]->text == "function" &&
            flatAST[i + 3]->text == "(") {

            std::string funcName = flatAST[i]->text;
            auto it = TypeParser::functionContracts.find(funcName);
            if (it == TypeParser::functionContracts.end()) continue;
            const FunctionContract& contract = it->second;

            // Collect argument names
            std::vector<std::string> argNames;
            size_t j = i + 4;
            while (j < flatAST.size() && flatAST[j] != nullptr && flatAST[j]->text != ")") {
                if (flatAST[j] != nullptr && !flatAST[j]->text.empty() && flatAST[j]->text != ",") {
                    argNames.push_back(flatAST[j]->text);
                }
                ++j;
            }
            if (j >= flatAST.size() || !flatAST[j] || flatAST[j]->text != ")") continue;

            size_t bodyStart = j + 1;
            while (bodyStart < flatAST.size() && flatAST[bodyStart] != nullptr && flatAST[bodyStart]->text != "{") {
                ++bodyStart;
            }
            if (bodyStart >= flatAST.size()) continue;

            ++bodyStart;

            // Insert stopifnot(...) checks for each argument
            std::vector<std::pair<size_t, std::vector<ParseNode*>>> insertions;

            for (size_t k = 0; k < argNames.size() && k < contract.argTypes.size(); ++k) {
                if (contract.argTypes[k] == nullptr) {
                    std::cerr << "Error: Argument type at index " << k << " is null for function " << funcName << std::endl;
                    continue;
                }

                std::string typeStr = contract.argTypes[k]->toString();

                std::string checkExpr = "stopifnot(typeof(" + argNames[k] + ") == \"" + typeStr + "\")";

                std::vector<ParseNode*> checkNodes = generateASTFromSource(checkExpr);

                insertions.emplace_back(bodyStart, checkNodes);
            }

            for (const auto& insertion : insertions) {
                flatAST.insert(flatAST.begin() + insertion.first, insertion.second.begin(), insertion.second.end());
                bodyStart += insertion.second.size();
            }
        }
    }

    // --- Re-extract statements and reconstruct source code (WIP)
    std::vector<StatementRange> statementRanges = extractStatements(flatAST);
    std::vector<std::string> statementStrings = getStatementStrings(flatAST, statementRanges);

    std::cout << "Writing to file: " << outputPath << std::endl;
    
    int fd = open(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    for (const auto& str : statementStrings) {
		std::string trimmed = str;
		trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());
	
		std::string line = str;
		if (!trimmed.empty()) {
			line += ";";
		}
		line += "\n";
	
		std::cout << line;
		write(fd, line.c_str(), line.size());
	}

    close(fd);
    Rf_endEmbeddedR(0);
}


bool fileExists(const char* path) {
	struct stat buffer;
	return stat(path, &buffer) == 0;
}

int main(int argc, char* argv[]) {
	if (argc != 5) {
		std::cerr << "Usage: " << argv[0] << " run <filename>" << std::endl;
		return 1;
	}

	const char* commandFlag = argv[1];
	const char* filename = argv[2];
	const char* outputFlag = argv[3];
	const char* outputPath = argv[4];

	if (strcmp(commandFlag, "run") != 0) {
		std::cerr << "Error: Unknown flag '" << commandFlag << "'" << std::endl;
		return 1;
	}

	if (!fileExists(filename)) {
		std::cerr << "Error: File not found: " << filename << std::endl;
		return 1;
	}

	if (strcmp(outputFlag, "-o") != 0) {
		std::cerr << "Error: Unknown flag '" << outputFlag << "'" << std::endl;
		return 1;
	}

	if (std::getenv("R_HOME") == nullptr) {
		setenv("R_HOME", "/usr/lib64/R", 1);
	}

	run(filename, outputPath);
	return 0;
}
