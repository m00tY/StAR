#include "typelang.h"
#include <iostream>
#include <unordered_map>

std::string ScalarType::toString() const {
    return typeName;
}

std::string VectorType::toString() const {
    return baseType->toString() + "[]"; 
}

std::string FunctionType::toString() const {
    std::string result = "(";
    for (size_t i = 0; i < arguments.size(); ++i) {
        result += arguments[i]->toString();
        if (i < arguments.size() - 1) result += ", ";
    }
    result += ") -> " + returnType->toString();
    return result;
}

std::string ListType::toString() const {
    return "list<" + elementType->toString() + ">";
}

std::string ClassType::toString() const {
    std::string result = "class<";
    for (size_t i = 0; i < classIDs.size(); ++i) {
        result += classIDs[i];
        if (i < classIDs.size() - 1) result += ", ";
    }
    result += ">";
    return result;
}

std::string NullableType::toString() const {
    return baseType->toString() + "?";
}

std::string UnionType::toString() const {
    return leftType->toString() + " | " + rightType->toString();
}

std::string EnvironmentType::toString() const {
    return "env";
}

std::string NullType::toString() const {
    return "null";
}

TypeParser::TypeParser(const std::string& input) : input(input), pos(0) {}

std::unordered_map<std::string, FunctionContract> TypeParser::functionContracts;

Type* TypeParser::parseType() {
    Type* left = parsePrimary();
    if (match('?')) {
        return new NullableType(left);
    }
    if (match('[') && match(']')) {
        return new VectorType(left);
    }
    return parseUnion(left);
}

Type* TypeParser::parsePrimary() {
    skipWhitespace();

    if (match('(')) {
        auto args = parseArgumentList();
        expect("->");
        Type* ret = parseType();
        expect(')');
        return new FunctionType(args, ret);
    }

    if (consume("list<")) {
        Type* inner = parseType();
        expect('>');
        return new ListType(inner);
    }

    if (consume("class<")) {
        std::string name = parseIdentifier();
        expect('>');
        return new ClassType(std::vector<std::string>{name});
    }

    return new ScalarType(parseIdentifier());
}

Type* TypeParser::parseUnion(Type* first) {
    skipWhitespace();
    if (!match('|')) return first;

    Type* left = first;
    do {
        Type* right = parseType();
        left = new UnionType(left, right);
    } while (match('|'));

    return left;
}

std::vector<Type*> TypeParser::parseArgumentList() {
    std::vector<Type*> args;
    skipWhitespace();
    while (!peek(")")) {
        args.push_back(parseType());
        skipWhitespace();
        match(','); 
    }
    return args;
}

std::string TypeParser::parseIdentifier() {
    skipWhitespace();
    size_t start = pos;
    while (pos < input.size() && (std::isalnum(input[pos]) || input[pos] == '_')) ++pos;
    return input.substr(start, pos - start);
}

void TypeParser::skipWhitespace() {
    while (pos < input.size() && std::isspace(input[pos])) ++pos;
}

bool TypeParser::match(char c) {
    skipWhitespace();
    if (pos < input.size() && input[pos] == c) {
        ++pos;
        return true;
    }
    return false;
}

bool TypeParser::consume(const std::string& s) {
    skipWhitespace();
    if (input.substr(pos, s.size()) == s) {
        pos += s.size();
        return true;
    }
    return false;
}

void TypeParser::expect(char c) {
    if (!match(c)) {
        throw std::runtime_error(std::string("Expected '") + c + "' at position " + std::to_string(pos));
    }
}

void TypeParser::expect(const std::string& s) {
    if (!consume(s)) {
        throw std::runtime_error("Expected \"" + s + "\" at position " + std::to_string(pos));
    }
}

bool TypeParser::peek(const std::string& s) {
    skipWhitespace();
    return input.substr(pos, s.size()) == s;
}

bool TypeParser::typesAreCompatible(Type* actualType, Type* expectedType) {
    return actualType->toString() == expectedType->toString();
}

void TypeParser::addContract(const std::string& functionName, const std::vector<Type*>& argumentTypes, Type* returnType) {
    functionContracts[functionName] = {argumentTypes, returnType};
}

void TypeParser::addFunctionContract(const std::string& name, const FunctionContract& contract) {
    functionContracts[name] = contract;
}

void TypeParser::verifyFunctionCall(const std::string& functionName, const std::vector<Type*>& actualArgs) {
    auto contractIt = functionContracts.find(functionName);
    if (contractIt == functionContracts.end()) {
        std::cerr << "Contract for function " << functionName << " not found.\n";
        return;
    }

    const FunctionContract& contract = contractIt->second;

    if (contract.argTypes.size() != actualArgs.size()) {
        std::cerr << "Argument count mismatch for function " << functionName << ". Expected "
                  << contract.argTypes.size() << ", got " << actualArgs.size() << ".\n";
        return;
    }

    for (size_t i = 0; i < actualArgs.size(); ++i) {
        if (!typesAreCompatible(actualArgs[i], contract.argTypes[i])) {
            std::cerr << "Argument type mismatch for function " << functionName << " at position "
                      << i + 1 << ". Expected " << contract.argTypes[i]->toString() << ", got "
                      << actualArgs[i]->toString() << ".\n";
        }
    }

    // Check return type
    if (!typesAreCompatible(actualArgs.back(), contract.returnType)) {
        std::cerr << "Return type mismatch for function " << functionName << ". Expected "
                  << contract.returnType->toString() << ", got "
                  << actualArgs.back()->toString() << ".\n";
    }
}
