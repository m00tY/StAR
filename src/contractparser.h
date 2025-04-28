#ifndef CONTRACTPARSER_H
#define CONTRACTPARSER_H

#include <string>
#include <vector>

enum class ContractTokenType {
    LParen,
    RParen,
    Arrow,
    Comma,
    Identifier,
    EndOfFile,
};

struct ContractToken {
    ContractTokenType type;
    std::string text;
};

class ContractLexer {
public:
    explicit ContractLexer(const std::string& input);

    ContractToken nextToken();

private:
    std::string input;
    size_t pos;

    char peek();
    void skipWhitespace();
};

struct Type {
	std::string name;
};

struct Contract {
	std::vector<Type> argumentTypes;
	Type returnType;
};

class ContractParser {
public:
	explicit ContractParser(ContractLexer& lexer);

	Contract parseContract();
private:
	ContractLexer& lexer;
	ContractToken current;

	void advance();
	void expect(ContractTokenType type);
	std::vector<Type> parseTypeList();

	Type parseType();
};

#endif
