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

#include <calao/internal/compiler.hpp>
#include <calao/internal/token.hpp>

#define VISIT() CALAO_UNUSED(node); throw error("Cannot compile %", __FUNCTION__);
#define EMIT(...) code->emit(node->line_no, __VA_ARGS__)
#define THROW(...) throw RuntimeError(node->line_no, __VA_ARGS__)

namespace calao {

using Lexeme = Token::Lexeme;

std::shared_ptr<ScriptRoutine> Compiler::compile(AutoAst ast)
{
	initialize();
	ast->visit(*this);
	finalize();

	return std::move(this->routine);
}

void Compiler::initialize()
{
	current_scope = 0;
	routine = std::make_shared<ScriptRoutine>();
	code = &routine->code;
}

void Compiler::finalize()
{
	code->emit_return();
	code = nullptr;
}

void Compiler::open_scope()
{
	++current_scope;
}

void Compiler::close_scope()
{
	--current_scope;
}

void Compiler::visit_constant(ConstantLiteral *node)
{
	switch (node->lex)
	{
		case Lexeme::Null:
			EMIT(Opcode::PushNull);
			break;
		case Lexeme::True:
			EMIT(Opcode::PushBoolean, 1);
			break;
		case Lexeme::False:
			EMIT(Opcode::PushBoolean, 0);
			break;
		case Lexeme::Nan:
			EMIT(Opcode::PushNan);
			break;
		default:
			THROW("[Internal error] Invalid constant in visit_constant()");
	}
}

void Compiler::visit_integer(IntegerLiteral *node)
{
	intptr_t value = node->value;

	// optimize small integers that can fit in an opcode.
	if (value >= (std::numeric_limits<int16_t>::min)() && value <= (std::numeric_limits<int16_t>::max)())
	{
		auto small_int = (int16_t) value;
		EMIT(Opcode::PushSmallInt, (Instruction) small_int);
	}
	else
	{
		auto index = code->add_integer_constant(value);
		EMIT(Opcode::PushInteger, index);
	}
}

void Compiler::visit_float(FloatLiteral *node)
{
	auto index = code->add_float_constant(node->value);
	EMIT(Opcode::PushFloat, index);
}

void Compiler::visit_string(StringLiteral *node)
{
	auto index = code->add_string_constant(std::move(node->value));
	EMIT(Opcode::PushString, index);
}

void Compiler::visit_unary(UnaryExpression *node)
{
	node->expr->visit(*this);

	switch (node->op)
	{
		case Lexeme::Not:
			EMIT(Opcode::Not);
			break;
		case Lexeme::OpMinus:
			EMIT(Opcode::Negate);
			break;
		default:
			THROW("[Internal error] Invalid operator in unary expression");
	}
}

void Compiler::visit_binary(BinaryExpression *node)
{
	node->lhs->visit(*this);
	node->rhs->visit(*this);

	switch (node->op)
	{
		case Lexeme::OpConcat:
			EMIT(Opcode::Concat);
			break;
		case Lexeme::OpPlus:
			EMIT(Opcode::Add);
			break;
		case Lexeme::OpMinus:
			EMIT(Opcode::Subtract);
			break;
		case Lexeme::OpStar:
			EMIT(Opcode::Multiply);
			break;
		case Lexeme::OpSlash:
			EMIT(Opcode::Divide);
			break;
		case Lexeme::OpPower:
			EMIT(Opcode::Power);
			break;
		case Lexeme::OpMod:
			EMIT(Opcode::Modulus);
			break;
		case Lexeme::OpEqual:
			EMIT(Opcode::Equal);
			break;
		case Lexeme::OpNotEqual:
			EMIT(Opcode::NotEqual);
			break;
		case Lexeme::OpLessThan:
			EMIT(Opcode::Less);
			break;
		case Lexeme::OpLessEqual:
			EMIT(Opcode::LessEqual);
			break;
		case Lexeme::OpGreaterThan:
			EMIT(Opcode::Greater);
			break;
		case Lexeme::OpGreaterEqual:
			EMIT(Opcode::GreaterEqual);
			break;
		case Lexeme::OpCompare:
			EMIT(Opcode::Compare);
			break;
		default:
			THROW("[Internal error] Invalid operator in binary expression");
	}
}

void Compiler::visit_statements(StatementList *node)
{
	for (auto &stmt : node->statements) {
		stmt->visit(*this);
	}
}

void Compiler::visit_declaration(Declaration *node)
{
	if (node->lhs.size() != 1 || node->rhs.size() > 1) {
		THROW("Multiple declaration not implemented");
	}
	auto ident = dynamic_cast<Variable*>(node->lhs.front().get());
	if (!ident) {
		THROW("[Syntax error] Expected a variable name in declaration");
	}
	if (node->rhs.empty()) {
		EMIT(Opcode::PushNull);
	}
	else {
		node->rhs.front()->visit(*this);
	}
	auto global = code->add_string_constant(ident->name);
	EMIT(Opcode::DefineGlobal, global);
}

void Compiler::visit_print_statement(PrintStatement *node)
{
	node->expr->visit(*this);
	Opcode op = node->new_line ? Opcode::PrintLine : Opcode::Print;
	EMIT(op);
}

void Compiler::visit_call(CallExpression *node)
{
	VISIT()
}

void Compiler::visit_variable(Variable *node)
{
	auto var = code->add_string_constant(node->name);
	EMIT(Opcode::GetGlobal, var);
}

void Compiler::visit_assignment(Assignment *node)
{
	auto var = dynamic_cast<Variable*>(node->lhs.get());
	if (!var) {
		THROW("[Syntax error] Expected a variable name in assignment");
	}
	node->rhs->visit(*this);
	auto arg = code->add_string_constant(var->name);
	EMIT(Opcode::SetGlobal, arg);
}


} // namespace calao

#undef VISIT
#undef EMIT
#undef THROW