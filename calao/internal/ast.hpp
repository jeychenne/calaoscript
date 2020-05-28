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
 * Purpose: abstract syntax tree (AST). The compiler uses the AST produced by the parser to generate bytecode.        *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_AST_HPP
#define CALAO_AST_HPP

#include <memory>
#include <calao/string.hpp>
#include <calao/internal/token.hpp>

namespace calao {

// Forward declarations
class AstVisitor;
struct Ast;
using AutoAst = std::unique_ptr<Ast>;

// Abstract base class for all AST nodes
struct Ast
{
	using Lexeme = Token::Lexeme;

	explicit Ast(int ln) : line_no(ln) { }

	virtual ~Ast() = default;

	virtual void visit(AstVisitor &v) = 0;

	virtual void mark_assigned() { is_assigned = true; }

	template<class T>
	bool is() const { return dynamic_cast<T*>(this) != nullptr; }

	// Line number, for error reporting.
	int line_no;

	// Whether the node is the left hand side of an assignment
	bool is_assigned = false;
};

using AstList = std::vector<AutoAst>;

// Base class for data passed to AST visitors
struct AstContext
{
	virtual ~AstContext() = default;
};


//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------

struct Assignment final : public Ast
{
	Assignment(int line, AutoAst lhs, AutoAst rhs) : Ast(line), lhs(std::move(lhs)), rhs(std::move(rhs)) { }

	void visit(AstVisitor &v) override;

	AutoAst lhs, rhs;
};

// null, nan, true, false
struct ConstantLiteral final : public Ast
{
	ConstantLiteral(int line, Lexeme lex) : Ast(line), lex(lex) { }

	void visit(AstVisitor &v) override;

	Lexeme lex;
};

struct FloatLiteral final : public Ast
{
	FloatLiteral(int line, double value) : Ast(line), value(value) { }

	void visit(AstVisitor &v) override;

	double value;
};

struct IntegerLiteral final : public Ast
{
	IntegerLiteral(int line, intptr_t value) : Ast(line), value(value) { }

	void visit(AstVisitor &v) override;

	intptr_t value;
};

struct StringLiteral final : public Ast
{
	StringLiteral(int line, String value) : Ast(line), value(std::move(value)) { }

	void visit(AstVisitor &v) override;

	String value;
};


//---------------------------------------------------------------------------------------------------------------------

struct UnaryExpression final : public Ast
{
	UnaryExpression(int line, Lexeme op, AutoAst expr) : Ast(line), op(op), expr(std::move(expr)) { }

	void visit(AstVisitor &v) override;

	Lexeme op;
	AutoAst expr;
};

struct BinaryExpression final : public Ast
{
	BinaryExpression(int line, Lexeme op, AutoAst lhs, AutoAst rhs) : Ast(line), op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) { }

	void visit(AstVisitor &v) override;

	Lexeme op;
	AutoAst lhs, rhs;
};

struct ConcatExpression final : public Ast
{
	ConcatExpression(int line, AstList lst) : Ast(line), list(std::move(lst)) { }

	void visit(AstVisitor &v) override;

	AstList list;
};

struct CallExpression final : public Ast
{
	CallExpression(int line, AutoAst e, AstList args) : Ast(line), expr(std::move(e)), args(std::move(args)) { }

	void visit(AstVisitor &v) override;

	AutoAst expr;
	AstList args;
};


struct Variable final : public Ast
{
	Variable(int line, String name) : Ast(line), name(std::move(name)) { }

	void visit(AstVisitor &v) override;

	String name;
};

//---------------------------------------------------------------------------------------------------------------------

struct StatementList final : public Ast
{
	StatementList(int line, AstList stmts, bool open_scope = false) : Ast(line), statements(std::move(stmts)), open_scope(open_scope) { }

	void visit(AstVisitor &v) override;

	AstList statements;
	bool open_scope;
};

struct Declaration final : public Ast
{
	Declaration(int line, AstList lhs, AstList rhs, bool local) : Ast(line), lhs(std::move(lhs)), rhs(std::move(rhs)), local(local) { }

	void visit(AstVisitor &v) override;

	AstList lhs, rhs;
	bool local;
};

struct PrintStatement final : public Ast
{
	PrintStatement(int line, AutoAst e, bool new_line) : Ast(line), expr(std::move(e)), new_line(new_line) { }

	void visit(AstVisitor &v) override;

	AutoAst expr;
	bool new_line;
};

struct AssertStatement final : public Ast
{
	AssertStatement(int line, AutoAst e, AutoAst msg) : Ast(line), expr(std::move(e)), msg(std::move(msg)) { }

	void visit(AstVisitor &v) override;

	AutoAst expr, msg;
};

//---------------------------------------------------------------------------------------------------------------------

class AstVisitor
{
public:

	virtual ~AstVisitor() = default;
	
	virtual void visit_constant(ConstantLiteral *node) = 0;
	virtual void visit_integer(IntegerLiteral *node) = 0;
	virtual void visit_float(FloatLiteral *node) = 0;
	virtual void visit_string(StringLiteral *node) = 0;
	virtual void visit_unary(UnaryExpression *node) = 0;
	virtual void visit_binary(BinaryExpression *node) = 0;
	virtual void visit_statements(StatementList *node) = 0;
	virtual void visit_declaration(Declaration *node) = 0;
	virtual void visit_print_statement(PrintStatement *node) = 0;
	virtual void visit_call(CallExpression *node) = 0;
	virtual void visit_variable(Variable *node) = 0;
	virtual void visit_assignment(Assignment *node) = 0;
	virtual void visit_assert_statement(AssertStatement *node) = 0;
	virtual void visit_concat_expression(ConcatExpression *node) = 0;
};

} // namespace calao


#endif // CALAO_AST_HPP
