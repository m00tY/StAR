#include <cctype>
#include <iterator>
#include <stdexcept>
#include <vector>

#include "contractparser.h"

ContractLexer::ContractLexer(const std::string& input)
    : input(input), pos(0) {}

ContractToken ContractLexer::nextToken() {
    skipWhitespace();

    if (pos >= input.size()) {
        return {ContractTokenType::EndOfFile, ""};
    }

    char c = input[pos];

    if (c == '(') {
        pos++;
        return {ContractTokenType::LParen, "("};
    }

    if (c == ')') {
        pos++;
        return {ContractTokenType::RParen, ")"};
    }

    if (c == ',') {
        pos++;
        return {ContractTokenType::Comma, ","};
    }

    if (c == '-' && peek() == '>') {
        pos += 2;
        return {ContractTokenType::Arrow, "->"};
    }

    if (std::isalpha(c)) {
        std::string ident;
        while (pos < input.size() && (std::isalnum(input[pos]) || input[pos] == '_')) {
            ident += input[pos++];
        }
        return {ContractTokenType::Identifier, ident};
    }

    throw std::runtime_error(std::string("Unexpected character: ") + c);
}

char ContractLexer::peek() {
    return (pos + 1 < input.size()) ? input[pos + 1] : '\0';
}

void ContractLexer::skipWhitespace() {
    while (pos < input.size() && std::isspace(input[pos])) {
        pos++;
    }
}



ContractParser::ContractParser(ContractLexer& lexer)
    : lexer(lexer), current() {
    advance();
}

Contract ContractParser::parseContract() {
	expect(ContractTokenType::LParen);
	advance();
	std::vector<Type> args = parseTypeList();
	expect(ContractTokenType::RParen);
	advance();
	expect(ContractTokenType::Arrow);
	advance();
	Type ret = parseType();
	return Contract{args, ret};
}

void ContractParser::advance() {
    current = lexer.nextToken();
}

void ContractParser::expect(ContractTokenType type) {
    if (current.type != type) {
        throw std::runtime_error("expected token type does not match, got: " + current.text);
    }
}

std::vector<Type> ContractParser::parseTypeList() {
	std::vector<Type> types;
	types.push_back(parseType());
	while (current.type == ContractTokenType::Comma) {
		advance();
		types.push_back(parseType());
	}
	return types;
}

Type ContractParser::parseType() {
	if (current.type != ContractTokenType::Identifier) {
		throw std::runtime_error("expected type name, got: " + current.text);
	}

	Type t{current.text};
	advance();
	return t;
} 
