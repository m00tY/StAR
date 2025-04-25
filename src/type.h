#pragma once
#include <string>
#include <vector>
#include <memory>

struct Type {
    virtual ~Type() = default;
    virtual std::string str() const = 0;
};

using TypePtr = std::unique_ptr<Type>;

struct TopType : Type {
    std::string kind; // "any" or "env"
    TopType(std::string k) : kind(std::move(k)) {}
    std::string str() const override { return kind; }
};

struct ScalarType : Type {
    std::string name;
    ScalarType(std::string n) : name(std::move(n)) {}
    std::string str() const override { return name; }
};

struct NullableType : Type {
    TypePtr inner;
    NullableType(TypePtr t) : inner(std::move(t)) {}
    std::string str() const override { return "?" + inner->str(); }
};

struct VectorType : Type {
    TypePtr element;
    VectorType(TypePtr e) : element(std::move(e)) {}
    std::string str() const override { return element->str() + "[]"; }
};

struct UnionType : Type {
    std::vector<TypePtr> types;
    std::string str() const override {
        std::string result;
        for (size_t i = 0; i < types.size(); ++i) {
            if (i > 0) result += " | ";
            result += types[i]->str();
        }
        return result;
    }
};

struct FunctionType : Type {
    std::vector<TypePtr> args;
    TypePtr ret;
    std::string str() const override {
        std::string result = "(";
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) result += ", ";
            result += args[i]->str();
        }
        result += ") -> " + ret->str();
        return result;
    }
};

struct ListType : Type {
    TypePtr element;
    ListType(TypePtr e) : element(std::move(e)) {}
    std::string str() const override {
        return "list<" + element->str() + ">";
    }
};

struct ClassType : Type {
    std::string name;
    ClassType(std::string n) : name(std::move(n)) {}
    std::string str() const override {
        return "." + name;
    }
};

