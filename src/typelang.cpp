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
    skipWhitespace();
    std::cout << "[parseType] pos=" << pos 
              << ", remaining='" << input.substr(pos, std::min<size_t>(20, input.size() - pos)) 
              << "'\n";

    if (match('(')) {
        std::vector<Type*> args = parseArgumentList();
        expect(')');
        expect("->");
        Type* ret = parseType();
        return new FunctionType(args, ret);
    }

    if (consume("list<")) {
        Type* inner = parseType();
        expect('>');
        return new ListType(inner);
    }

    if (consume("class<")) {
        std::vector<std::string> ids;
        ids.push_back(parseIdentifier());
        while (match(',')) {
            ids.push_back(parseIdentifier());
        }
        expect('>');
        return new ClassType(ids);
    }

    Type* base = new ScalarType(parseIdentifier());

    if (match('?')) {
        base = new NullableType(base);
    }
    if (match('[')) {
        expect(']');
        base = new VectorType(base);
    }

    return parseUnion(base);
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

void TypeParser::verifyFunctionCalls(const std::vector<ASTNode>& rootNodes) {
    for (const auto& node : rootNodes) {
        if (node.type == "function_call") {
            std::string functionName = node.name;
            std::vector<Type*> argumentTypes = node.argumentTypes;

            TypeParser::verifySingleFunctionCall(functionName, argumentTypes);
        }
    }
}

void TypeParser::verifySingleFunctionCall(const std::string& functionName, const std::vector<Type*>& argumentTypes) {
    auto it = functionContracts.find(functionName);
    if (it == functionContracts.end()) {
        std::cerr << "Warning: No contract for function: " << functionName << std::endl;
        return;
    }

    const FunctionContract& contract = it->second;

    if (contract.argTypes.size() != argumentTypes.size()) {
        std::cerr << "Contract arity mismatch for " << functionName << ": expected "
                  << contract.argTypes.size() << ", got " << argumentTypes.size() << std::endl;
        return;
    }

    for (size_t i = 0; i < argumentTypes.size(); ++i) {
        if (contract.argTypes[i]->toString() != argumentTypes[i]->toString()) {
            std::cerr << "Type mismatch in " << functionName << " argument " << i << ": expected "
                      << contract.argTypes[i]->toString() << ", got "
                      << argumentTypes[i]->toString() << std::endl;
        }
    }
}

