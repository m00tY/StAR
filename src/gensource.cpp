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

std::vector<std::string> preprocessLines(const std::vector<std::string>& lines) {
    std::vector<std::string> processedLines;
    for (const auto& line : lines) {
        std::istringstream stream(line);
        std::string statement;
        while (std::getline(stream, statement, ';')) { // Split by semicolon
            statement.erase(0, statement.find_first_not_of(" \t")); // Trim leading whitespace
            statement.erase(statement.find_last_not_of(" \t") + 1); // Trim trailing whitespace
            if (!statement.empty()) {
                processedLines.push_back(statement); // Add the statement to the processed lines
            }
        }
    }
    return processedLines;
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

    lines = preprocessLines(lines);

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

                // Write the function declaration line
                outFile << lines[i] << "\n";

                // Process the function arguments
                while (i + 1 < lines.size() && lines[i + 1].find("{") == std::string::npos) {
                    outFile << lines[++i] << "\n";
                    args += lines[i];
                }

                // Write the opening brace
                if (i + 1 < lines.size() && lines[i + 1].find("{") != std::string::npos) {
                    outFile << lines[++i] << "\n";

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
                            if (argType == "dataframe") {
                                outFile << "stopifnot(is.data.frame(" << argNames[j] << "))\n";
                            } else if (!argType.empty() && argType.size() > 2 && argType.substr(argType.size() - 2) == "[]") {
                                std::string baseType = argType.substr(0, argType.size() - 2);
                                outFile << "stopifnot(is.vector(" << argNames[j] << "))\n";
                                outFile << "stopifnot(all(sapply(" << argNames[j] << ", is." << baseType << ")))\n";
                            } else {
                                outFile << "stopifnot(is." << argType << "(" << argNames[j] << "))\n";
                            }
                        }
                    }
                }
            } else {
                std::cerr << "Warning: No contract found for function: " << functionName << std::endl;
                outFile << lines[i] << "\n";
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

        // Ensure balanced parentheses and braces
        size_t openParenCount = std::count(returnExpression.begin(), returnExpression.end(), '(');
        size_t closeParenCount = std::count(returnExpression.begin(), returnExpression.end(), ')');
        size_t openBraceCount = std::count(returnExpression.begin(), returnExpression.end(), '{');
        size_t closeBraceCount = std::count(returnExpression.begin(), returnExpression.end(), '}');

        while (closeParenCount < openParenCount) {
            returnExpression += ")";
            ++closeParenCount;
        }
        while (closeBraceCount < openBraceCount) {
            returnExpression += "}";
            ++closeBraceCount;
        }

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

    lines = preprocessLines(lines);

    std::ofstream outFile(outputPath, std::ios::out);
    if (!outFile.is_open()) {
        std::cerr << "Error: Failed to open file for writing: " << outputPath << std::endl;
        return;
    }

    for (size_t i = 0; i < lines.size(); ++i) {
        std::smatch match;
        if (std::regex_search(lines[i], match, std::regex("^(\\w+)\\s*<-\\s*function\\s*\\(([^)]*)\\)"))) {
            std::string functionName = match[1];

            outFile << lines[i] << "\n";

            auto it = TypeParser::functionContracts.find(functionName);
            if (it != TypeParser::functionContracts.end()) {
                const FunctionContract &contract = it->second;

                size_t j = i + 1;
                bool returnFound = false;

                while (j < lines.size() && lines[j].find("{") == std::string::npos) {
                    outFile << lines[j] << "\n";
                    ++j;
                }

                if (j < lines.size() && lines[j].find("{") != std::string::npos) {
                    outFile << lines[j] << "\n";
                    ++j;
                }

                while (j < lines.size()) {
                    if (lines[j].find("return") != std::string::npos) {
                        returnFound = true;
                        std::string returnExpression = extractReturnExpression(lines, j);
                        std::string returnType = contract.returnType->toString();

                        // Ensure balanced parentheses in the return expression
                        returnExpression = returnExpression;

                        if (returnType == "dataframe") {
                            outFile << "outputTypecheckExpression <- " << returnExpression << "\n";
                            outFile << "if (!is.data.frame(outputTypecheckExpression)) stop('Output must be a data frame')\n";
                        } else if (returnType.size() > 2 && returnType.substr(returnType.size() - 2) == "[]") {
                            std::string baseType = returnType.substr(0, returnType.size() - 2);
                            outFile << "outputTypecheckExpression <- " << returnExpression << "\n";
                            outFile << "if (!is.list(outputTypecheckExpression)) stop('Output must be a list')\n";
                            outFile << "if (!all(sapply(outputTypecheckExpression, is." << baseType << "))) stop('All elements in the output list must be of type " << baseType << "')\n";
                        } else {
                            outFile << "outputTypecheckExpression <- " << returnExpression << "\n";
                            outFile << "if (!is." << returnType << "(outputTypecheckExpression)) stop('Output must be of type " << returnType << "')\n";
                        }

                        outFile << "return(outputTypecheckExpression)\n";
                        ++j;
                        continue;
                    }

                    outFile << lines[j] << "\n";
                    ++j;
                }

                i = j - 1;
            }
        } else {
            outFile << lines[i] << "\n";
        }
    }

    outFile.close();
    std::cout << "Output type checks successfully generated for file: " << outputPath << std::endl;
}