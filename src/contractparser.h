#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include "type.h"

struct Type; 
using TypePtr = std::unique_ptr<Type>;

struct Contract {
    std::vector<TypePtr> arg_types;
    TypePtr return_type;

    void print(std::ostream& out) const;
};

class ContractParser {
public:
    explicit ContractParser(const std::string& input);
	std::unique_ptr<Contract> parseContract();

	
	
private:
    std::string input;
    size_t pos;

    char peek() const;
    char advance();
    bool match(char expected);
    void skipWhitespace();

    std::string consumeWhile(bool (*predicate)(char));
    std::string consumeIdentifier();

    TypePtr parseType();
    TypePtr parseUnion();
    TypePtr parsePrimary();
    TypePtr parseFunction();
    std::vector<TypePtr> parseArguments();
};

