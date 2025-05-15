#ifndef TYPELANG_H
#define TYPELANG_H

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <unordered_map>

class Type;
class ScalarType;
class VectorType;
class FunctionType;
class ListType;
class ClassType;
class NullableType;
class UnionType;
class EnvironmentType;

class Type {
public:
    virtual ~Type() = default;
    virtual std::string toString() const = 0;
    virtual bool isScalar() const { return false; }
    virtual bool isVector() const { return false; }
    virtual bool isFunction() const { return false; }
    virtual bool isList() const { return false; }
    virtual bool isClass() const { return false; }
    virtual bool isNullable() const { return false; }
    virtual bool isUnion() const { return false; }
    virtual bool isEnvironment() const { return false; }
};

class ScalarType : public Type {
private:
    std::string typeName;
public:
    ScalarType(const std::string& name) : typeName(name) {}
    std::string toString() const override;
    bool isScalar() const override { return true; }
};

class VectorType : public Type {
private:
    Type* baseType;
public:
    VectorType(Type* base) : baseType(base) {}
    std::string toString() const override;
    bool isVector() const override { return true; }
    Type* getBaseType() const { return baseType; }
};

class FunctionType : public Type {
public:
    FunctionType(const std::vector<Type*>& args, Type* ret)
        : arguments(args), returnType(ret) {}
    std::string toString() const override;
    bool isFunction() const override { return true; }
    const std::vector<Type*>& getArguments() const { return arguments; }
    Type* getReturnType() const { return returnType; }

	std::vector<Type*> arguments; 
    Type* returnType;
};

class ListType : public Type {
private:
    Type* elementType;
public:
    ListType(Type* elem) : elementType(elem) {}
    std::string toString() const override;
    bool isList() const override { return true; }
    Type* getElementType() const { return elementType; }
};

class ClassType : public Type {
private:
    std::vector<std::string> classIDs;

public:
    ClassType(const std::vector<std::string>& ids) : classIDs(ids) {}

    std::string toString() const override;
       

    bool isClass() const override { return true; }

    const std::vector<std::string>& getClassIDs() const { return classIDs; }
};
class NullableType : public Type {
private:
    Type* baseType;
public:
    NullableType(Type* base) : baseType(base) {}
    std::string toString() const override;
    bool isNullable() const override { return true; }
    Type* getBaseType() const { return baseType; }
};

class UnionType : public Type {
private:
    Type* leftType;
    Type* rightType;
public:
    UnionType(Type* left, Type* right) : leftType(left), rightType(right) {}
    std::string toString() const override;
    bool isUnion() const override { return true; }
    Type* getLeftType() const { return leftType; }
    Type* getRightType() const { return rightType; }
};

class EnvironmentType : public Type {
public:
    std::string toString() const override;
    bool isEnvironment() const override { return true; }
};

class NullType : public Type {
public:
    std::string toString() const override;
};

struct FunctionContract {
    std::vector<Type*> argTypes;
    Type* returnType;
};

struct ASTNode {
    std::string type;
    std::string name;
    std::vector<Type*> argumentTypes;
};

class TypeParser {
public:
    explicit TypeParser(const std::string& input);
    static void addContract(const std::string& functionName, const std::vector<Type*>& argumentTypes, Type* returnType);
	static void addFunctionContract(const std::string& name, const FunctionContract& contract);
    static void verifyFunctionCalls(const std::vector<ASTNode>& rootNodes);
	static bool typesAreCompatible(Type* actualType, Type* expectedType);
	static void verifySingleFunctionCall(const std::string& functionName, const std::vector<Type*>& argumentTypes);
	

	Type* parseType();
	std::vector<Type*> parseArgumentList();

    static std::unordered_map<std::string, FunctionContract> functionContracts;

private:
    std::string input;
    size_t pos;

    Type* parsePrimary();
    Type* parseUnion(Type* first);
    std::string parseIdentifier();
    void skipWhitespace();
    bool match(char c);
    bool consume(const std::string& s);
    void expect(char c);
    void expect(const std::string& s);
    bool peek(const std::string& s);


};

#endif 

