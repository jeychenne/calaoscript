/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 25/05/2020                                                                                                *
 *                                                                                                                    *
 * Purpose: One-pass compiler that parses Calao code and emits bytecode. The design of this compiler is based on Bob  *
 * Nystrom's book __Crafting Interpreters__, available at <http://www.craftinginterpreters.com>.                      *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_COMPILER_HPP
#define CALAO_COMPILER_HPP

#include <functional>
#include <memory>
#include <calao/internal/scanner.hpp>
#include <calao/internal/code.hpp>

namespace calao {

class Runtime;


class Compiler final
{
public:

	Compiler(Runtime *rt);

	std::unique_ptr<Code> do_file(const String &path);

	std::unique_ptr<Code> do_string(const String &text);

private:

	enum class Precedence {
		None,
		Assignment,  // =
		Or,          // or
		And,         // and
		Equality,    // == !=
		Comparison,  // < > <= >=
		Term,        // + - &
		Factor,      // * / %
		Power,       // ^
		Unary,       // not -
		Call,        // . ()
		Primary
	};

	using ParseFunc = std::function<void(bool)>;

	struct ParseRule
	{
		ParseFunc prefix;
		ParseFunc infix;
		Precedence precedence;
	};

	using Lexeme = Token::Lexeme;

	// Unconditionally move to the next token.
	void advance();

	// Check the next token's type.
	bool check(Lexeme lex);

	// Move to the next token only if the current token is 'c'. Returns true if we moved.
	bool accept(Lexeme lex);

	void expect(Lexeme lex);

	void expect_separator();

	void report_error(const std::string &hint, const char *error_type = "Syntax");

	void emit(Instruction i);

	void emit(Opcode op);

	void emit(Opcode op, Instruction i);

	std::unique_ptr<Code> parse();

	void parse_grouping(bool can_assign);

	void parse_expression();

	void parse_unary_expression(bool can_assign);

	void parse_binary_expression(bool can_assign);

	void parse_precedence(Precedence prec);

	void parse_integer(bool can_assign);

	void parse_float(bool can_assign);

	void parse_string(bool can_assign);

	void parse_literal(bool can_assign);

	void parse_statement();

	void parse_print_statement();

	void parse_expression_statement();

	void parse_var_declaration();

	Instruction parse_var_name();

	Instruction add_identifier_constant(const String &ident);

	void define_variable(Instruction global);

	void parse_variable(bool can_assign);

	void parse_named_variable(const String &name, bool can_assign);

	ParseRule *get_rule(Lexeme lex);

	// Instance of the scanner (reads one token at a time).
	Scanner scanner;

	// Current token.
	Token previous_token, token;

	// Current code chunk.
	std::unique_ptr<Code> code;

	// Rules for the Pratt parser.
	std::vector<ParseRule> rules;

	// Pointer to the runtime, for string interning.
	Runtime *runtime;
};

} // namespace calao

#endif // CALAO_COMPILER_HPP
