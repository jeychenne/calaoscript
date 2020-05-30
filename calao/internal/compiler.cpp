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

#include <cfenv>
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
	int offset = code->get_current_offset() - 1;
	int previous_scope = open_scope(); // open module
	ast->visit(*this);
	close_scope(previous_scope);
	// Fix number of locals.
	code->backpatch_instruction(offset, (Instruction)routine->local_count());
	finalize();

	return std::move(this->routine);
}

void Compiler::initialize()
{
	scope_id = 0;
	current_scope = 0;
	set_routine(std::make_shared<Routine>(String()));
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
		// Not a constant, but we put it here to avoid creating a new node.
		case Lexeme::Pass:
			break; // no-op
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
	bool noop = false;

	// Convert negative numeric literals in place.
	if (node->op == Lexeme::OpMinus)
	{
		if (node->expr->is<FloatLiteral>())
		{
			auto e = static_cast<FloatLiteral*>(node->expr.get());
			std::feclearexcept(FE_ALL_EXCEPT);
			e->value = -e->value;
			if (fetestexcept(FE_OVERFLOW | FE_UNDERFLOW)) {
				throw RuntimeError(node->line_no, "[Math error] Invalid negative float literal");
			}
			noop = true;
		}
		else if (node->expr->is<IntegerLiteral>())
		{
			auto e = static_cast<IntegerLiteral*>(node->expr.get());
			if (e->value == (std::numeric_limits<intptr_t>::max)()) {
				throw RuntimeError(node->line_no, "[Math error] Invalid negative integer literal");
			}
			e->value = -e->value;
			noop = true;
		}
	}
	node->expr->visit(*this);

	switch (node->op)
	{
		case Lexeme::Not:
			EMIT(Opcode::Not);
			break;
		case Lexeme::OpMinus:
			if (!noop) EMIT(Opcode::Negate);
			break;
		default:
			THROW("[Internal error] Invalid operator in unary expression");
	}
}

void Compiler::visit_binary(BinaryExpression *node)
{
	// Handle AND and OR.
	if (node->op == Lexeme::And)
	{
		// Don't evaluate rhs if lhs is false.
		node->lhs->visit(*this);
		auto jmp = code->emit_jump(node->line_no, Opcode::JumpFalse);
		node->rhs->visit(*this);
		code->backpatch(jmp);
		return;
	}
	if (node->op == Lexeme::Or)
	{
		// Don't evaluate rhs if lhs is true.
		node->lhs->visit(*this);
		auto jmp = code->emit_jump(node->line_no, Opcode::JumpTrue);
		node->rhs->visit(*this);
		code->backpatch(jmp);
		return;
	}

	// Handle other operators.
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
			auto index = add_local(ident->name);
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
	// First push the arguments.
	for (auto &arg : node->args) {
		arg->visit(*this);
	}

	// Next push the function and emit the call.
	node->expr->visit(*this);
	EMIT(Opcode::Call, Instruction(node->args.size()));
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

void Compiler::visit_if_condition(IfCondition *node)
{
    node->cond->visit(*this);
    // Jump to the next branch (we will need to backpatch)
    node->conditional_jump = code->emit_jump(node->line_no, Opcode::JumpFalse);
    // Compile TRUE case
    node->block->visit(*this);
}

void Compiler::visit_if_statement(IfStatement *node)
{
	/*
	  Given a block such as:

	  if x < 0 then
		print "-"
	  else
		print "+"
	  end

	  We generate the following opcodes (simplified):

	  i01: PUSH x
	  i02: PUSH 0
	  i03: LESS_THAN
	  i04: JUMP_FALSE i08  ; jump to FALSE case if condition not satisfied
	  i05: PUSH "-"  ; TRUE case
	  i06: PRINT
	  i07: JUMP i10  ; skip FALSE case
	  i08: PUSH "+"  ; FALSE case
	  i09: PRINT
	  i10: END      ; end of the block

	  We need to backpatch forward addresses for the jumps since they
	  are not known when the code is compiled. Elsif branches work just
	  like a sequence of if branches.
	*/
	for (auto &stmt : node->if_conds)
	{
		auto if_cond = static_cast<IfCondition*>(stmt.get());
		if_cond->visit(*this);
		if_cond->unconditional_jump = code->emit_jump(node->line_no, Opcode::Jump);
		// Now we are at the beginning of the next branch, we can backpatch JumpFalse
		code->backpatch(if_cond->conditional_jump);
	}

	// Compile the else branch
	if (node->else_block) {
		node->else_block->visit(*this);
	}

	// We can now backpatch the jump in i07 with i10
	for (auto &stmt : node->if_conds)
	{
		auto if_cond = static_cast<IfCondition*>(stmt.get());
		code->backpatch(if_cond->unconditional_jump);
	}
}

void Compiler::visit_while_statement(WhileStatement *node)
{
	int previous_break_count = break_count;
	break_count = 0;
	int loop_start = code->get_current_offset();
	node->cond->visit(*this);
	int exit_jump = code->emit_jump(node->line_no, Opcode::JumpFalse);
	node->block->visit(*this);
	code->emit_jump(node->line_no, Opcode::Jump, loop_start);
	code->backpatch(exit_jump);
	backpatch_breaks(previous_break_count);
}

void Compiler::visit_for_statement(ForStatement *node)
{
	auto scope = open_scope();
	int previous_break_count = break_count;
	int previous_continue_count = continue_count;
	break_count = continue_count = 0;

	// Initialize loop variable
	auto ident = dynamic_cast<Variable*>(node->var.get());
	node->start->visit(*this);
	auto var_index = add_local(ident->name);
	EMIT(Opcode::DefineLocal, var_index);

	// Evaluate end condition once and store it in a hidden variable.
	node->end->visit(*this);
	auto end_index = add_local("$end");
	EMIT(Opcode::DefineLocal, end_index);

	// Evaluate step condition if it was provided and store it in a hidden variable;
	// otherwise, we use special-purpose instructions to increment/decrement the counter.
	Instruction step_index = (std::numeric_limits<Instruction>::max)();

	if (node->step)
	{
		node->step->visit(*this);
		step_index = add_local("$step");
		EMIT(Opcode::DefineLocal, step_index);
	}

	// The loop starts here.
	int loop_start = code->get_current_offset();

	// End condition.
	EMIT(Opcode::GetLocal, var_index);
	EMIT(Opcode::GetLocal, end_index);
	auto cmp = node->down ? Opcode::Less : Opcode::Greater;
	EMIT(cmp);
	int jump_end = code->emit_jump(node->line_no, Opcode::JumpTrue);

	// Compile block
	node->block->visit(*this);

	backpatch_continues(previous_continue_count);
	// Update counter
	if (node->step)
	{
		EMIT(Opcode::GetLocal, var_index);
		EMIT(Opcode::GetLocal, step_index);
		auto op = node->down ? Opcode::Subtract : Opcode::Add;
		EMIT(op);
		EMIT(Opcode::SetLocal, var_index);
	}
	else
	{
		auto op = node->down ? Opcode::DecrementLocal : Opcode::IncrementLocal;
		EMIT(op, var_index);
	}

	// Go back to the beginning of the loop.
	code->emit_jump(node->line_no, Opcode::Jump, loop_start);
	code->backpatch(jump_end);
	backpatch_breaks(previous_break_count);

	close_scope(scope);
}

void Compiler::backpatch_breaks(int previous)
{
	for (int i = 0; i < break_count; i++)
	{
		int addr = break_jumps.back();
		break_jumps.pop_back();
		code->backpatch(addr);
	}
	break_count = previous;
}

void Compiler::visit_loop_exit(LoopExitStatement *node)
{
	int addr = code->emit_jump(node->line_no, Opcode::Jump);

	if (node->lex == Lexeme::Break)
	{
		break_jumps.push_back(addr);
		break_count++;
	}
	else
	{
		assert(node->lex == Lexeme::Continue);
		continue_jumps.push_back(addr);
		continue_count++;
	}
}

void Compiler::backpatch_continues(int previous)
{
	for (int i = 0; i < continue_count; i++)
	{
		int addr = continue_jumps.back();
		continue_jumps.pop_back();
		code->backpatch(addr);
	}
	continue_count = previous;
}

void Compiler::visit_parameter(RoutineParameter *node)
{
	if (node->add_names)
	{
		auto ident = static_cast<Variable*>(node->variable.get());
		// From the function's point of view, the parameters are just the first locals.
		add_local(ident->name);
	}
	else
	{
		if (node->type)
		{
			// The expression must evaluate to a Class at runtime.
			node->type->visit(*this);
		}
		else
		{
			// Parameters with no type are implicitly tagged as Object.
			auto id = routine->add_string_constant(Class::get_name<Object>());
			EMIT(Opcode::GetGlobal, id);
		}
	}
}

void Compiler::visit_routine(RoutineDefinition *node)
{
	if (node->params.size() > PARAM_BITSET_SIZE) {
		throw RuntimeError(node->line_no, "[Syntax error] Maximum number of parameters exceeded (limit is %)", PARAM_BITSET_SIZE);
	}
	auto ident = static_cast<Variable*>(node->name.get());
	auto &name = ident->name;
	auto func = create_function_symbol(node, name);

	// Compile inner routine.
	auto previous_scope = open_scope();
	auto outer_routine = routine;
	set_routine(std::make_shared<Routine>(name));
	EMIT(Opcode::NewFrame, 0);
	int frame_offset = code->get_current_offset() - 1;

	for (auto &param : node->params)
	{
		// Compile names in the new function.
		static_cast<RoutineParameter*>(param.get())->add_names = true;
		param->visit(*this);
	}
	node->body->visit(*this);
	EMIT(Opcode::Return);
	// Fix number of locals.
	code->backpatch_instruction(frame_offset, (Instruction)routine->local_count());
	close_scope(previous_scope);

	auto index = outer_routine->add_routine(routine);
	func->add_routine(std::move(routine));
	set_routine(std::move(outer_routine));

	// Compile type information in the outer routine.
	for (auto &param : node->params)
	{
		static_cast<RoutineParameter*>(param.get())->add_names = false;
		param->visit(*this);
	}
	EMIT(Opcode::SetSignature, index, Instruction(node->params.size()));
}

Handle<Function> Compiler::create_function_symbol(RoutineDefinition *node, const String &name)
{
	Handle<Function> func;

	if (node->local || scope_depth > 1)
	{
		auto symbol = routine->find_local(name, current_scope);

		if (symbol)
		{
			func = routine->get_function(*symbol);
		}
		else
		{
			func = make_handle<Function>(name);
			auto symbol = routine->add_function(func);
			auto index = add_local(name);
			EMIT(Opcode::PushFunction, symbol);
			EMIT(Opcode::DefineLocal, index);
		}
	}
	else
	{
		func = make_handle<Function>(name);
		auto symbol = routine->add_function(func);
		auto index = routine->add_string_constant(name);
		EMIT(Opcode::PushFunction, symbol);
		// Don't use DefineGlobal here.
		EMIT(Opcode::SetGlobal, index);
	}

	return func;
}

Instruction Compiler::add_local(const String &name)
{
	return routine->add_local(name, current_scope, scope_depth);
}

void Compiler::set_routine(std::shared_ptr<Routine> r)
{
	this->code = r ? &r->code : nullptr;
	this->routine = std::move(r);
}

void Compiler::visit_return_statement(ReturnStatement *node)
{
	if (node->expr)
	{
		node->expr->visit(*this);
	}
	else
	{
		EMIT(Opcode::PushNull);
	}
	EMIT(Opcode::Return);
}


} // namespace calao

#undef VISIT
#undef EMIT
#undef THROW