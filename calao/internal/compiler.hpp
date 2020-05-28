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
 * Purpose: the compiler turns an AST into an executable routine.                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/


#ifndef CALAO_COMPILER_HPP
#define CALAO_COMPILER_HPP

#include <memory>
#include <calao/function.hpp>
#include <calao/internal/ast.hpp>

namespace calao {

class Compiler final : public AstVisitor
{
public:

	Compiler() = default;

	std::shared_ptr<Routine> compile(AutoAst ast);

	void visit_constant(ConstantLiteral *node) override;
	void visit_integer(IntegerLiteral *node) override;
	void visit_float(FloatLiteral *node) override;
	void visit_string(StringLiteral *node) override;
	void visit_unary(UnaryExpression *node) override;
	void visit_binary(BinaryExpression *node) override;
	void visit_statements(StatementList *node) override;
	void visit_declaration(Declaration *node) override;
	void visit_print_statement(PrintStatement *node) override;
	void visit_call(CallExpression *node) override;
	void visit_variable(Variable *node) override;
	void visit_assignment(Assignment *node) override;
	void visit_assert_statement(AssertStatement *node) override;
	void visit_concat_expression(ConcatExpression *node) override;
	void visit_if_condition(IfCondition *node) override;
	void visit_if_statement(IfStatement *node) override;
	void visit_while_statement(WhileStatement *node) override;

private:

	// Routine being compiled.
	std::shared_ptr<Routine> routine;

	void initialize();

	void finalize();

	int open_scope();

	void close_scope(int previous);

	std::pair<int,int> get_scope() const;

	// Code of the routine being compiled.
	Code *code = nullptr;

	// Keep track of scopes.
	int current_scope = 0;

	// Generate unique ID's for scopes (0 = global scope, 1 = module scope).
	int scope_id = 0;

	// In addition to scope ID's, we record each scope's depth to resolve non-local variables.
	int scope_depth = 0;
};

} // namespace calao

#endif // CALAO_COMPILER_HPP
