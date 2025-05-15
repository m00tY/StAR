#ifndef GENSOURCE_H
#define GENSOURCE_H

#include <string>
#include <vector>
#include <functional>

struct StatementRange {
    size_t start;
    size_t end;
};

std::vector<StatementRange> extractStatements(const std::vector<ParseNode*> nodes);

std::vector<std::string> getStatementStrings(const std::vector<ParseNode*> nodes, std::vector<StatementRange> ranges);

std::string generateTypeCheck(const std::string& argName, const std::string& typeName);

/*
class Argument {
	private:
		std::string name;
		std::string type;
	
	public:
		Argument(std::string name, std::string type)
			: name(std::move(name)), type(std::move(type)) {}
	
		static Argument fromAST(const std::vector<std::string>& lines);
	
		const std::string& getName() const { return name; }
		const std::string& getType() const { return type; }
	};

	class FunctionDeclaration {
		private:
			std::string name;
			std::vector<Argument> arguments;
			std::vector<std::string> bodyLines;
		
		public:
			FunctionDeclaration(std::string name, std::vector<Argument> args,
								std::vector<std::string> body)
				: name(std::move(name)), arguments(std::move(args)), bodyLines(std::move(body)) {}
		
			static FunctionDeclaration fromAST(const std::vector<std::string>& lines);
			
			const std::string& getName() const { return name; }
			const std::vector<Argument>& getArguments() const { return arguments; }
			const std::vector<std::string>& getBody() const { return bodyLines; }
		};

class TemplateRenderer {
public:
	std::string renderFunctionDeclaration(const FunctionDeclaration& func);	

private:
	Queue<std::function<std::string()>> renderQueue;
};
*/

#endif
