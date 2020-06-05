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

	virtual bool is_literal() const { return false; }

	virtual void mark_assigned() { is_assigned = true; }

	template<class T>
	bool is() const { return dynamic_cast<const T*>(this) != nullptr; }

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

struct Literal : public Ast
{
	explicit Literal(int line) : Ast(line) { }

	bool is_literal() const override { return true; }
};

// null, nan, true, false
struct ConstantLiteral final : public Literal
{
	ConstantLiteral(int line, Lexeme lex) : Literal(line), lex(lex) { }

	void visit(AstVisitor &v) override;

	Lexeme lex;
};

struct FloatLiteral final : public Literal
{
	FloatLiteral(int line, double value) : Literal(line), value(value) { }

	void visit(AstVisitor &v) override;

	double value;
};

struct IntegerLiteral final : public Literal
{
	IntegerLiteral(int line, intptr_t value) : Literal(line), value(value) { }

	void visit(AstVisitor &v) override;

	intptr_t value;
};

struct StringLiteral final : public Literal
{
	StringLiteral(int line, String value) : Literal(line), value(std::move(value)) { }

	void visit(AstVisitor &v) override;

	String value;
};

struct ListLiteral final : public Literal
{
	ListLiteral(int line, AstList items) : Literal(line), items(std::move(items)){ }

	void visit(AstVisitor &v) override;

	AstList items;
};

// Table or Set
struct TableLiteral final : public Literal
{
	TableLiteral(int line, AstList keys, AstList values) : Literal(line), keys(std::move(keys)), values(std::move(values)) { }

	void visit(AstVisitor &v) override;

	bool is_set() const { return !keys.empty() && values.empty(); }

	AstList keys, values;
};

//---------------------------------------------------------------------------------------------------------------------

struct ReferenceExpression final : public Ast
{
	ReferenceExpression(int line, AutoAst e) : Ast(line), expr(std::move(e)) { }

	void visit(AstVisitor &v) override;

	AutoAst expr;
};

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
	PrintStatement(int line, AstList lst, bool new_line) : Ast(line), list(std::move(lst)), new_line(new_line) { }

	void visit(AstVisitor &v) override;

	AstList list;
	bool new_line;
};

struct AssertStatement final : public Ast
{
	AssertStatement(int line, AutoAst e, AutoAst msg) : Ast(line), expr(std::move(e)), msg(std::move(msg)) { }

	void visit(AstVisitor &v) override;

	AutoAst expr, msg;
};

struct IfCondition final : public Ast
{
	IfCondition(int line, AutoAst cond, AutoAst block) : Ast(line), cond(std::move(cond)), block(std::move(block)) { }

	void visit(AstVisitor &v) override;

	AutoAst cond, block;
	int conditional_jump = -1;
	int unconditional_jump = -1;
};

struct IfStatement final : public Ast
{
	IfStatement(int line, AstList ifs, AutoAst else_block) : Ast(line), if_conds(std::move(ifs)), else_block(std::move(else_block)) { }

	void visit(AstVisitor &v) override;

	// The first entry in this list represents the obligatory "if" statement.
	// Additional entries represent optional "elsif" statements.
	AstList if_conds;
	AutoAst else_block;
};

struct WhileStatement final : public Ast
{
	WhileStatement(int line, AutoAst e, AutoAst block) : Ast(line), cond(std::move(e)), block(std::move(block)) { }

	void visit(AstVisitor &v) override;

	AutoAst cond, block;
};

struct ForStatement final : public Ast
{
	ForStatement(int line, AutoAst var, AutoAst e1, AutoAst e2, AutoAst e3, AutoAst block, bool down) :
		Ast(line), var(std::move(var)), start(std::move(e1)), end(std::move(e2)), step(std::move(e3)), block(std::move(block)), down(down) { }

	void visit(AstVisitor &v) override;

	AutoAst var, start, end, step, block;
	bool down;
};

struct ForeachStatement final : public Ast
{
	ForeachStatement(int line, AutoAst k, AutoAst v, AutoAst coll, AutoAst block) : Ast(line),
		key(std::move(k)), value(std::move(v)), collection(std::move(coll)), block(std::move(block)) { }

	void visit(AstVisitor &v) override;

	AutoAst key, value, collection, block;
};

struct LoopExitStatement final : public Ast
{
	LoopExitStatement(int line, Lexeme lex) : Ast(line), lex(lex) { }

	void visit(AstVisitor &v) override;

	Lexeme lex;
};

//---------------------------------------------------------------------------------------------------------------------

struct RoutineParameter final : public Ast
{
	RoutineParameter(int line, AutoAst var, AutoAst type, bool ref) :
		Ast(line), variable(std::move(var)), type(std::move(type)), by_ref(ref) { }

	void visit(AstVisitor &v) override;

	AutoAst variable, type;
	bool by_ref;             // passed by value or by reference?
	bool add_names = false;  // flag for the compiler
};

struct RoutineDefinition final : public Ast
{
	RoutineDefinition(int line, AutoAst name, AstList params, AutoAst body, bool local, bool method) :
		Ast(line), name(std::move(name)),  body(std::move(body)), params(std::move(params)), local(local), method(method) { }

	void visit(AstVisitor &v) override;

	AutoAst name, body;
	AstList params;
	bool local, method;
};

struct CallExpression final : public Ast
{
	CallExpression(int line, AutoAst e, AstList args) : Ast(line), expr(std::move(e)), args(std::move(args)) { }

	void visit(AstVisitor &v) override;

	AutoAst expr;
	AstList args;
	bool return_reference = false; // flag for the compiler
};

struct ReturnStatement final : public Ast
{
	ReturnStatement(int line, AutoAst e) : Ast(line), expr(std::move(e)) { }

	void visit(AstVisitor &v) override;

	AutoAst expr;
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
	virtual void visit_list(ListLiteral *node) = 0;
	virtual void visit_table(TableLiteral *node) = 0;
	virtual void visit_unary(UnaryExpression *node) = 0;
	virtual void visit_binary(BinaryExpression *node) = 0;
	virtual void visit_statements(StatementList *node) = 0;
	virtual void visit_declaration(Declaration *node) = 0;
	virtual void visit_print_statement(PrintStatement *node) = 0;
	virtual void visit_call(CallExpression *node) = 0;
	virtual void visit_parameter(RoutineParameter *node) = 0;
	virtual void visit_routine(RoutineDefinition *node) = 0;
	virtual void visit_variable(Variable *node) = 0;
	virtual void visit_assignment(Assignment *node) = 0;
	virtual void visit_assert_statement(AssertStatement *node) = 0;
	virtual void visit_concat_expression(ConcatExpression *node) = 0;
	virtual void visit_if_condition(IfCondition *node) = 0;
	virtual void visit_if_statement(IfStatement *node) = 0;
	virtual void visit_while_statement(WhileStatement *node) = 0;
	virtual void visit_for_statement(ForStatement *node) = 0;
	virtual void visit_foreach_statement(ForeachStatement *node) = 0;
	virtual void visit_loop_exit(LoopExitStatement *node) = 0;
	virtual void visit_return_statement(ReturnStatement *node) = 0;
	virtual void visit_reference_expression(ReferenceExpression *node) = 0;
};

} // namespace calao


#endif // CALAO_AST_HPP
