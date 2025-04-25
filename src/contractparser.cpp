#include <cctype>
#include <stdexcept>
#include <regex>
#include <iostream>

#include "ast.h"
#include "contractparser.h"
#include "type.h"

void Contract::print(std::ostream& out) const {
    out << "Contract(";

    if (!arg_types.empty()) {
        out << "args: ";
        for (size_t i = 0; i < arg_types.size(); ++i) {
            out << arg_types[i]->str();
            if (i + 1 < arg_types.size()) out << ", ";
        }
    }

    out << "; returns: " << return_type->str();
    out << ")";
}

ContractParser::ContractParser(const std::string& in) : input(in), pos(0) {}

char ContractParser::peek() const {
	return pos < input.size() ? input[pos] : '\0';
}

char ContractParser::advance() {
	return pos < input.size() ? input[pos++] : '\0';
}

bool ContractParser::match(char expected) {
	if (peek() == expected) {
		++pos;
		return true;
	}
	return false;
}

void ContractParser::skipWhitespace() {
	while (std::isspace(peek())) advance();
}

std::string ContractParser::consumeWhile(bool(*predicate)(char)) {
	std::string result;
	while (predicate(peek())) result += advance();
	return result;
}

std::string ContractParser::consumeIdentifier() {
	return consumeWhile([](char c) { return std::isalnum(c) || c == '_'; });
}

std::unique_ptr<Contract> ContractParser::parseContract() {
	auto contract = std::make_unique<Contract>();
	return contract;
}


TypePtr ContractParser::parseType() {
	return parseUnion();
}

TypePtr ContractParser::parseUnion() {
	auto left = parsePrimary();
	skipWhitespace();
	if (match('|')) {
		auto unionType = std::make_unique<UnionType>();
		unionType->types.push_back(std::move(left));
		do {
			skipWhitespace();
			unionType->types.push_back(parsePrimary());
			skipWhitespace();
		} while (match('|'));
		return unionType;
	}
	return left;
}

TypePtr ContractParser::parsePrimary() {
	skipWhitespace();
	if (match('(')) {
		auto type = parseFunction();
		if (!match(')')) throw std::runtime_error("Expected ')'");
		return type;
	}

	if (match('?')) {
		return std::make_unique<NullableType>(parsePrimary());
	}

	if (std::isalpha(peek())) {
		auto id = consumeIdentifier();
		if (id == "list") {
			if (!match('<')) throw std::runtime_error("Expected '<' after list");
			auto inner = parseType();
			if (!match('>')) throw std::runtime_error("Expected '>'");
			return std::make_unique<ListType>(std::move(inner));
		}
		if (id == "any" || id == "env") return std::make_unique<TopType>(id);
		return std::make_unique<ScalarType>(id);
	}

	if (peek() == '.') {
		advance();
		auto cls = consumeIdentifier();
		return std::make_unique<ClassType>(cls);
	}

	auto t = parsePrimary();
	if (match('[')) {
		if (!match(']')) throw std::runtime_error("Expected ']' for vector type");
		return std::make_unique<VectorType>(std::move(t));
	}

	throw std::runtime_error("Unknown type format");
}

TypePtr ContractParser::parseFunction() {
	if (!match('(')) throw std::runtime_error("Expected '(' at start of function type");
	auto args = parseArguments();
	if (!match(')')) throw std::runtime_error("Expected ')' after arguments");
	if (!match('-') || !match('>')) throw std::runtime_error("Expected '->'");

	auto ret = parseType();
	auto f = std::make_unique<FunctionType>();
	f->args = std::move(args);
	f->ret = std::move(ret);
	return f;
}

std::vector<TypePtr> ContractParser::parseArguments() {
	std::vector<TypePtr> args;
	skipWhitespace();
	if (peek() == ')') return args;

	while (true) {
		args.push_back(parseType());
		skipWhitespace();
		if (!match(',')) break;
		skipWhitespace();
	}
	return args;
}


void extractAndParseContracts(ParseNode* node) {
	if (!node) return;

	if (node->token == "COMMENT") {
		const std::string& comment = node->text;
		std::regex contract_regex(R"(#\s*@contract\s+(.*))");
		std::smatch match;
		if (std::regex_match(comment, match, contract_regex)) {
			const std::string contract_text = match[1];
			std::cout << "[contract] " << contract_text << std::endl;

			try {
				ContractParser parser(contract_text);
				std::unique_ptr<Contract> contract = parser.parseContract();
				if (contract) {
					std::cout << "Parsed contract: ";
					contract->print(std::cout);
					std::cout << std::endl;
			} else {
					std::cerr << "Parsed contract is null." << std::endl;
				}
			} catch (const std::exception& e) {
				std::cerr << "Failed to parse contract: " << contract_text << std::endl;
				std::cerr << "  Error: " << e.what() << std::endl;
			}

		}
	}

	for (ParseNode* child : node->children) {
		extractAndParseContracts(child);
	}
}


