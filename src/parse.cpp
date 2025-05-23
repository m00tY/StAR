#include <R.h>
#include <Rinternals.h>
#include <R_ext/Parse.h>
#include <Rembedded.h>

#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <functional>

#include "parse.h"

SEXP tokenizeRSource(const char* filename) {
    int r_argc = 2;
    char* r_argv[] = {
        const_cast<char*>("R"),
        const_cast<char*>("--silent")
    };

    FILE* file = fopen(filename, "r");
    if (!file) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        Rf_endEmbeddedR(0);
        return R_NilValue;
    }

    std::string source;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), file)) {
        source += buffer;
    }
    fclose(file);

    if (source.empty()) {
        std::cerr << "Error: File is empty." << std::endl;
        Rf_endEmbeddedR(0);
        return R_NilValue;
    }

    SEXP srcfileCall = PROTECT(Rf_lang2(Rf_install("srcfile"), Rf_mkString(filename)));
    SEXP srcfile = PROTECT(Rf_eval(srcfileCall, R_GlobalEnv));

    SEXP text = PROTECT(Rf_mkString(source.c_str()));
    ParseStatus status;
    SEXP exprs = PROTECT(R_ParseVector(text, -1, &status, srcfile));
    if (status != PARSE_OK) {
        std::cerr << "Error: Parsing failed." << std::endl;
        UNPROTECT(4);
        Rf_endEmbeddedR(0);
        return R_NilValue;
    }

    SEXP gpdCall = PROTECT(Rf_lang2(Rf_install("getParseData"), exprs));
    SEXP result = PROTECT(Rf_eval(gpdCall, R_GlobalEnv));

    if (!Rf_inherits(result, "data.frame")) {
        std::cerr << "Sanity check failed: result is not a data.frame!" << std::endl;
        UNPROTECT(6); 
        Rf_endEmbeddedR(0);
        return R_NilValue;
    }

    if (result == R_NilValue) {
        std::cerr << "Error: getParseData returned NULL." << std::endl;
    } else if (!Rf_inherits(result, "data.frame")) {
        std::cerr << "Error: getParseData did not return a data.frame." << std::endl;
    } else {
        std::cout << "Parse data successfully returned as data.frame." << std::endl;
    }

    UNPROTECT(6);
    
    return result;
}

std::vector<ParseNode*> generateAST(SEXP parsedData) {
    std::vector<ParseNode*> roots;
    std::unordered_map<int, ParseNode*> nodeMap;

    if (!Rf_inherits(parsedData, "data.frame")) {
        std::cerr << "Error: Parsed data is not a data.frame." << std::endl;
        return roots;
    }

    int nrows = Rf_length(VECTOR_ELT(parsedData, 0));  
    SEXP colnames = Rf_getAttrib(parsedData, R_NamesSymbol);  

    if (colnames == R_NilValue) {
        std::cerr << "Error: Parsed data has no column names." << std::endl;
        return roots;
    }

    int idIndex = -1, parentIndex = -1, tokenIndex = -1, textIndex = -1;
    for (int i = 0; i < Rf_length(colnames); ++i) {
        std::string name = CHAR(STRING_ELT(colnames, i));
        if (name == "id") idIndex = i;
        else if (name == "parent") parentIndex = i;
        else if (name == "token") tokenIndex = i;
        else if (name == "text") textIndex = i;
    }

    if (idIndex < 0 || parentIndex < 0 || tokenIndex < 0 || textIndex < 0) {
        std::cerr << "Error: Could not find required columns in parse data." << std::endl;
        return roots;
    }

    SEXP idCol = VECTOR_ELT(parsedData, idIndex);
    SEXP parentCol = VECTOR_ELT(parsedData, parentIndex);
    SEXP tokenCol = VECTOR_ELT(parsedData, tokenIndex);
    SEXP textCol = VECTOR_ELT(parsedData, textIndex);

    for (int i = 0; i < nrows; ++i) {
        auto* node = new ParseNode;
        node->id = INTEGER(idCol)[i];
        node->parent = INTEGER(parentCol)[i];
        node->token = CHAR(STRING_ELT(tokenCol, i));
        node->text = CHAR(STRING_ELT(textCol, i));

        if (node->token == "SYMBOL_FUNCTION_CALL") {
            node->addArgument("arg1");
            node->addArgument("arg2");
        }

        nodeMap[node->id] = node;
    }

    std::vector<ParseNode*> orderedNodes;
    for (auto& [id, node] : nodeMap) {
        orderedNodes.push_back(node);
    }

    std::sort(orderedNodes.begin(), orderedNodes.end(), [](ParseNode* a, ParseNode* b) {
        return a->id < b->id;
    });

    for (auto* node : orderedNodes) {
        if (node->parent == 0 || node->parent == R_NaInt) {
            roots.push_back(node);
        } else {
            auto it = nodeMap.find(node->parent);
            if (it != nodeMap.end()) {
                it->second->children.push_back(node);
            } else {
                roots.push_back(node);
            }
        }
    }

    return roots;
}

void debugAST(ParseNode* node, int depth) {
    std::string indent(depth * 2, ' ');

    if (node->token == "COMMENT") {
        std::cout << indent << "[COMMENT] " << node->text << std::endl;
    } else {
        std::cout << indent << node->token << ": " << node->text << std::endl;
    }

    if (!node->arguments.empty()) {
        std::cout << indent << "  Arguments: ";
        for (const auto& arg : node->arguments) {
            std::cout << arg << " ";
        }
        std::cout << std::endl;
    }

    for (ParseNode* child : node->children) {
        debugAST(child, depth + 1);
    }
}

std::vector<ParseNode*> flattenAST(const std::vector<ParseNode*>& roots) {
    std::vector<ParseNode*> ordered;

    std::function<void(ParseNode*)> collect = [&](ParseNode* node) {
        ordered.push_back(node);
        for (ParseNode* child : node->children) {
            collect(child);
        }
    };

    for (ParseNode* root : roots) {
        collect(root);
    }

    std::sort(ordered.begin(), ordered.end(), [](ParseNode* a, ParseNode* b) {
        return a->id < b->id;
    });

    return ordered;
}