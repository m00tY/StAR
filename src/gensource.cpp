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

std::string extractReturnExpression(const std::vector<std::string>& lines, size_t returnLineIndex) {
    std::cerr << "Extracting return expression from line " << returnLineIndex << ": " << lines[returnLineIndex] << std::endl;

    size_t returnPos = lines[returnLineIndex].find("return");
    if (returnPos != std::string::npos) {
        size_t exprStart = returnPos + 6; // Length of "return"
        while (exprStart < lines[returnLineIndex].size() && std::isspace(lines[returnLineIndex][exprStart])) {
            ++exprStart;
        }

        // Start building the return expression
        std::string returnExpression = lines[returnLineIndex].substr(exprStart);

        // Check if the expression spans multiple lines
        size_t currentLine = returnLineIndex + 1;
        while (currentLine < lines.size() && lines[currentLine].find("}") == std::string::npos) {
            returnExpression += " " + lines[currentLine];
            ++currentLine;
        }

        // Trim any trailing whitespace
        returnExpression.erase(returnExpression.find_last_not_of(" \t\n\r") + 1);

        std::cerr << "Extracted return expression: " << returnExpression << std::endl;
        return returnExpression;
    }
    std::cerr << "No return expression found on line: " << returnLineIndex << std::endl;
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
            std::cerr << "Processing function: " << functionName << std::endl;

            // Look for a contract for the function
            auto it = TypeParser::functionContracts.find(functionName);
            if (it != TypeParser::functionContracts.end()) {
                const FunctionContract &contract = it->second;
                std::cerr << "Found contract for function: " << functionName << std::endl;

                // Write the function declaration line
                outFile << lines[i] << "\n";

                // Process the function block
                size_t j = i + 1;
                bool returnFound = false;
                int braceDepth = 0;

                // Find the opening brace of the function
                while (j < lines.size() && lines[j].find("{") == std::string::npos) {
                    outFile << lines[j] << "\n";
                    ++j;
                }

                if (j < lines.size() && lines[j].find("{") != std::string::npos) {
                    ++braceDepth; // Entering the function block
                    outFile << lines[j] << "\n";
                    ++j;
                }

                while (j < lines.size() && braceDepth > 0) {
                    std::cerr << "Inspecting line " << j << ": " << lines[j] << std::endl;

                    // Adjust brace depth
                    if (lines[j].find("{") != std::string::npos) {
                        ++braceDepth;
                    }
                    if (lines[j].find("}") != std::string::npos) {
                        --braceDepth;
                    }

                    if (lines[j].find("return") != std::string::npos && braceDepth == 1) {
                        std::cerr << "Found return statement on line " << j << ": " << lines[j] << std::endl;
                        returnFound = true;
                        std::string returnExpression = extractReturnExpression(lines, j);
                        std::string returnType = contract.returnType->toString();
                        std::cerr << "Return type for function " << functionName << ": " << returnType << std::endl;
                        std::cerr << "Return expression: " << returnExpression << std::endl;

                        if (!returnType.empty() && returnType.size() > 2 && returnType.substr(returnType.size() - 2) == "[]") {
                            // Handle vector types (e.g., integer[])
                            std::string baseType = returnType.substr(0, returnType.size() - 2);
                            std::cerr << "Injecting type checks for vector return type: " << returnType << std::endl;
                            outFile << "outputTypecheckExpression <- (" << returnExpression << ")\n";
                            outFile << "if (!is.list(outputTypecheckExpression)) ";
                            outFile << "stop('Output must be a list')\n";
                            outFile << "if (!all(sapply(outputTypecheckExpression, is." << baseType << "))) ";
                            outFile << "stop('All elements in the output list must be of type " << baseType << "')\n";
                        } else {
                            // Handle scalar types (e.g., integer, double, logical)
                            std::cerr << "Injecting type checks for scalar return type: " << returnType << std::endl;
                            outFile << "outputTypecheckExpression <- (" << returnExpression << ")\n";
                            outFile << "if (!is." << returnType << "(outputTypecheckExpression)) ";
                            outFile << "stop('Output must be of type " << returnType << "')\n";
                        }
                    }

                    outFile << lines[j] << "\n";
                    ++j;
                }

                if (!returnFound) {
                    std::cerr << "Warning: No return statement found for function: " << functionName << std::endl;
                }

                // Skip to the end of the function block
                i = j - 1;
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