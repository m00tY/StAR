#ifndef SOURCEPARSER_H
#define SOURCEPARSER_H

#include <Rinternals.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <functional>

struct ParseNode {
    int id;
    int parent;
    std::string token; 
    std::string text;
    std::vector<ParseNode*> children;
    std::vector<std::string> arguments;

    void addArgument(const std::string& arg) {
        arguments.push_back(arg);
    }
};

SEXP tokenizeRSource(const char* filename);

std::vector<ParseNode*> generateAST(SEXP parsedData);

void debugAST(ParseNode* node, int depth);

void walkAST(ParseNode* node, std::function<void(ParseNode*)> callback);

std::vector<ParseNode*> flattenAST(const std::vector<ParseNode*>& roots);

#endif 