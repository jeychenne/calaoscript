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

#include <calao/internal/ast.hpp>

#define VISIT(NODE) v.visit_##NODE(this);

namespace calao {

void calao::UnaryExpression::visit(AstVisitor &v)
{
	VISIT(unary);
}

void BinaryExpression::visit(AstVisitor &v)
{
	VISIT(binary);
}

void ConstantLiteral::visit(AstVisitor &v)
{
	VISIT(constant);
}

void FloatLiteral::visit(AstVisitor &v)
{
	VISIT(float);
}

void IntegerLiteral::visit(AstVisitor &v)
{
	VISIT(integer);
}

void StringLiteral::visit(AstVisitor &v)
{
	VISIT(string);
}

void StatementList::visit(AstVisitor &v)
{
	VISIT(statements);
}

void Declaration::visit(AstVisitor &v)
{
	VISIT(declaration);
}

void PrintStatement::visit(AstVisitor &v)
{
	VISIT(print_statement);
}

void CallExpression::visit(AstVisitor &v)
{
	VISIT(call);
}

void Variable::visit(AstVisitor &v)
{
	VISIT(variable);
}

void Assignment::visit(AstVisitor &v)
{
	VISIT(assignment);
}

void AssertStatement::visit(AstVisitor &v)
{
	VISIT(assert_statement);
}

void ConcatExpression::visit(AstVisitor &v)
{
	VISIT(concat_expression);
}

void IfStatement::visit(AstVisitor &v)
{
	VISIT(if_statement);
}

void IfCondition::visit(AstVisitor &v)
{
	VISIT(if_condition);
}

void WhileStatement::visit(AstVisitor &v)
{
	VISIT(while_statement);
}
} // namespace calao
#undef VISIT
