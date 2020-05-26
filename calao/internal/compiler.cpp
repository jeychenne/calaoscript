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
 * Purpose: see header.                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include <calao/internal/compiler.hpp>
#include <calao/runtime.hpp>

namespace calao {

Compiler::Compiler(Runtime *rt) : runtime(rt)
{
#define FUNC(F) [this](bool can_assign) { F(can_assign); }

	rules = {
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Unknown
		{ ParseFunc(), ParseFunc(), Precedence::None }, // And
		{ ParseFunc(), ParseFunc(), Precedence::None }, // As
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Assert
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Break
		{ ParseFunc(), ParseFunc(), Precedence::None }, // By
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Class
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Continue
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Debug
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Do
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Downto
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Each
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Else
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Elsif
		{ ParseFunc(), ParseFunc(), Precedence::None }, // End
		{ FUNC(parse_literal), ParseFunc(), Precedence::None }, // False
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Field
		{ ParseFunc(), ParseFunc(), Precedence::None }, // For
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Function
		{ ParseFunc(), ParseFunc(), Precedence::None }, // If
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Import
		{ ParseFunc(), ParseFunc(), Precedence::None }, // In
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Inherits
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Method
		{ FUNC(parse_literal), ParseFunc(), Precedence::None }, // Nan
		{ ParseFunc(), ParseFunc(), Precedence::None }, // New
		{ FUNC(parse_unary_expression), ParseFunc(), Precedence::None }, // Not
		{ FUNC(parse_literal), ParseFunc(), Precedence::None }, // Null
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Opt
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Option
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Or
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Print
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Pass
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Ref
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Return
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Super
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Then
		{ ParseFunc(), ParseFunc(), Precedence::None }, // This
		{ ParseFunc(), ParseFunc(), Precedence::None }, // To
		{ FUNC(parse_literal), ParseFunc(), Precedence::None }, // True
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Until
		{ ParseFunc(), ParseFunc(), Precedence::None }, // Var
		{ ParseFunc(), ParseFunc(), Precedence::None }, // While

		{ ParseFunc(), ParseFunc(), Precedence::None }, // OpAssign
		{ ParseFunc(), ParseFunc(), Precedence::None }, // OpCompare
		{ ParseFunc(), FUNC(parse_binary_expression), Precedence::Term }, // OpConcat
		{ ParseFunc(), FUNC(parse_binary_expression), Precedence::Equality }, // OpEqual
		{ ParseFunc(), FUNC(parse_binary_expression), Precedence::Comparison }, // OpGreaterEqual
		{ ParseFunc(), FUNC(parse_binary_expression), Precedence::Comparison }, // OpGreaterThan
		{ ParseFunc(), FUNC(parse_binary_expression), Precedence::Comparison }, // OpLessEqual
		{ ParseFunc(), FUNC(parse_binary_expression), Precedence::Comparison }, // OpLessThan
		{ FUNC(parse_unary_expression), FUNC(parse_binary_expression), Precedence::Term }, // OpMinus
		{ ParseFunc(), FUNC(parse_binary_expression), Precedence::Factor }, // OpMod
		{ ParseFunc(), FUNC(parse_binary_expression), Precedence::Equality }, // OpNotEqual
		{ ParseFunc(), FUNC(parse_binary_expression), Precedence::Term }, // OpPlus
		{ ParseFunc(), FUNC(parse_binary_expression), Precedence::Power }, // OpPower
		{ ParseFunc(), FUNC(parse_binary_expression), Precedence::Factor }, // OpSlash
		{ ParseFunc(), FUNC(parse_binary_expression), Precedence::Factor }, // OpStar

		{ ParseFunc(), ParseFunc(), Precedence::None }, // Comma
		{ ParseFunc(),          ParseFunc(), Precedence::None }, // Colon
		{ ParseFunc(),          ParseFunc(), Precedence::None }, // Dot
		{ FUNC(parse_grouping), ParseFunc(), Precedence::None }, // LParen
		{ ParseFunc(),          ParseFunc(), Precedence::None }, // RParen
		{ ParseFunc(),          ParseFunc(), Precedence::None }, // LCurl
		{ ParseFunc(),          ParseFunc(), Precedence::None }, // RCurl
		{ ParseFunc(),          ParseFunc(), Precedence::None }, // LSquare
		{ ParseFunc(),          ParseFunc(), Precedence::None }, // RSquare
		{ ParseFunc(),          ParseFunc(), Precedence::None }, // Semicolon

		{ FUNC(parse_variable), ParseFunc(), Precedence::None }, // Identifier
		{ FUNC(parse_integer),  ParseFunc(), Precedence::None }, // IntegerLiteral
		{ FUNC(parse_float),    ParseFunc(), Precedence::None }, // FloatLiteral
		{ FUNC(parse_string),   ParseFunc(), Precedence::None }, // StringLiteral

		{ ParseFunc(),          ParseFunc(), Precedence::None }, // Eol
		{ ParseFunc(),          ParseFunc(), Precedence::None }, // Eot
	};
#undef FUNC
}

void Compiler::advance()
{
	previous_token = std::move(token);
	token = scanner.advance();
}

void Compiler::report_error(const std::string &hint, const char *error_type)
{
	// Move error indicator to the beginning of the current token
	scanner.report_error(hint, token.size(), error_type);
}

void Compiler::expect(Lexeme lex)
{
	if (!token.is(lex))
	{
		auto msg = utils::format("expected \"%\" but got \"%\"", Token::get_name(lex), token.get_name());
		report_error(msg);
	}
	advance();
}

void Compiler::expect_separator()
{
	if (token.is_separator()) {
		advance();
	}
	else {
		report_error("Expected a new line or a semicolon");
	}
}

bool Compiler::check(Lexeme lex)
{
	return token.is(lex);
}

bool Compiler::accept(Lexeme lex)
{
	if (check(lex))
	{
		advance();
		return true;
	}

	return false;
}

std::unique_ptr<Code> Compiler::do_file(const String &path)
{
	scanner.load_file(path);
	return parse();
}

std::unique_ptr<Code> Compiler::do_string(const String &text)
{
	scanner.load_string(text);
	return parse();
}

void Compiler::emit(Instruction i)
{
	code->emit(previous_token.line_no, i);
}

void Compiler::emit(Opcode op)
{
	code->emit(previous_token.line_no, op);
}

void Compiler::emit(Opcode op, Instruction i)
{
	code->emit(previous_token.line_no, op, i);
}

std::unique_ptr<Code> Compiler::parse()
{
	code = std::make_unique<Code>();
	advance();

	while (!accept(Lexeme::Eot)) {
		parse_statement();
	}
	emit(Opcode::Return);

	return std::move(code);
}

void Compiler::parse_statement()
{
	if (accept(Lexeme::Print))
	{
		parse_print_statement();
	}
	else if (accept(Lexeme::Var))
	{
		parse_var_declaration();
	}
	else if (token.is_separator())
	{
		advance(); // blank line or empty statement
	}
	else
	{
		parse_expression_statement();
	}
}

void Compiler::parse_grouping(bool)
{
	parse_expression();
	expect(Lexeme::RParen);
}

void Compiler::parse_expression()
{
	parse_precedence(Precedence::Assignment);
}

void Compiler::parse_unary_expression(bool)
{
	auto op = previous_token.id;
	parse_precedence(Precedence::Unary);

	switch (op)
	{
		case Lexeme::Not:
		{
			emit(Opcode::Not);
			break;
		}
		case Lexeme::OpMinus:
		{
			emit(Opcode::Negate);
			break;
		}
		default:
			break;
	}
}

void Compiler::parse_precedence(Compiler::Precedence prec)
{
	advance();
	auto &prefix_rule = get_rule(previous_token.id)->prefix;

	if (! prefix_rule) {
		report_error("Expected an expression");
	}
	bool can_assign = (prec <= Precedence::Assignment);
	prefix_rule(can_assign);

	while (prec <= get_rule(token.id)->precedence)
	{
		advance();
		auto &infix_rule = get_rule(previous_token.id)->infix;
		assert(infix_rule);
		infix_rule(can_assign);
	}

	if (can_assign && accept(Lexeme::OpAssign)) {
		report_error("Invalid assignment target");
	}
}

void Compiler::parse_binary_expression(bool)
{
	auto op = previous_token.id;

	// Compile the right hand operand.
	auto rule = get_rule(op);
	auto prec = Precedence(static_cast<int>(rule->precedence) + 1);
	parse_precedence(prec);

	// Emit the operator instruction.
	switch (op)
	{
		case Lexeme::OpConcat:
		{
			emit(Opcode::Concat);
			break;
		}
		case Lexeme::OpEqual:
		{
			emit(Opcode::Equal);
			break;
		}
		case Lexeme::OpGreaterEqual:
		{
			emit(Opcode::GreaterEqual);
			break;
		}
		case Lexeme::OpGreaterThan:
		{
			emit(Opcode::Greater);
			break;
		}
		case Lexeme::OpLessEqual:
		{
			emit(Opcode::Less);
			break;
		}
		case Lexeme::OpLessThan:
		{
			emit(Opcode::LessEqual);
			break;
		}
		case Lexeme::OpMinus:
		{
			emit(Opcode::Subtract);
			break;
		}
		case Lexeme::OpMod:
		{
			emit(Opcode::Modulus);
			break;
		}
		case Lexeme::OpNotEqual:
		{
			emit(Opcode::NotEqual);
			break;
		}
		case Lexeme::OpPlus:
		{
			emit(Opcode::Add);
			break;
		}
		case Lexeme::OpPower:
		{
			emit(Opcode::Power);
			break;
		}
		case Lexeme::OpSlash:
		{
			emit(Opcode::Divide);
			break;
		}
		case Lexeme::OpStar:
		{
			emit(Opcode::Multiply);
			break;
		}
		default:
			break;
  }
}

void Compiler::parse_integer(bool)
{
	bool ok = false;
	intptr_t value = previous_token.spelling.to_int(&ok);
	if (!ok) {
		report_error("Invalid Integer literal");
	}

	// optimize small integers that can fit in an opcode.
	if (value >= (std::numeric_limits<int16_t>::min)() && value <= (std::numeric_limits<int16_t>::max)())
	{
		auto small_int = (int16_t) value;
		emit(Opcode::PushSmallInt, (Instruction) small_int);
	}
	else
	{
		auto index = code->add_integer_constant(value);
		emit(Opcode::PushInteger, index);
	}
}

void Compiler::parse_float(bool)
{
	bool ok = false;
	double value = previous_token.spelling.to_float(&ok);

	if (!ok) {
		report_error("Invalid Float literal");
	}

	auto index = code->add_float_constant(value);
	emit(Opcode::PushFloat, index);
}

void Compiler::parse_string(bool)
{
	auto s = runtime->intern_string(previous_token.spelling);
	auto index = code->add_string_constant(std::move(s));
	emit(Opcode::PushString, index);
}

Compiler::ParseRule *Compiler::get_rule(Compiler::Lexeme lex)
{
	return &rules[static_cast<int>(lex)];
}

void Compiler::parse_literal(bool)
{
	switch (previous_token.id)
	{
		case Lexeme::False:
		{
			emit(Opcode::PushFalse);
			break;
		}
		case Lexeme::Nan:
		{
			emit(Opcode::PushNan);
			break;
		}
		case Lexeme::Null:
		{
			emit(Opcode::PushNull);
			break;
		}
		case Lexeme::True:
		{
			emit(Opcode::PushTrue);
			break;
		}
		default:
			break;
	}
}

void Compiler::parse_print_statement()
{
	parse_expression();
	emit(Opcode::Print);
	expect_separator();
}

void Compiler::parse_expression_statement()
{
	parse_expression();
	// FIXME: I don't think we want to pop expressions off the stack. We can clean up the stack when we finalize a stack frame.
	//  But if we do, we need to make sure other operations don't pop the value beforehand.
	//emit(Opcode::Pop);
	expect_separator();
}

void Compiler::parse_var_declaration()
{
	auto global = parse_var_name();

	if (accept(Lexeme::OpAssign))
	{
		parse_expression();
	}
	else
	{
		emit(Opcode::PushNull);
	}
	define_variable(global);
	expect_separator();
}

Instruction Compiler::parse_var_name()
{
	expect(Lexeme::Identifier);
	return add_identifier_constant(previous_token.spelling);
}

Instruction Compiler::add_identifier_constant(const String &ident)
{
	auto s = runtime->intern_string(ident);
	return code->add_string_constant(s);
}

void Compiler::define_variable(Instruction global)
{
	emit(Opcode::DefineGlobal, global);
}

void Compiler::parse_variable(bool can_assign)
{
	parse_named_variable(previous_token.spelling, can_assign);
}

void Compiler::parse_named_variable(const String &name, bool can_assign)
{
	auto arg = add_identifier_constant(name);

	if (can_assign && accept(Lexeme::OpAssign))
	{
		parse_expression();
		emit(Opcode::SetGlobal, arg);
	}
	else
	{
		emit(Opcode::GetGlobal, arg);
	}
}


} // namespace calao