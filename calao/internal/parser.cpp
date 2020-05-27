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
 * Purpose: see header.                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include <iostream>
#include <calao/runtime.hpp>
#include <calao/internal/parser.hpp>

#if 0
#define debug_ast() std::cerr << token.line_no << ": " << __FUNCTION__ << std::endl;
#else
#define debug_ast()
#endif

namespace calao {

Parser::Parser(Runtime *rt) : runtime(rt)
{

}

void Parser::accept()
{
	token = scanner.read_token();
}

void Parser::report_error(const std::string &hint, const char *error_type)
{
	// Move error indicator to the beginning of the current token
	scanner.report_error(hint, token.size(), error_type);
}

void Parser::expect(Lexeme lex, const char *hint)
{
	if (!token.is(lex))
	{
		auto msg = utils::format("expected \"%\" % but got \"%\"", Token::get_name(lex), hint, token.get_name());
		report_error(msg);
	}
	accept();
}

void Parser::expect_separator()
{
	if (token.is(Lexeme::Eot)) return;
	if (token.is_separator()) {
		accept();
	}
	else {
		report_error("Expected a new line or a semicolon");
	}
}

bool Parser::check(Lexeme lex)
{
	return token.is(lex);
}

bool Parser::accept(Lexeme lex)
{
	if (check(lex))
	{
		accept();
		return true;
	}

	return false;
}

AutoAst Parser::parse_file(const String &path)
{
	scanner.load_file(path);
	return parse();
}

AutoAst Parser::do_string(const String &text)
{
	scanner.load_string(text);
	return parse();
}

int Parser::get_line()
{
	return (int) scanner.line_no();
}

void Parser::initialize()
{

}

AutoAst Parser::parse()
{
	initialize();
	accept();
	AstList block;
	while (token.is_separator()) accept();

	while (!accept(Lexeme::Eot))
	{
		block.push_back(parse_statement());
		while (token.is_separator()) accept();
	}

	return make<StatementList>(std::move(block));
}

AutoAst Parser::parse_statement()
{
	debug_ast();

	if (accept(Lexeme::Print))
	{
		return parse_print_statement();
	}
	else if (accept(Lexeme::Var))
	{
		return parse_declaration();
	}
//	else if (accept(Lexeme::Do))
//	{
//		open_scope();
//		parse_block();
//		close_scope();
//	}
	else
	{
		return parse_expression_statement();
	}
}

AutoAst Parser::parse_print_statement()
{
	debug_ast();
	auto e = parse_expression();
	bool add_newline = !accept(Lexeme::Comma);

	return make<PrintStatement>(std::move(e), add_newline);
}

AutoAst Parser::parse_expression_statement()
{
	debug_ast();
	auto e = parse_expression();
	if (check(Lexeme::OpAssign))
	{
		accept();
		return make<Assignment>(std::move(e), parse_expression());
	}
	expect_separator();

	return e;
}

AutoAst Parser::parse_expression()
{
	debug_ast();
	return parse_or_expression();
}

AutoAst Parser::parse_declaration()
{
	debug_ast();
	// rhs may be empty if the variable(s) are declared but not assigned.
	AstList lhs, rhs;
	lhs.push_back(parse_identifier("in variable declaration"));

	// Multiple declaration?
	while (accept(Lexeme::Comma))
	{
		lhs.push_back(parse_identifier("in variable declaration"));
	}

	if (accept(Lexeme::OpAssign))
	{
		rhs.push_back(parse_expression());
	}

	while (accept(Lexeme::Comma))
	{
		rhs.push_back(parse_expression());
	}

	if (! rhs.empty() && lhs.size() != rhs.size()) {
		report_error("Invalid declaration: the number of elements on the left hand side and right and side doesn't match");
	}

	expect_separator();

	return make<Declaration>(std::move(lhs), std::move(rhs));
}

AutoAst Parser::parse_identifier(const char *msg)
{
	debug_ast();
	// This may not be an identifier, but we will throw if that's not the case...
	auto ident = token.spelling;
	expect(Lexeme::Identifier, msg);
	// ... now we are sure that ident is indeed an indentifier.

	return make<Variable>(runtime->intern_string(ident));
}

AutoAst Parser::parse_or_expression()
{
	debug_ast();
	auto e = parse_and_expression();

	if (check(Lexeme::Or))
	{
		accept();
		return make<BinaryExpression>(Lexeme::Or, std::move(e), parse_or_expression());
	}

	return e;
}

AutoAst Parser::parse_and_expression()
{
	debug_ast();
	auto e = parse_not_expression();

	if (check(Lexeme::And))
	{
		accept();
		return make<BinaryExpression>(Lexeme::And, std::move(e), parse_and_expression());
	}

	return e;
}

AutoAst Parser::parse_not_expression()
{
	debug_ast();
	if (check(Lexeme::Not))
	{
		accept();
		return make<UnaryExpression>(Lexeme::Not, parse_comp_expression());
	}

	return parse_comp_expression();
}

AutoAst Parser::parse_comp_expression()
{
	debug_ast();
	auto e = parse_additive_expression();

	while (check(Lexeme::OpEqual) || check(Lexeme::OpNotEqual) || check(Lexeme::OpGreaterEqual) || check(Lexeme::OpGreaterThan) || check(Lexeme::OpLessEqual)
		|| check(Lexeme::OpLessThan) || check(Lexeme::OpCompare))
	{
		auto op = token.id;
		accept();
		e = make<BinaryExpression>(op, std::move(e), parse_additive_expression());
	}

	return e;
}

AutoAst Parser::parse_additive_expression()
{
	debug_ast();
	auto e = parse_multiplicative_expression();

	while (check(Lexeme::OpPlus) || check(Lexeme::OpMinus) || check(Lexeme::OpConcat))
	{
		auto op = token.id;
		accept();
		e = make<BinaryExpression>(op, std::move(e), parse_multiplicative_expression());
	}

	return e;
}

AutoAst Parser::parse_multiplicative_expression()
{
	debug_ast();
	auto e = parse_signed_expression();

	while (check(Lexeme::OpStar) || check(Lexeme::OpSlash) || check(Lexeme::OpMod))
	{
		auto op = token.id;
		accept();
		e = make<BinaryExpression>(op, std::move(e), parse_signed_expression());
	}

	return e;
}

AutoAst Parser::parse_signed_expression()
{
	debug_ast();
	auto e = parse_exponential_expression();

	if (accept(Lexeme::OpMinus))
		return make<UnaryExpression>(Lexeme::OpMinus, std::move(e));

	return e;
}

AutoAst Parser::parse_exponential_expression()
{
	debug_ast();
	auto e = parse_call_expression();

	while (accept(Lexeme::OpPower))
	{
		e = make<BinaryExpression>(Lexeme::OpPower, std::move(e), parse_call_expression());
	}
	return e;
}

AutoAst Parser::parse_call_expression()
{
	debug_ast();
	auto e = parse_new_expression();

	LOOP:
	if (accept(Lexeme::Dot))
	{
		e = make<BinaryExpression>(Lexeme::Dot, std::move(e), parse_identifier("in dot expression"));
		goto LOOP;
	}
	else if (accept(Lexeme::LParen))
	{
		e = make<CallExpression>(std::move(e), parse_arguments());
		goto LOOP;
	}

	return e;
}

AutoAst Parser::parse_new_expression()
{
	debug_ast();
	// TODO: new and function

	return parse_primary_expression();
}

AutoAst Parser::parse_primary_expression()
{
	debug_ast();
	if (check(Lexeme::Identifier))
	{
		auto name = std::move(token.spelling);
		accept();

		return make<Variable>(std::move(name));
	}
	else if (check(Lexeme::StringLiteral))
	{
		auto s = std::move(token.spelling);
		accept();
		return make<StringLiteral>(std::move(s));
	}
	else if (check(Lexeme::IntegerLiteral))
	{
		bool ok;
		intptr_t value = token.spelling.to_int(&ok);
		if (!ok) report_error("Invalid integer");
		accept();
		return make<IntegerLiteral>(value);
	}
	else if (check(Lexeme::FloatLiteral))
	{
		bool ok;
		double value = token.spelling.to_float(&ok);
		if (!ok) report_error("Invalid float number");
		accept();
		return make<FloatLiteral>(value);
	}
	else if (check(Lexeme::True) || check(Lexeme::False))
	{
		auto value = token.id;
		accept();
		return make<ConstantLiteral>(value);
	}
	report_error("Invalid primary expression");

	return nullptr;
}

AstList Parser::parse_arguments()
{
	debug_ast();
	AstList args;

	if (accept(Lexeme::RParen))
	{
		return args;
	}
	args.push_back(parse_expression());

	while (accept(Lexeme::Comma))
	{
		args.push_back(parse_expression());
	}
	expect(Lexeme::RParen, "in argument list");

	return args;
}

} // namespace calao

#undef debug_ast