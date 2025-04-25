#include <R.h>
#include <Rinternals.h>

#include <string>
#include <vector>

#ifndef AST_H
#define AST_H

struct ParseNode {
    int id;
    int parent;
    std::string token;
    std::string text;
    std::vector<ParseNode*> children;
};

SEXP tokenizeRSource(const char* filename);
std::vector<ParseNode*> generateAST (SEXP parseData);
void debugAST(ParseNode* node, int depth = 0);

#endif
