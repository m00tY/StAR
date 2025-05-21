#ifndef GENSOURCE_H
#define GENSOURCE_H

#include <string>
#include <vector>
#include <functional>

#include <R.h>
#include <R_ext/Rdynload.h>
#include <Rinternals.h>
#include <Rembedded.h>
#include <R_ext/Parse.h>

#undef length

struct StatementRange {
    size_t start;
    size_t end;
};

std::vector<StatementRange> extractStatements(const std::vector<ParseNode*> nodes);

std::vector<std::string> getStatementStrings(const std::vector<ParseNode*> nodes, std::vector<StatementRange> ranges);

std::string generateTypeCheck(const std::string& argName, const std::string& typeName);

void injectInputTypeChecks(const char *outputPath);
void generateOutputTypeChecks(const char *outputPath);

#endif
