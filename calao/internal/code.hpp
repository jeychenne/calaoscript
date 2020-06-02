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
#include <calao/internal/ast.hpp>

namespace calao {

using Instruction = uint16_t;



enum class Opcode : Instruction
{
	Assert,
	Add,
	Call,
	Compare,
	Concat,
	DecrementLocal,
	DefineGlobal,
	DefineLocal,
	GetGlobal,			// Get global by value
	GetGlobalArg,		// Get global either by value or by reference
	GetGlobalRef,		// Get global by reference
	GetIndex,			// Get index by value
	GetIndexArg,		// Get index by value or by reference
	GetIndexRef,		// Get index by reference
	GetLocal,			// Get global by value
	GetLocalArg,		// Get global either by value or by reference
	GetLocalRef,		// Get global by reference
	GetUniqueGlobal,	// Unshare and push a global
	GetUniqueLocal,		// Unshare and push a local
	Divide,
	Equal,
	Greater,
	GreaterEqual,
	IncrementLocal,
	Jump,
	JumpFalse,
	JumpTrue,
	Less,
	LessEqual,
	Modulus,
	Multiply,
	Negate,
	NewArray,
	NewFrame,
	NewList,
	NewTable,
	Not,
	NotEqual,
	Pop,
	Power,
	Precall,
	Print,
	PrintLine,
	PushBoolean,
	PushFalse,
	PushFloat,
	PushFunction,
	PushInteger,
	PushNan,
	PushNull,
	PushSmallInt,
	PushString,
	PushTrue,
	Return,
	SetGlobal,
	SetIndex,
	SetLocal,
	SetSignature,
	Subtract
};


class Code final
{
	using Storage = std::vector<Instruction>;

	// For error reporting.
	using LineNo = uint16_t;

public:

	// Simple trick to convert back and forth between an int and bytecodes.
	union IntSerializer
	{
		static constexpr size_t IntSize = sizeof(int) / sizeof(Instruction);
		int value;
		Instruction ins[IntSize];
	};

	Code() = default;

	Code(const Code &) = delete;

	Code(Code &&) = default;

	~Code() = default;

	void emit(intptr_t line_no, Instruction i) { add_line(line_no); code.push_back(i); }

	void emit(intptr_t line_no, Opcode op) { emit(line_no, static_cast<Instruction>(op)); }

	void emit(intptr_t line_no, Opcode op, Instruction i) { emit(line_no, op); emit(line_no, i); }

	void emit(intptr_t line_no, Opcode op, Instruction i1, Instruction i2) { emit(line_no, op); emit(line_no, i1); emit(line_no, i2); }

	static int read_integer(const Instruction *&ip);

	void emit_return();

	const Instruction *data() const { return code.data(); }

	const Instruction *end() const { return code.data() + code.size(); }

	const Instruction &operator[](size_t i) const { return code[i]; }

	size_t size() const { return code.size(); }

	int get_line(int offset) const;

	void backpatch_instruction(int at, Instruction value);

	void backpatch(int at);

	int emit_jump(intptr_t line_no, Opcode jmp);

	int emit_jump(intptr_t line_no, Opcode jmp, int addr);

	int get_current_offset() const { return int(code.size()); }

	static const char *get_opcode_name(Instruction op);

private:

	void add_line(intptr_t line_no);

	// Byte codes.
	Storage code;

	// Line numbers on which byte codes are found, for error reporting.
	// first = line number; second = number of instructions on that line
	std::vector<std::pair<LineNo,LineNo>> lines;
};

} // namespace calao

#endif // CALAO_CODE_HPP
