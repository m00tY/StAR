#include <sstream>
#include <unordered_map>

#include "parse.h"
#include "gensource.h"


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
        result.push_back(oss.str());
    }

    return result;
}


std::string generateTypeCheck(const std::string& argName, const std::string& typeName) {
    enum TypeEnum { INT, DOUBLE, LOGICAL, UNKNOWN };

    static const std::unordered_map<std::string, TypeEnum> typeMap = {
        {"integer", INT},
        {"double", DOUBLE},
        {"logical", LOGICAL},
    };

    auto it = typeMap.find(typeName);
    TypeEnum t = (it != typeMap.end()) ? it->second : UNKNOWN;

    switch (t) {
        case INT:
            return "if (!is.integer(" + argName + ")) stop(\"Argument '" + argName + "' must be integer\")";
        case DOUBLE:
            return "if (!is.double(" + argName + ")) stop(\"Argument '" + argName + "' must be double\")";
        case LOGICAL:
            return "if (!is.logical(" + argName + ")) stop(\"Argument '" + argName + "' must be logical\")";
        default:
            return "";
    }
}

