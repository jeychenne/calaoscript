/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 18/07/2019                                                                                                *
 *                                                                                                                    *
 * Purpose: tokens for Calao's scripting language.                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_TOKEN_HPP
#define CALAO_TOKEN_HPP

#include <calao/string.hpp>

namespace calao {

struct Token final
{
	// end of text character in ASCII
	static const char32_t ETX = 3;

	// Tokens of the language
	enum class Code
	{
		Unknown = 0,
		And,
		As,
		Assert,
		Break,
		By,
		Class,
		Continue,
		Debug,
		Do,
		Downto,
		Each,
		Else,
		Elsif,
		End,
		False,
		Field,
		For,
		Function,
		If,
		Import,
		In,
		Inherits,
		Method,
		New,
		Not,
		Null,
		Opt,
		Option,
		Or,
		Pass,
		Ref,
		Repeat,
		Return,
		Super,
		Then,
		This,
		To,
		True,
		Until,
		Var,
		While,

		OpAssign,
		OpCompare,
		OpConcat,
		OpDec,
		OpDiv,
		OpEqual,
		OpGreaterEqual,
		OpGreaterThan,
		OpInc,
		OpLessEqual,
		OpLessThan,
		OpMinus,
		OpMod,
		OpNotEqual,
		OpPlus,
		OpPower,
		OpStar,

		Comma,
		Colon,
		Dot,
		LParen,
		RParen,
		LCurl,
		RCurl,
		LSquare,
		RSquare,
		Semicolon,

		Identifier,
		NumberLiteral,
		StringLiteral,

		Eol, // end of line
		Eot // end of text
	};
	
	Token() = default;

	Token(const Token &other) = default;

	Token(Token &&other) = default;

	Token(const String &spelling, intptr_t line, bool ident);

	Token(Code type, const String &spelling, intptr_t line);

	~Token() = default;

	Token &operator=(const Token &) = default;

	Token &operator=(Token &&) = default;

	intptr_t size() const { return (id == Code::Eol or id == Code::Eot) ? 0 : spelling.size(); }

	bool is_eot() const {return id == Code::Eot; }

	String to_string() const;

	bool is_separator() const { return id == Code::Eol || id == Code::Semicolon; }

	bool is_block_end() const;

	bool is(Code c) const { return id == c; }

	static void initialize();

	static String get_name(Code c);

	String get_name() const;

	bool is_binary_op() const;

	static int get_precedence(Code t);

	int get_precedence() const;

	bool is_right_associative() const;

	static int highest_precedence();

	static bool is_right_associative(Code t);

	bool is_command_separator() const { return id == Code::Eol || id == Code::Semicolon; }

	bool is_unary_op() const;


	String spelling;

	intptr_t line_no = 0;

	Code id = Code::Unknown;

};

} // namespace calao

#endif // CALAO_TOKEN_HPP
