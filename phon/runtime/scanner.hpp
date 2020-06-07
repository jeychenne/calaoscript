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
 * Purpose: the scanner performs lexical analysis of a chunk of source code, read from a file or from a string. The   *
 * source is expected (and assumed) to be encoded in UTF-8, and is scanned one code point at a time.                  *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef PHONOMETRICA_SCANNER_HPP
#define PHONOMETRICA_SCANNER_HPP

#include <deque>
#include <phon/runtime/token.hpp>
#include <phon/runtime/source_code.hpp>

namespace phonometrica {

class Scanner final
{
public:

    Scanner();

    // Set source code from a file on disk
    void load_file(const String &path);

    // Set source code from a string
    void load_string(const String &code);

    Token read_token();


    void report_error(const std::string &hint, intptr_t offset = 0, const char *error_type = "Syntax");

    intptr_t line_no() const { return m_line_no; }

    bool has_content() const { return !m_source->empty(); }

    std::shared_ptr<SourceCode> source_code() const { return m_source; }

private:

	// Source code (from a file or string).
    std::shared_ptr<SourceCode> m_source;

    // List of tokens for the current line.
    // This is used for look-ahead, which is needed by our single-pass compiler.
    std::deque<Token> tokens;

    // Current word (accumulates code points)
    String m_spelling;

    // Current line
    String m_line;

    // Current line number
    intptr_t m_line_no;

    // Current position in the current line
    String::const_iterator m_pos;

    // Current code point
    char32_t m_char;

    void reset();

    void rewind();

    void read_char();

    void get_char();

    void set_line(intptr_t index);

    void read_line();

    void skip();

    void skip_white();

    void accept();

    void scan_digits();

    void scan_string(char32_t end);

	// Same as isspace(), but does not consider '\n' as a space since it's used by the parser.
	static bool check_space(char32_t c);
};

} // namespace phonometrica

#endif // PHONOMETRICA_SCANNER_HPP
