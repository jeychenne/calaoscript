/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 24/05/2020                                                                                                *
 *                                                                                                                    *
 * Purpose: see header.                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include <limits>
#include <calao/error.hpp>
#include <calao/internal/code.hpp>

namespace calao {

Instruction Code::add_integer_constant(intptr_t i)
{
	return add_constant(integer_pool, i);
}

Instruction Code::add_float_constant(double n)
{
	return add_constant(float_pool, n);
}

Instruction Code::add_string_constant(String s)
{
	return add_constant(string_pool, std::move(s));
}

void Code::add_line(intptr_t line_no)
{
	constexpr auto max_lines = (std::numeric_limits<uint16_t>::max)();

	if (unlikely(line_no > max_lines)) {
		throw error("Source file too long: a file can contain at most % lines", max_lines);
	}

	if (lines.empty() || lines.back().first != line_no)
	{
		lines.emplace_back(uint16_t(line_no), 1);
	}
	else
	{
		lines.back().second++;
	}
}

int Code::get_line(int offset) const
{
	int count = 0;

	for (auto ln : lines)
	{
		count += ln.second;

		if (offset < count) {
			return ln.first;
		}
	}

	throw error("[Internal error] Cannot determine line number: invalid offset %", offset);
}

} // namespace calao
