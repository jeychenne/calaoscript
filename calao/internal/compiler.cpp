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

std::shared_ptr<Routine> Compiler::compile(AutoAst ast)
{
	initialize();
	code->emit(ast->line_no, Opcode::NewFrame, 0);
	int offset = code->get_backpatch_offset();
	int previous_scope = open_scope(); // open module
	ast->visit(*this);
	close_scope(previous_scope);
	// Fix number of locals.
	code->backpatch(offset, routine->local_count());
	finalize();

	return std::move(this->routine);
}

void Compiler::initialize()
{
	scope_id = 0;
	current_scope = 0;
	routine = std::make_shared<Routine>();
	code = &routine->code;
}

void Compiler::finalize()
{
	code->emit_return();
	code = nullptr;
}

int Compiler::open_scope()
{
	int previous = current_scope;
	current_scope = ++scope_id;
	++scope_depth;

	return previous;
}

void Compiler::close_scope(int previous)
{
	--scope_depth;
	current_scope = previous;
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
		auto index = routine->add_integer_constant(value);
		EMIT(Opcode::PushInteger, index);
	}
}

void Compiler::visit_float(FloatLiteral *node)
{
	auto index = routine->add_float_constant(node->value);
	EMIT(Opcode::PushFloat, index);
}

void Compiler::visit_string(StringLiteral *node)
{
	auto index = routine->add_string_constant(std::move(node->value));
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
	int scope;
	if (node->open_scope) scope = open_scope();

	for (auto &stmt : node->statements) {
		stmt->visit(*this);
	}
	if (node->open_scope) close_scope(scope);
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

	if (node->local || scope_depth > 1)
	{
		try
		{
			auto index = routine->add_local(ident->name, current_scope, scope_depth);
			EMIT(Opcode::DefineLocal, index);
		}
		catch (std::runtime_error &e)
		{
			THROW(e.what());
		}
	}
	else
	{
		auto index = routine->add_string_constant(ident->name);
		EMIT(Opcode::DefineGlobal, index);
	}
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
	// Try to find a local variable, otherwise try to get a global.
	auto index = routine->find_local(node->name, current_scope);
	if (index)
	{
		EMIT(Opcode::GetLocal, *index);
	}
	else
	{
		auto var = routine->add_string_constant(node->name);
		EMIT(Opcode::GetGlobal, var);
	}
}

void Compiler::visit_assignment(Assignment *node)
{
	auto var = dynamic_cast<Variable*>(node->lhs.get());
	if (!var) {
		THROW("[Syntax error] Expected a variable name in assignment");
	}
	node->rhs->visit(*this);

	// Try to find a local variable, otherwise try to get a global.
	auto index = routine->find_local(var->name, current_scope);
	if (index)
	{
		EMIT(Opcode::SetLocal, *index);
	}
	else
	{
		auto arg = routine->add_string_constant(var->name);
		EMIT(Opcode::SetGlobal, arg);
	}
}

std::pair<int, int> Compiler::get_scope() const
{
	return { current_scope, scope_depth };
}

void Compiler::visit_assert_statement(AssertStatement *node)
{
	Instruction narg = (node->msg == nullptr) ? 1 : 2;
	node->expr->visit(*this);
	if (narg == 2) node->msg->visit(*this);
	EMIT(Opcode::Assert, narg);
}

void Compiler::visit_concat_expression(ConcatExpression *node)
{
	for (auto &e : node->list) {
		e->visit(*this);
	}
	EMIT(Opcode::Concat, Instruction(node->list.size()));
}


} // namespace calao

#undef VISIT
#undef EMIT
#undef THROW