// main.cpp
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
#include <regex>

#include <R.h>
#include <R_ext/Rdynload.h>
#include <Rinternals.h>
#include <Rembedded.h>
#include <R_ext/Parse.h>

#undef length

#include "parse.h"
#include "typelang.h"
#include "gensource.h"

bool startsWith(const std::string &str, const char *prefix)
{
    size_t lenPrefix = std::strlen(prefix);
    return str.size() >= lenPrefix && std::strncmp(str.c_str(), prefix, lenPrefix) == 0;
}

SEXP tokenizeRString(const char *code)
{
    SEXP expr = PROTECT(Rf_mkString(code));
    ParseStatus parseStatus;
    SEXP parsed = PROTECT(R_ParseVector(expr, -1, &parseStatus, R_NilValue));
    if (parseStatus != PARSE_OK)
    {
        UNPROTECT(2);
        throw std::runtime_error("Failed to parse the provided R code.");
    }
    UNPROTECT(2);
    return parsed;
}

std::vector<ParseNode *> generateASTFromSource(const std::string &code)
{
    SEXP tokens = tokenizeRString(code.c_str());
    return generateAST(tokens);
}

void run(const char *filename, const char *outputPath)
{
    SEXP tokens = tokenizeRSource(filename);
    if (!Rf_inherits(tokens, "data.frame"))
    {
        std::cerr << "Error: tokenization did not return a data.frame." << std::endl;
        return;
    }

    std::vector<ParseNode *> rootNodes = generateAST(tokens);
    std::vector<ParseNode *> flatAST = flattenAST(rootNodes);

    // Load contracts
    TypeParser::functionContracts.clear();
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line))
    {
        if (!startsWith(line, "# @contract"))
            continue;
        std::string contractLine = line.substr(11);
        contractLine.erase(0, contractLine.find_first_not_of(" \t"));

        std::istringstream iss(contractLine);
        std::string functionName;
        iss >> functionName;
        std::string typeExpr;
        std::getline(iss, typeExpr);
        typeExpr.erase(0, typeExpr.find_first_not_of(" \t"));

        try
        {
            TypeParser parser(typeExpr);
            if (auto *funcType = dynamic_cast<FunctionType *>(parser.parseType()))
            {
                TypeParser::addFunctionContract(functionName, {.argTypes = funcType->arguments,
                                                               .returnType = funcType->returnType});
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Contract parse error: " << e.what() << std::endl;
        }
    }

    // Inject type checks for functions with contracts
    for (size_t i = 0; i + 4 < flatAST.size(); ++i)
    {
        if (!flatAST[i])
            continue;

        if (flatAST[i + 1]->text == "<-" &&
            flatAST[i + 2]->text == "function" &&
            flatAST[i + 3]->text == "(")
        {

            std::string funcName = flatAST[i]->text;
            auto it = TypeParser::functionContracts.find(funcName);
            if (it == TypeParser::functionContracts.end())
                continue;

            const auto &contract = it->second;

            std::vector<std::string> argNames;
            size_t j = i + 4;
            while (j < flatAST.size() && flatAST[j] && flatAST[j]->text != ")")
            {
                if (flatAST[j]->text != "," && !flatAST[j]->text.empty())
                {
                    argNames.push_back(flatAST[j]->text);
                }
                ++j;
            }

            if (j >= flatAST.size() || flatAST[j]->text != ")")
            {
                std::cerr << "Warning: Missing ')' in function " << funcName << std::endl;
                continue;
            }

            size_t bodyStart = j + 1;
            while (bodyStart < flatAST.size() && flatAST[bodyStart]->text != "{")
                ++bodyStart;
            if (bodyStart >= flatAST.size() || flatAST[bodyStart]->text != "{")
                continue;
            ++bodyStart;

            std::vector<std::vector<ParseNode *>> insertBlocks;
            for (size_t k = 0; k < argNames.size() && k < contract.argTypes.size(); ++k)
            {
                const Type *type = contract.argTypes[k];
                if (!type)
                    continue;
                std::string typeStr = type->toString();
                std::string checkExpr = "if (!is." + typeStr + "(" + argNames[k] + ")) stop('Argument `" + argNames[k] + "` must be of type " + typeStr + "')";
                std::vector<ParseNode *> checkNodes = generateASTFromSource(checkExpr);
                if (!checkNodes.empty())
                    insertBlocks.push_back(checkNodes);
            }

            for (auto it = insertBlocks.rbegin(); it != insertBlocks.rend(); ++it)
            {
                flatAST.insert(flatAST.begin() + bodyStart, it->begin(), it->end());
            }
        }
    }

    // Reassemble
    std::vector<StatementRange> statementRanges = extractStatements(flatAST);
    std::vector<std::string> statementStrings = getStatementStrings(flatAST, statementRanges);

    std::cout << "Writing to file: " << outputPath << std::endl;
    int fd = open(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    for (const auto &stmt : statementStrings) {
        std::string line = stmt;
    
        line = std::regex_replace(line, std::regex("^\\s+|\\s+$"), "");
    
        line = std::regex_replace(line, std::regex("\\}"), "\n}");
    
        line = std::regex_replace(line, std::regex("\\{"), "{\n");
    
        line = std::regex_replace(line, std::regex(";"), ";\n");
    
        line = std::regex_replace(line, std::regex("([a-zA-Z0-9_])\\s*\\("), "$1(");
    
        line = std::regex_replace(line, std::regex("\\s*=\\s*"), " = ");
        line = std::regex_replace(line, std::regex("\\s*\\+\\s*"), " + ");
        line = std::regex_replace(line, std::regex("\\s*<-\\s*"), " <- ");
        line = std::regex_replace(line, std::regex("\\s*/\\s*"), " / ");
        line = std::regex_replace(line, std::regex("\\s*\\*\\s*"), " * ");
    
        line = std::regex_replace(line, std::regex("\\$\\s*"), "$");

        line = std::regex_replace(line, std::regex("\\s*\\)\\s*"), " ) \n");
    
        // Optional: indent body lines inside functions
        if (!line.empty() && line != "{" && line != "}" && line.find("function") == std::string::npos)
            line = "    " + line;
    
        line += "\n";
        std::cout << line;
        write(fd, line.c_str(), line.size());
    }
    

    close(fd);
    Rf_endEmbeddedR(0);
}

bool fileExists(const char *path)
{
    struct stat buffer;
    return stat(path, &buffer) == 0;
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        std::cerr << "Usage: " << argv[0] << " run <filename> -o <output path>" << std::endl;
        return 1;
    }

    const char *commandFlag = argv[1];
    const char *filename = argv[2];
    const char *outputFlag = argv[3];
    const char *outputPath = argv[4];

    if (strcmp(commandFlag, "run") != 0 || strcmp(outputFlag, "-o") != 0)
    {
        std::cerr << "Invalid command or output flag." << std::endl;
        return 1;
    }

    if (!fileExists(filename))
    {
        std::cerr << "File not found: " << filename << std::endl;
        return 1;
    }

    if (!std::getenv("R_HOME"))
    {
        setenv("R_HOME", "/usr/lib64/R", 1);
    }

    int r_argc = 2;
    char *r_argv[] = {const_cast<char *>("R"), const_cast<char *>("--silent")};
    Rf_initEmbeddedR(r_argc, r_argv);

    try
    {
        run(filename, outputPath);
        injectInputTypeChecks(outputPath);
        generateOutputTypeChecks(outputPath);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        Rf_endEmbeddedR(0);
        return 1;
    }

    Rf_endEmbeddedR(0);
    return 0;
}
