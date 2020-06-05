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
#define trace_ast() std::cerr << token.line_no << ": " << __FUNCTION__ << std::endl;
#else
#define trace_ast()
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
	return (int) token.line_no;
}

void Parser::initialize()
{

}

AutoAst Parser::parse()
{
	initialize();
	accept();
	auto line = get_line();
	AstList block;
	while (token.is_separator()) accept();

	while (!check(Lexeme::Eot))
	{
		block.push_back(parse_statement());
		while (token.is_separator()) accept();
	}
	expect(Lexeme::Eot, "at end of file");
	return std::make_unique<StatementList>(line, std::move(block));
}

AutoAst Parser::parse_statement()
{
	trace_ast();

	if (accept(Lexeme::Print))
	{
		return parse_print_statement();
	}
	if (accept(Lexeme::Local))
	{
		while (accept(Lexeme::Eol)) { }
		if (accept(Lexeme::Function)) {
			return parse_function_declaration(true);
		}
		// "var" is optional after "local".
		accept(Lexeme::Var);

		return parse_declaration(true);
	}
	else if (accept(Lexeme::Var))
	{
		return parse_declaration(false);
	}
	else if (accept(Lexeme::If))
	{
		return parse_if_statement();
	}
	else if (accept(Lexeme::While))
	{
		return parse_while_statement();
	}
	else if (accept(Lexeme::For))
	{
		return parse_for_statement();
	}
	else if (accept(Lexeme::Foreach))
	{
		return parse_foreach_statement();
	}
	else if (accept(Lexeme::Function))
	{
		return parse_function_declaration(false);
	}
	else if (accept(Lexeme::Return))
	{
		return parse_return_statement();
	}
	else if (accept(Lexeme::Break))
	{
		return make<LoopExitStatement>(Lexeme::Break);
	}
	else if (accept(Lexeme::Continue))
	{
		return make<LoopExitStatement>(Lexeme::Continue);
	}
	else if (accept(Lexeme::Assert))
	{
		return parse_assertion();
	}
	else if (accept(Lexeme::Do))
	{
		return parse_statements(true);
	}
	else if (accept(Lexeme::Pass))
	{
		// Stuff this in constant literal to avoid creating a new AST node. This is a no-op anyway.
		return make<ConstantLiteral>(Lexeme::Pass);
	}
	else
	{
		return parse_expression_statement();
	}
}

AutoAst Parser::parse_statements(bool open_scope)
{
	trace_ast();
	AstList block;
	auto line = get_line();
	while (token.is_separator()) { accept(); }

	while (!check(Lexeme::End))
	{
		block.push_back(parse_statement());
		while (token.is_separator()) accept();
	}
	accept(Lexeme::End);

	return std::make_unique<StatementList>(line, std::move(block), open_scope);
}

AutoAst Parser::parse_if_block()
{
	trace_ast();
	AstList block;
	auto line = get_line();
	while (token.is_separator()) { accept(); }

	while (!check(Lexeme::End) && !check(Lexeme::Elsif) && !check(Lexeme::Else))
	{
		block.push_back(parse_statement());
		while (token.is_separator()) accept();
	}
	accept(Lexeme::End);

	return std::make_unique<StatementList>(line, std::move(block), true);
}

AutoAst Parser::parse_print_statement()
{
	trace_ast();
	AstList lst;
	bool add_newline = true;
	lst.push_back(parse_expression());
	while (accept(Lexeme::Comma))
	{
		if (accept(Lexeme::Eol))
		{
			add_newline = false;
			break;
		}
		lst.push_back(parse_expression());
	}

	return make<PrintStatement>(std::move(lst), add_newline);
}

AutoAst Parser::parse_expression_statement()
{
	trace_ast();
	auto e = parse_expression();
	if (check(Lexeme::OpAssign))
	{
		accept();
		return make<Assignment>(std::move(e), parse_expression());
	}

	return e;
}

AutoAst Parser::parse_expression()
{
	trace_ast();
	return parse_conditional_expression();
}

AutoAst Parser::parse_declaration(bool local)
{
	trace_ast();
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

	return make<Declaration>(std::move(lhs), std::move(rhs), local);
}

AutoAst Parser::parse_identifier(const char *msg)
{
	trace_ast();
	// This may not be an identifier, but we will throw if that's not the case...
	auto ident = token.spelling;
	expect(Lexeme::Identifier, msg);
	// ... now we are sure that ident is indeed an indentifier.

	return make<Variable>(runtime->intern_string(ident));
}

AutoAst Parser::parse_or_expression()
{
	trace_ast();
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
	trace_ast();
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
	trace_ast();
	if (accept(Lexeme::Not))
	{
		return make<UnaryExpression>(Lexeme::Not, parse_comp_expression());
	}

	return parse_comp_expression();
}

AutoAst Parser::parse_comp_expression()
{
	trace_ast();
	auto e = parse_additive_expression();

	if (check(Lexeme::OpEqual) || check(Lexeme::OpNotEqual) || check(Lexeme::OpGreaterEqual) || check(Lexeme::OpGreaterThan) || check(Lexeme::OpLessEqual)
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
	trace_ast();
	auto e = parse_multiplicative_expression();

	if (accept(Lexeme::OpConcat)) {
		return parse_concat_expression(std::move(e));
	}
	while (check(Lexeme::OpPlus) || check(Lexeme::OpMinus))
	{
		auto op = token.id;
		accept();
		e = make<BinaryExpression>(op, std::move(e), parse_multiplicative_expression());
	}

	return e;
}

AutoAst Parser::parse_multiplicative_expression()
{
	trace_ast();
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
	trace_ast();
	if (accept(Lexeme::OpMinus))
		return make<UnaryExpression>(Lexeme::OpMinus, parse_exponential_expression());

	return parse_exponential_expression();
}

AutoAst Parser::parse_exponential_expression()
{
	trace_ast();
	auto e = parse_call_expression();

	while (accept(Lexeme::OpPower))
	{
		e = make<BinaryExpression>(Lexeme::OpPower, std::move(e), parse_call_expression());
	}
	return e;
}

AutoAst Parser::parse_call_expression()
{
	trace_ast();
	auto e = parse_new_expression();

	LOOP:
	if (accept(Lexeme::Dot))
	{
		e = make<BinaryExpression>(Lexeme::Dot, std::move(e), parse_identifier("in dot expression"));
		goto LOOP;
	}
	else if (accept(Lexeme::LSquare))
	{
		auto e2 = parse_expression();
		expect(Lexeme::RSquare, "in index");
		e = make<BinaryExpression>(Lexeme::LSquare, std::move(e), std::move(e2));
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
	trace_ast();
	// TODO: new and function
	if (accept(Lexeme::Ref)) {
		return make<ReferenceExpression>(parse_expression());
	}

	return parse_primary_expression();
}

AutoAst Parser::parse_primary_expression()
{
	trace_ast();
	if (check(Lexeme::Identifier))
	{
		auto name = std::move(token.spelling);
		accept();

		return make<Variable>(std::move(name));
	}
	else if (check(Lexeme::StringLiteral))
	{
		auto s = runtime->intern_string(token.spelling);
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
	else if (check(Lexeme::True) || check(Lexeme::False) || check(Lexeme::Null) || check(Lexeme::Nan))
	{
		auto value = token.id;
		accept();
		return make<ConstantLiteral>(value);
	}
	else if (accept(Lexeme::LSquare))
	{
		return parse_list_literal();
	}
//	else if (accept(Lexeme::OpAt))
//	{
//		expect(Lexeme::LSquare, "in array literal");
//		return parse_array_literal();
//	}
	else if (accept(Lexeme::LCurl))
	{
		return parse_table_literal();
	}
	else if (accept(Lexeme::LParen))
	{
		auto e = parse_expression();
		expect(Lexeme::RParen, "in parenthesized expression");
		return e;
	}

	report_error("Invalid primary expression");
	return nullptr;
}

AstList Parser::parse_arguments()
{
	trace_ast();
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

AstList Parser::parse_parameters()
{
	trace_ast();
	AstList params;

	if (accept(Lexeme::RParen))
	{
		return params;
	}
	params.push_back(parse_parameter());

	while (accept(Lexeme::Comma))
	{
		params.push_back(parse_parameter());
	}
	expect(Lexeme::RParen, "in parameter list");

	return params;
}

AutoAst Parser::parse_parameter()
{
	bool by_ref = accept(Lexeme::Ref);
	auto var = parse_identifier("in parameter list");
	AutoAst type;
	if (accept(Lexeme::As)) {
		type = parse_expression();
	}

	return make<RoutineParameter>(std::move(var), std::move(type), by_ref);
}

AutoAst Parser::parse_assertion()
{
	auto e = parse_expression();
	AutoAst msg;

	if (accept(Lexeme::Comma))
	{
		msg = parse_expression();
	}

	return make<AssertStatement>(std::move(e), std::move(msg));
}

AutoAst Parser::parse_concat_expression(AutoAst e)
{
	AstList lst;
	lst.push_back(std::move(e));
	lst.push_back(parse_multiplicative_expression());

	while (accept(Lexeme::OpConcat)) {
		lst.push_back(parse_multiplicative_expression());
	}

	return make<ConcatExpression>(std::move(lst));
}

AutoAst Parser::parse_if_statement()
{
	AstList ifs;
	AutoAst else_block;
	auto line = get_line();
	auto e = parse_expression();
	expect(Lexeme::Then, "in \"if\" statement");
	auto block = parse_if_block();
	ifs.push_back(make<IfCondition>(std::move(e), std::move(block)));

	while (accept(Lexeme::Elsif))
	{
		e = parse_expression();
		expect(Lexeme::Then, "in \"elsif\" condition");
		block = parse_if_block();
		ifs.push_back(make<IfCondition>(std::move(e), std::move(block)));
	}
	if (accept(Lexeme::Else)) {
		else_block = parse_if_block();
	}

	return std::make_unique<IfStatement>(line, std::move(ifs), std::move(else_block));
}

AutoAst Parser::parse_while_statement()
{
	auto line = get_line();
	auto e = parse_expression();
	expect(Lexeme::Do, "in while statement");
	auto block = parse_statements(true);

	return std::make_unique<WhileStatement>(line, std::move(e), std::move(block));
}

AutoAst Parser::parse_for_statement()
{
	constexpr const char *hint = "in for loop";
	auto line = get_line();
	AutoAst e1, e2, e3;
	bool down = false;
	// var keyword is optional
	accept(Lexeme::Var);
	auto var = parse_identifier(hint);
	expect(Lexeme::OpAssign, hint);
	e1 = parse_expression();

	if (accept(Lexeme::To))
	{
		e2 = parse_expression();
	}
	else if (accept(Lexeme::Downto))
	{
		e2 = parse_expression();
		down = true;
	}
	else
	{
		report_error("Expected \"to\" or \"downto\" in for loop");
	}

	if (accept(Lexeme::Step)) {
		e3 = parse_expression();
	}
	expect(Lexeme::Do, hint);
	// Don't open a scope for the block: we will open it ourselves so that we can include the loop variable in it.
	auto block = parse_statements(false);

	return std::make_unique<ForStatement>(line, std::move(var), std::move(e1), std::move(e2), std::move(e3), std::move(block), down);
}

AutoAst Parser::parse_foreach_statement()
{
	constexpr const char *hint = "in foreach loop";
	auto line = get_line();
	auto key = parse_identifier(hint);
	AutoAst val;

	if (accept(Lexeme::Comma))
	{
		if (accept(Lexeme::Ref)) {
			val = make<ReferenceExpression>(parse_identifier(hint));
		}
		else {
			val = parse_identifier(hint);
		}
	}
	else
	{
		std::swap(key, val);
	}
	expect(Lexeme::In, hint);
	auto coll = parse_expression();
	// We need a reference, which will be grabbed by the iterator.
	if (!dynamic_cast<ReferenceExpression*>(coll.get())) {
		coll = make<ReferenceExpression>(std::move(coll));
	}
	expect(Lexeme::Do, hint);
	// Don't open a scope for the block: we will open it ourselves so that we can include the loop variable in it.
	auto block = parse_statements(false);

	return std::make_unique<ForeachStatement>(line, std::move(key), std::move(val), std::move(coll), std::move(block));
}

AutoAst Parser::parse_conditional_expression()
{
	trace_ast();
	auto e = parse_or_expression();
	if (accept(Lexeme::If))
	{

		AstList if_cond;
		if_cond.push_back(make<IfCondition>(parse_expression(), std::move(e)));
		expect(Lexeme::Else, "in conditional expression");
		auto else_block = parse_expression();
		e = make<IfStatement>(std::move(if_cond), std::move(else_block));
	}

	return e;
}

AutoAst Parser::parse_function_declaration(bool local)
{
	trace_ast();
	int line = get_line();
	constexpr const char *hint = "in function declaration";
	auto name = parse_identifier(hint);
	expect(Lexeme::LParen, hint);
	auto params = parse_parameters();
	// Don't open a scope for the block: the function will do it so that we include the parameters in the scope.
	auto body = parse_statements(false);

	return std::make_unique<RoutineDefinition>(line, std::move(name), std::move(params), std::move(body), local, false);
}

AutoAst Parser::parse_return_statement()
{
	AutoAst e;

	if (!token.is_separator())
	{
		e = parse_expression();
	}

	return make<ReturnStatement>(std::move(e));
}

AutoAst Parser::parse_member_expression()
{
	auto e = parse_new_expression();

	while (accept(Lexeme::Dot))
	{
		e = make<BinaryExpression>(Lexeme::Dot, std::move(e), parse_identifier("in member expression"));
	}

	return e;
}

AutoAst Parser::parse_list_literal()
{
	auto line = get_line();
	skip_empty_lines();
	if (accept(Lexeme::RSquare)) {
		return make<ListLiteral>(AstList());
	}
	AstList items;
	items.push_back(parse_expression());
	skip_empty_lines();

	while (accept(Lexeme::Comma))
	{
		skip_empty_lines();
		items.push_back(parse_expression());
	}
	skip_empty_lines();
	expect(Lexeme::RSquare, "at the end of list or array literal");

	return std::make_unique<ListLiteral>(line, std::move(items));
}

AutoAst Parser::parse_table_literal()
{
	constexpr const char *hint = "in table literal";
	auto line = get_line();
	skip_empty_lines();
	if (accept(Lexeme::RCurl)) {
		return make<TableLiteral>(AstList(), AstList());
	}
	AstList keys, values;
	keys.push_back(parse_expression());
	expect(Lexeme::Colon, hint);
	values.push_back(parse_expression());

	while (accept(Lexeme::Comma))
	{
		skip_empty_lines();
		keys.push_back(parse_expression());
		expect(Lexeme::Colon, hint);
		values.push_back(parse_expression());
	}
	skip_empty_lines();
	expect(Lexeme::RCurl, hint);

	return std::make_unique<TableLiteral>(line, std::move(keys), std::move(values));
}

} // namespace calao

#undef trace_ast