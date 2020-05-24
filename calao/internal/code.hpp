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
 * Purpose: bytecode object, which represents a chunk of compiled code.                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_CODE_HPP
#define CALAO_CODE_HPP

#include <algorithm>
#include <utility>
#include <vector>
#include <calao/string.hpp>

namespace calao {

using Instruction = uint16_t;



enum class Opcode : Instruction
{
	Add,
	Divide,
	Multiply,
	Negate,
	PushBoolean,
	PushInteger,
	PushFloat,
	PushSmallInt,
	PushString,
	Return,
	Subtract
};


class Code final
{
	using Storage = std::vector<Instruction>;

	// For error reporting.
	using LineNo = uint16_t;

public:

	Code() = default;

	~Code() = default;

	void emit(intptr_t line_no, Instruction i) { add_line(line_no); code.push_back(i); }

	void emit(intptr_t line_no, Opcode op) { emit(line_no, static_cast<Instruction>(op)); }

	void emit(intptr_t line_no, Opcode op, Instruction i) { emit(line_no, op); emit(line_no, i); }

	const Instruction *data() const { return code.data(); }

	const Instruction *end() const { return code.data() + code.size(); }

	const Instruction &operator[](size_t i) const { return code[i]; }

	size_t size() const { return code.size(); }

	intptr_t add_integer_constant(intptr_t i);

	intptr_t add_double_constant(double n);

	intptr_t add_string_constant(String s);

	double get_float(intptr_t i) const { return float_pool[i]; }

	intptr_t get_integer(intptr_t i) const { return integer_pool[i]; }

	String get_string(intptr_t i) const { return string_pool[i]; }

	int get_line(int offset) const;

private:

	template<class T>
	intptr_t add_constant(std::vector<T> &vec, T value)
	{
		auto it = std::find(vec.begin(), vec.end(), value);

		if (it == vec.end())
		{
			vec.push_back(std::move(value));
			return intptr_t(vec.size() - 1);
		}

		return intptr_t(std::distance(vec.begin(), it));
	}

	void add_line(intptr_t line_no);

	// Byte codes.
	Storage code;

	// Line numbers on which byte codes are found, for error reporting.
	// first = line number; second = number of instructions on that line
	std::vector<std::pair<LineNo,LineNo>> lines;

	// Constant pools.
	std::vector<double> float_pool;
	std::vector<intptr_t> integer_pool;
	std::vector<String> string_pool;
};

} // namespace calao

#endif // CALAO_CODE_HPP
