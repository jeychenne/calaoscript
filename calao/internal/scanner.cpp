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

#include <calao/file.hpp>
#include <calao/internal/scanner.hpp>

namespace calao {

// Same as isspace(), but does not consider '\n' as a space since it's used by the parser.
static bool check_space(char32_t c)
{
    switch (c)
    {
    case ' ':
    case '\t':
    case '\r':
    case '\f':
    case '\v':
        return true;
    default:
        return false;
    }
}

Scanner::Scanner() :
    m_source(std::make_shared<SourceCode>())
{
    m_line_no = 0;
    m_pos = nullptr;
    m_char = 0;
}

void Scanner::load_file(const String &path)
{
    reset();
    m_source->load_file(path);
    read_char();
}

void Scanner::load_string(const String &code)
{
    reset();
    m_source->load_code(code);
    read_char();
}

void Scanner::reset()
{
    m_pos = nullptr;
    m_line_no = 0;
    m_line.clear();
    m_char = 0;
}

void Scanner::rewind()
{
    if (m_char != Token::ETX)
    {
        // Move back to the beginning of the string
        m_pos = m_line.begin();
        read_char();
    }
    else
    {
        m_pos = nullptr;
    }
}

void Scanner::read_char()
{
    // Never read past the end of the source
    assert(m_char != Token::ETX);

    if (m_pos == m_line.end() || m_pos == nullptr)
    {
        read_line();
        rewind();
    }
    else
    {
        get_char();
    }
}

void Scanner::get_char()
{
    m_char = *m_pos++;
}

void Scanner::set_line(intptr_t index)
{
    m_line = m_source->get_line(index);
}

void Scanner::read_line()
{
    if (m_line_no == m_source->size())
    {
        m_line.clear();
        m_char = Token::ETX;
    }
    else
    {
        set_line(++m_line_no);
    }
}

void Scanner::skip_white()
{

    while (check_space(m_char))
    {
        read_char();
    }
}

void Scanner::skip()
{
    read_char();
}

void Scanner::accept()
{
    m_spelling.append(m_char);
    read_char();
}

void Scanner::scan_digits()
{
    // Allow '_' as a group separator
    while (isdigit(m_char) || m_char == '_')
    {
        if (m_char == '_')
            skip();
        else
            accept();
    }
}

void Scanner::scan_string(char32_t end)
{
    skip();

    while (m_char != end && m_char != Token::ETX)
    {
        if (m_char == '\\')
        {
            // Skip for now. It may be restored if it's not a special character.
            skip();

            if (m_char == 'n')
            {
                m_char = '\n'; // line feed (new line)
            }
            else if (m_char == 't')
            {
                m_char = '\t'; // horizontal tab
            }
            else if (m_char == 'r')
            {
                m_char = '\r'; // carriage return
            }
            else if (m_char == '\\')
            {
                m_char = '\\'; // backslash
            }
            else if (m_char == '\'')
            {
                m_char = '\''; // single quote
            }
            else if (m_char == '"')
            {
                m_char = '\"'; // double quote
            }
            else if (m_char == 'v')
            {
                m_char = '\v'; // vertical tab
            }
            else if (m_char == 'a')
            {
                m_char = '\a'; // audible bell
            }
            else if (m_char == 'b')
            {
                m_char = '\b'; // backspace
            }
            else if (m_char == 'f')
            {
                m_char = '\f'; // form feed (new page)
            }
            else
            {
                // Restore
                m_spelling.push_back('\\');
            }
        }
        accept();
    }

    // If we haven't reached the end of the text, ignore string terminating character
    if (m_char == end)
    { skip(); }
}


// Read one token from the source code
Token Scanner::read_token()
{
    RETRY:
    m_spelling.clear();
    skip_white();

    // An identifier must start with a Unicode "alphabetic character". This includes characters such as
    // Chinese '漢' or Korean '한'.
    if (String::is_letter(m_char))
    {
        accept();

        while (String::is_letter(m_char) || isdigit(m_char) || m_char == U'_')
        {
            accept();
        }

        // Identifier can end with '$'. This is used for "special" symbols, normally used for implementation details.
        // Special methods, such as the init$ constructor, end with this character. Private instance members should also
        // end with '$', although this is not enforced.
        if (m_char == U'$')
        {
            accept();
            // Allow '$'*, so that users can for instance create their own `init$$` symbol if they want to.
            while (m_char == '$')
            { accept(); }
        }

        return Token(m_spelling, m_line_no, true);
    }

    // Scan a number.
    if (isdigit(m_char))
    {
        accept();
        scan_digits();

        if (m_char == U'.')
        {
            accept();
            scan_digits();
        }

        return Token(Token::Code::NumberLiteral, m_spelling, m_line_no);
    }

    switch (m_char)
    {
    case U'=':
    {
        accept();

        if (m_char == U'=')
        {
            accept();
            return Token(m_spelling, m_line_no, false);
        }
        else
        {
            return Token(m_spelling, m_line_no, false);
        }
    }
    case U'"':
    {
        scan_string(U'"');
        return Token(Token::Code::StringLiteral, m_spelling, m_line_no);
    }
    case U'\n':
    {
        accept();
        return Token(Token::Code::Eol, "EOL", m_line_no);
    }
    case Token::ETX:
    {
        // Don't accept token since we reached the end.
        return Token(Token::Code::Eot, "EOT", m_line_no);
    }
    case U'(':
    {
	    accept();
    	return Token(Token::Code::LParen, "(", m_line_no);
    }
    case U')':
    {
	    accept();
	    return Token(Token::Code::RParen, ")", m_line_no);
    }
    case U'{':
    {
	    accept();
	    return Token(Token::Code::LCurl, "{", m_line_no);
    }
    case U'}':
    {
	    accept();
	    return Token(Token::Code::RCurl, "}", m_line_no);
    }
    case U'[':
    {
	    accept();
	    return Token(Token::Code::LSquare, "[", m_line_no);
    }
    case U']':
    {
	    accept();
	    return Token(Token::Code::RSquare, "]", m_line_no);
    }
    case U'+':
    {
    	accept();
    	if (m_char == '+')
	    {
    		accept();
    		return Token(Token::Code::OpInc, "INC", m_line_no);
	    }

    	return Token(Token::Code::OpPlus, "+", m_line_no);
    }
    case U'-':
    {
	    accept();
	    if (m_char == '-')
	    {
		    accept();
		    return Token(Token::Code::OpDec, "DEC", m_line_no);
	    }

	    return Token(Token::Code::OpMinus, "-", m_line_no);
    }
    case U'*':
    {
	    accept();
	    return Token(Token::Code::OpStar, "*", m_line_no);
    }
    case U'/':
    {
	    accept();
	    return Token(Token::Code::OpDiv, "/", m_line_no);
    }
    case U'&':
    {
	    accept();
	    return Token(Token::Code::OpConcat, "&", m_line_no);
    }
    case U',':
    {
	    accept();
	    return Token(Token::Code::Comma, ",", m_line_no);
    }
    case U';':
    {
	    accept();
	    return Token(Token::Code::Semicolon, ";", m_line_no);
    }
    case U':':
    {
	    accept();
	    return Token(Token::Code::Colon, ":", m_line_no);
    }
    case U'.':
    {
        accept();
        return Token(Token::Code::Dot, ".", m_line_no);
    }
    case U'#':
    {
        // Skip comment and read next token
        do skip(); while (m_char != '\n');
        skip();
        goto RETRY;
    }
    case U'!':
    {
        accept();

        if (m_char == U'=')
        {
            accept();
            return Token(Token::Code::OpNotEqual, m_spelling, m_line_no);
        }

        report_error("invalid token");
        break; // never reached.
    }
    case U'<':
    {
        accept();

        if (m_char == U'=')
        {
            accept();

            if (m_char == U'>')
            {
                accept();
                return Token(Token::Code::OpCompare, m_spelling, m_line_no);
            }
            else
            {
                return Token(Token::Code::OpLessEqual, m_spelling, m_line_no);
            }
        }
        else
        {
            return Token(Token::Code::OpLessThan, m_spelling, m_line_no);
        }
    }
    case U'>':
    {
        accept();

        if (m_char == U'=')
        {
            accept();
            return Token(Token::Code::OpGreaterEqual, m_spelling, m_line_no);
        }
        else
        {
            return Token(Token::Code::OpGreaterThan, m_spelling, m_line_no);
        }
    }

    default:
        break;
    }

    report_error("invalid token");

    return Token();
}

void Scanner::report_error(const std::string &hint, intptr_t offset, const char *error_type)
{
	assert(m_line_no != 0);
	String line = m_source->get_line(m_line_no);
	// These must be computed before trimming, since the iterator will be invalidated
	auto step_back = intptr_t(m_pos > line.begin());
	auto left_space = intptr_t(m_pos - line.begin());

	line.rtrim();

	// normalize tabs
	auto old_size = line.size();
	line.replace("\t", "    ");
	auto additional_padding = line.size() - old_size;

	// Set spacing to the location of the error
	auto beginning = step_back; // move 1 char back, unless we are at the beginning
	intptr_t count = left_space + additional_padding - offset - beginning;
	String filler;
	filler.fill(U' ', count);

	auto message = utils::format("[% error] File \"%\" at line %\n%\n%^",
	                             error_type, m_source->filename(), m_line_no, line, filler);

	if (!hint.empty())
	{
		message.append("\nHint: ");
		message.append(hint);
	}

	throw error(message);
}

} // namespace calao