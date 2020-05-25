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
 * Purpose: see header.                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include <calao/hashmap.hpp>
#include <calao/internal/token.hpp>

namespace calao {

static bool tokens_initialized = false;

static Hashmap<String, Token::Lexeme> token_codes;

static Array<String> token_names = {
        "unknown",
        "and",
        "as",
        "assert",
        "break",
        "by",
        "class",
        "continue",
        "debug",
        "do",
        "downto",
        "each",
        "else",
        "elsif",
        "end",
        "false",
        "field",
        "for",
        "function",
        "if",
        "import",
        "in",
        "inherits",
        "method",
        "nan",
        "new",
        "not",
        "null",
        "opt",
        "option",
        "or",
        "pass",
        "print",
        "ref",
        "return",
        "super",
        "then",
        "this",
        "to",
        "true",
        "until",
        "var",
        "while",
        "=",
        "<=>",
        "&",
        "==",
        ">=",
        ">",
        "<=",
        "<",
        "-",
        "%",
        "!=",
        "+",
        "^",
		"/",
        "*",
        ",",
        ":",
        ".",
        "(",
        ")",
        "{",
        "}",
        "[",
        "]",
        ";",
        "identifier",
        "integer literal",
        "float literal",
        "string literal",
        "end of line",
        "end of text"
};

Token::Token(const String &spelling, intptr_t line, bool ident) :
        spelling(spelling), line_no(line)
{
    auto it = token_codes.find(spelling);

    if (it == token_codes.end())
    {
        this->id = ident ? Lexeme::Identifier : Lexeme::Unknown;
        assert(ident || id != Lexeme::Unknown);
    }
    else
    {
        this->id = it->second;
    }
}

Token::Token(Lexeme type, const String &spelling, intptr_t line) :
        spelling(spelling), line_no(line), id(type)
{

}

String Token::to_string() const
{
    if (id == Lexeme::StringLiteral)
    {
        return String::format("\"%s\"", spelling.data());
    }
    else
    {
        return spelling;
    }
}

bool Token::is_block_end() const
{
    switch (id)
    {
    case Lexeme::End:
    case Lexeme::Else:
    case Lexeme::Elsif:
    case Lexeme::Eot:
        return true;
    default:
        return false;
    }
}

void Token::initialize()
{
    if (tokens_initialized)
    {
        throw error("[Internal error] Tokens must be initialized only once");
    }

    auto last_token = static_cast<int>(Lexeme::Eot);

    for (int i = 0; i <= last_token; ++i)
    {
        auto tok = static_cast<Lexeme>(i);
        auto &name = token_names[i + 1];
        token_codes[name] = tok;
    }

    tokens_initialized = true;
}

String Token::get_name(Lexeme c)
{
    return token_names[static_cast<int>(c) + 1];
}

String Token::get_name() const
{
    return Token::get_name(id);
}

} // namespace calao