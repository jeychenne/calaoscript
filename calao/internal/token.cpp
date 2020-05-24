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

static Hashmap<String, Token::Code> token_codes;

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
        "new",
        "not",
        "null",
        "opt",
        "option",
        "or",
        "pass",
        "ref",
        "repeat",
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
        "--",
        "/",
        "==",
        ">=",
        ">",
        "++",
        "<=",
        "<",
        "-",
        "%",
        "!=",
        "+",
        "^",
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
        "float literal",
        "string literal",
        "end of file",
        "end of text"
};

Token::Token(const String &spelling, intptr_t line, bool ident) :
        spelling(spelling), line_no(line)
{
    auto it = token_codes.find(spelling);

    if (it == token_codes.end())
    {
        this->id = ident ? Code::Identifier : Code::Unknown;
        assert(ident || id != Code::Unknown);
    }
    else
    {
        this->id = it->second;
    }
}

Token::Token(Code type, const String &spelling, intptr_t line) :
        spelling(spelling), line_no(line), id(type)
{

}

String Token::to_string() const
{
    if (id == Code::StringLiteral)
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
    case Code::End:
    case Code::Else:
    case Code::Elsif:
    case Code::Eot:
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

    auto last_token = static_cast<int>(Code::Eot);

    for (int i = 0; i <= last_token; ++i)
    {
        auto tok = static_cast<Code>(i);
        auto &name = token_names[i + 1];
        token_codes[name] = tok;
    }

    tokens_initialized = true;
}

String Token::get_name(Code c)
{
    return token_names[static_cast<int>(c) + 1];
}

String Token::get_name() const
{
    return Token::get_name(id);
}

bool Token::is_binary_op() const
{
    switch (id)
    {
    case Code::OpEqual:
    case Code::OpNotEqual:
    case Code::OpGreaterThan:
    case Code::OpLessThan:
    case Code::OpGreaterEqual:
    case Code::OpLessEqual:
    case Code::OpPlus:
    case Code::OpMinus:
    case Code::OpStar:
    case Code::OpDiv:
    case Code::OpMod:
    case Code::OpPower:
    case Code::Dot:
    case Code::Or:
    case Code::And:
        return true;

    default:
        return false;
    }
}

bool Token::is_unary_op() const
{
    return (id == Code::Not) || (id == Code::OpMinus);
}

int Token::get_precedence(Code t)
{
    switch (t)
    {
    case Code::Or:
    case Code::And:
        return 0;

    case Code::OpEqual:
    case Code::OpNotEqual:
    case Code::OpGreaterThan:
    case Code::OpLessThan:
    case Code::OpGreaterEqual:
    case Code::OpLessEqual:
        return 100;

    case Code::OpPlus:
    case Code::OpMinus:
        return 200;

    case Code::OpStar:
    case Code::OpDiv:
        return 300;

    case Code::OpMod:
    case Code::OpPower:
        return 400;

    case Code::LParen:
    case Code::LSquare:
        return 500;

    case Code::Dot:
        return 600;

    case Code::Ref:
        return 700;

    default:
        throw error("[Syntax error] Token is not a binary operator");
    }
}

int Token::get_precedence() const
{
    return get_precedence(id);
}

bool Token::is_right_associative() const
{
    return is_right_associative(id);
}

int Token::highest_precedence()
{
    return get_precedence(Code::Dot) + 10;
}

bool Token::is_right_associative(Code)
{
    return false; // For now, all operators are left-associative
}

} // namespace calao