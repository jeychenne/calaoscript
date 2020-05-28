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

void Code::emit_return()
{
	intptr_t index = lines.empty() ? intptr_t(0) : intptr_t(lines.back().first);
	emit(index, Opcode::Return);
}

void Code::backpatch(int at)
{
	// If this fails, adjust the following code, as well as read_integer() and emit_jump().
	static_assert(IntSerializer::IntSize == 2);
	IntSerializer s = { .value = get_current_offset() };
	code[at] = s.ins[0];
 	code[at + 1] = s.ins[1];
}

int Code::read_integer(const Instruction *&ip)
{
	IntSerializer s;
	s.ins[0] = *ip++;
	s.ins[1] = *ip++;

	return s.value;
}

int Code::emit_jump(intptr_t line_no, Opcode jmp)
{
	return emit_jump(line_no, jmp, 0);
}

void Code::backpatch_instruction(int at, Instruction value)
{
	code[at] = value;
}

int Code::emit_jump(intptr_t line_no, Opcode jmp, int addr)
{
	emit(line_no, jmp);
	IntSerializer s = { .value = addr };
	auto offset = get_current_offset();
	emit(line_no, s.ins[0]);
	emit(line_no, s.ins[1]);

	return offset;
}


} // namespace calao
