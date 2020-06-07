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

#include <phon/file.hpp>
#include <phon/runtime/source_code.hpp>

namespace phonometrica {

SourceCode::SourceCode()
{

}

void SourceCode::load_file(const String &path)
{
    File infile(path, File::Mode::Read);
    m_lines = infile.read_lines();
    this->m_path = path;
}

void SourceCode::load_code(const String &code)
{
    m_lines = code.split("\n");
    // The splitter is stripped when splitting a string, so we need to put it back.
    for (auto &ln : m_lines)
    {
        ln.append('\n');
    }

    m_path.clear();
}

String SourceCode::filename() const
{
    return m_path.empty() ? String("string buffer") : m_path;
}

String SourceCode::get_line(intptr_t index) const
{
    return m_lines[index];
}

intptr_t SourceCode::size() const
{
    return m_lines.size();
}

void SourceCode::report_error(const char *error_type, intptr_t line_no, const std::string &hint)
{
    assert(line_no > 0);
    String line = m_lines[line_no];
    line.rtrim();
    auto message = utils::format("[%] File \"%\" at line %\n\t%", error_type, this->filename(), line_no, line);

    if (!hint.empty())
    {
        message.append("\nHint: ");
        message.append(hint);
    }

    throw error(message);
}

} // namespace phonometrica