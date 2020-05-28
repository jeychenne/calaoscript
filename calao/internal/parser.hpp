/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 27/05/2020                                                                                                *
 *                                                                                                                    *
 * Purpose: the parser analyses a chunk of source code (from a file or a string) and produces an AST which is         *
 * consumed by the compiler.                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_PARSER_HPP
#define CALAO_PARSER_HPP

#include <calao/internal/scanner.hpp>
#include <calao/internal/ast.hpp>

namespace calao {

class Runtime;

class Parser final
{
public:

	explicit Parser(Runtime *rt);

	AutoAst parse_file(const String &path);

	AutoAst do_string(const String &path);

private:

	using Lexeme = Token::Lexeme;

	// Make node with default debug info.
	template <class T, class... Args>
	std::unique_ptr<T> make(Args &&... args)
	{
		return std::make_unique<T>(get_line(), std::forward<Args>(args)...);
	}

	int get_line();

	void initialize();

	// Unconditionally move to the next token.
	void accept();

	// Check the next token's type.
	bool check(Lexeme lex);

	// Move to the next token only if the current token is 'c'. Returns true if we moved.
	bool accept(Lexeme lex);

	void expect(Lexeme lex, const char *hint);

	void expect_separator();

	void report_error(const std::string &hint, const char *error_type = "Syntax");

	AutoAst parse();

	AutoAst parse_statement();

	AutoAst parse_statements();

	AutoAst parse_print_statement();

	AutoAst parse_expression_statement();

	AutoAst parse_expression();

	AutoAst parse_declaration(bool local);

	AutoAst parse_or_expression();

	AutoAst parse_and_expression();

	AutoAst parse_not_expression();

	AutoAst parse_comp_expression();

	AutoAst parse_additive_expression(); // +, -, &

	AutoAst parse_multiplicative_expression(); // *, /, %

	AutoAst parse_signed_expression(); // +x, -x

	AutoAst parse_exponential_expression(); // ^

	AutoAst parse_call_expression();

	AutoAst parse_new_expression();

	AutoAst parse_primary_expression();

	AstList parse_arguments();

	AutoAst parse_identifier(const char *msg);

	AutoAst parse_assertion();

	AutoAst parse_concat_expression(AutoAst e);

	AutoAst parse_if_statement();

	AutoAst parse_if_block();

	AutoAst parse_while_statement();

	// Instance of the scanner (reads one token at a time).
	Scanner scanner;

	// Current token.
	Token token;

	// Pointer to the runtime, for string interning.
	Runtime *runtime;
};

} // namespace calao

#endif // CALAO_PARSER_HPP
