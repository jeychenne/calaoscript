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

#include <vector>
#include <calao/definitions.hpp>

namespace calao {

using Instruction = uint8_t;


enum class Opcode : Instruction
{
	Return
};


class Code final
{
	using Storage = std::vector<Instruction>;

public:

	Code() = default;

	~Code() = default;

	void write(Instruction i) { code.push_back(i); }

	void write(Opcode op) { write(static_cast<Instruction>(op)); }

	const Instruction *opcodes() const { return code.data(); }

	const Instruction *end() const { return code.data() + code.size(); }

	const Instruction &operator[](size_t i) const { return code[i]; }

	size_t size() const { return code.size(); }

private:

	Storage code;
};

} // namespace calao

#endif // CALAO_CODE_HPP
