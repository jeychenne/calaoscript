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

	// Tokens of the language. Don't forget to update the token names.
	enum class Lexeme
	{
		Unknown = 0,
		And,
		As,
		Assert,
		Break,
		Class,
		Continue,
		Debug,
		Do,
		Downto,
		Else,
		Elsif,
		End,
		False,
		Field,
		For,
		Foreach,
		Function,
		If,
		Import,
		In,
		Inherits,
		Local,
		Method,
		Nan,
		Not,
		Null,
		Option,
		Or,
		Pass,
		Print,
		Ref,
		Return,
		Step,
		Super,
		Then,
		This,
		Throw,
		To,
		True,
		Until,
		Var,
		While,

		OpAssign,
		OpAt,
		OpCompare,
		OpConcat,
		OpEqual,
		OpGreaterEqual,
		OpGreaterThan,
		OpLessEqual,
		OpLessThan,
		OpMinus,
		OpMod,
		OpNotEqual,
		OpPlus,
		OpPower,
		OpSlash,
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
		IntegerLiteral,
		FloatLiteral,
		StringLiteral,

		Eol, // end of line
		Eot // end of text
	};
	
	Token() = default;

	Token(const Token &other) = default;

	Token(Token &&other) = default;

	Token(const String &spelling, intptr_t line, bool ident);

	Token(Lexeme type, const String &spelling, intptr_t line);

	~Token() = default;

	Token &operator=(const Token &) = default;

	Token &operator=(Token &&) = default;

	intptr_t size() const { return id == Lexeme::Eot ? 0 : spelling.size(); }

	bool is_eot() const {return id == Lexeme::Eot; }

	String to_string() const;

	bool is_block_end() const;

	bool is_separator() const { return id == Lexeme::Eol || id == Lexeme::Semicolon; }

	bool is(Lexeme c) const { return id == c; }

	static void initialize();

	static String get_name(Lexeme c);

	String get_name() const;

	String spelling;

	intptr_t line_no = 0;

	Lexeme id = Lexeme::Unknown;

};

} // namespace calao

#endif // CALAO_TOKEN_HPP
