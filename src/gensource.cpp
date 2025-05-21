#include <sstream>
#include <unordered_map>
#include <fstream>
#include <regex>

#include "parse.h"
#include "gensource.h"
#include "typelang.h"

#undef length


std::vector<StatementRange> extractStatements(const std::vector<ParseNode*> nodes) {
    std::vector<StatementRange> statements;

    int parenDepth = 0;
    int braceDepth = 0;
    size_t stmtStart = 0;
    bool inStatement = false;

    for (size_t i = 0; i < nodes.size(); ++i) {
        const std::string& tok = nodes[i]->text;

        // Skip standalone comments
        if (tok.rfind("#", 0) == 0) {
            while (i + 1 < nodes.size() && nodes[i + 1]->text.rfind("#", 0) == 0)
                ++i;
            continue;
        }

        if (!inStatement && parenDepth == 0 && braceDepth == 0) {
            stmtStart = i;
            inStatement = true;
        }

        if (tok == "(") ++parenDepth;
        else if (tok == ")") --parenDepth;
        else if (tok == "{") ++braceDepth;
        else if (tok == "}") --braceDepth;

        if (inStatement && parenDepth == 0 && braceDepth == 0) {
            if (tok == ")" || tok == "}" || i + 1 == nodes.size() || nodes[i + 1]->text.rfind("#", 0) == 0) {
                statements.push_back({stmtStart, i});
                inStatement = false;
            }
        }
    }

    return statements;
}



std::vector<std::string> getStatementStrings(const std::vector<ParseNode*> nodes, std::vector<StatementRange> ranges) {
    std::vector<std::string> result;

    for (const auto& range : ranges) {
        std::ostringstream oss;
        for (size_t i = range.start; i <= range.end; ++i) {
            if (i > range.start) oss << " ";
            oss << nodes[i]->text;
        }

        // Add a newline after each statement to mimic real R code
        oss << "\n";
        result.push_back(oss.str());
    }

    return result;
}

void injectInputTypeChecks(const char *outputPath) {
    std::cout << "Injecting type checks into file: " << outputPath << std::endl;

    std::ifstream file(outputPath);
    if (!file.is_open()) {
        std::cerr << "Error: Failed to open file: " << outputPath << std::endl;
        return;
    }

    std::vector<std::string> lines;
    std::string line;

    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    file.close();

    std::ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        std::cerr << "Error: Failed to open file for writing: " << outputPath << std::endl;
        return;
    }

    for (size_t i = 0; i < lines.size(); ++i) {
        std::smatch match;
        if (std::regex_search(lines[i], match, std::regex("^(\\w+)\\s*<-\\s*function\\s*\\(([^)]*)\\)"))) {
            std::string functionName = match[1];
            std::string args = match[2];

            auto it = TypeParser::functionContracts.find(functionName);
            if (it != TypeParser::functionContracts.end()) {
                const FunctionContract &contract = it->second;

                outFile << lines[i] << "\n";

                while (i + 1 < lines.size() && lines[i + 1].find("{") == std::string::npos) {
                    outFile << lines[++i] << "\n";
                    args += lines[i];
                }

                if (i + 1 < lines.size() && lines[i + 1].find("{") != std::string::npos) {
                    outFile << lines[i + 1] << "\n";
                    ++i;

                    std::vector<std::string> argNames;
                    std::istringstream argStream(args);
                    std::string argName;
                    while (std::getline(argStream, argName, ',')) {
                        argName.erase(0, argName.find_first_not_of(" \t"));
                        argName.erase(argName.find_last_not_of(" \t") + 1);
                        argNames.push_back(argName);
                    }

                    for (size_t j = 0; j < contract.argTypes.size(); ++j) {
                        if (j < argNames.size() && contract.argTypes[j]) {
                            std::string argType = contract.argTypes[j]->toString();
                            std::cerr << "argType: " << argType << std::endl;
                            if (!argType.empty() && argType.size() > 2 && argType.substr(argType.size() - 2) == "[]") {
                                std::string baseType = argType.substr(0, argType.size() - 2);
                                outFile << "stopifnot(is.list(" << argNames[j] << "))\n";
                                outFile << "stopifnot(all(sapply(" << argNames[j] << ", is." << baseType << ")))\n";
                            } else {
                                outFile << "stopifnot(is." << argType << "(" << argNames[j] << "))\n";
                            }
                        }
                    }
                }
            } else {
                std::cerr << "Warning: No contract found for function: " << functionName << std::endl;
            }
        } else {
            outFile << lines[i] << "\n";
        }
    }

    outFile.close();
    std::cout << "Type checks successfully injected into file: " << outputPath << std::endl;
}

std::string extractReturnExpression(const std::string& returnLine) {
    size_t returnPos = returnLine.find("return");
    if (returnPos != std::string::npos) {
        size_t exprStart = returnPos + 6; // Length of "return"
        while (exprStart < returnLine.size() && std::isspace(returnLine[exprStart])) {
            ++exprStart;
        }
        return returnLine.substr(exprStart);
    }
    return "";
}

void generateOutputTypeChecks(const char *outputPath) {
    std::cout << "Generating output type checks for file: " << outputPath << std::endl;

    std::ifstream file(outputPath);
    if (!file.is_open()) {
        std::cerr << "Error: Failed to open file: " << outputPath << std::endl;
        return;
    }

    std::vector<std::string> lines;
    std::string line;

    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    file.close();

    std::ofstream outFile(outputPath, std::ios::out);
    if (!outFile.is_open()) {
        std::cerr << "Error: Failed to open file for writing: " << outputPath << std::endl;
        return;
    }

    for (size_t i = 0; i < lines.size(); ++i) {
        std::smatch match;
        if (std::regex_search(lines[i], match, std::regex("^(\\w+)\\s*<-\\s*function\\s*\\(([^)]*)\\)"))) {
            std::string functionName = match[1];

            // Look for a contract for the function
            auto it = TypeParser::functionContracts.find(functionName);
            if (it != TypeParser::functionContracts.end()) {
                const FunctionContract &contract = it->second;

                // Write the function declaration line
                outFile << lines[i] << "\n";

                // Process the function block
                size_t j = i + 1;
                while (j < lines.size() && lines[j].find("}") == std::string::npos) {
                    if (lines[j].find("return") != std::string::npos) {
                        std::string returnExpression = extractReturnExpression(lines[j]);
                        std::string returnType = contract.returnType->toString();

                        outFile << "outputTypecheckExpression <- (" << returnExpression << ")\n";
                        outFile << "if (!is." << returnType << "(outputTypecheckExpression)) ";
                        outFile << "stop('Output must be of type " << returnType << "')\n";
                    }
                    outFile << lines[j] << "\n";
                    ++j;
                }

                // Write the closing brace
                if (j < lines.size()) {
                    outFile << lines[j] << "\n";
                }

                // Skip to the end of the function block
                i = j;
            } else {
                std::cerr << "Warning: No contract found for function: " << functionName << std::endl;
                outFile << lines[i] << "\n";
            }
        } else {
            // Write the current line to the output file
            outFile << lines[i] << "\n";
        }
    }

    outFile.close();
    std::cout << "Output type checks successfully generated for file: " << outputPath << std::endl;
}