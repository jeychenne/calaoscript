/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 22/05/2020                                                                                                *
 *                                                                                                                    *
 * Purpose: Function object.                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_FUNCTION_HPP
#define CALAO_FUNCTION_HPP

#include <bitset>
#include <functional>
#include <vector>
#include <calao/typed_object.hpp>
#include <calao/variant.hpp>
#include <calao/internal/code.hpp>

namespace calao {

class Runtime;
class CallInfo;

// Flags used to distinguish references and values in function signatures.
using ParamBitset = std::bitset<64>;

// A native C++ callback.
using NativeCallback = std::function<Variant(CallInfo &)>;

// A Routine is an internal data type used to represent one particular signature for a function. Each function has at least
// one routine, and each routine is owned by at least one function.
struct Routine
{
	Routine() = default; // routine without parameters

	Routine(std::vector<Handle<Class>> sig, ParamBitset ref_flags, int min_arg, int max_arg);

	virtual ~Routine() = default;

	virtual void call(Runtime &, CallInfo &) = 0;

	bool accept_argc(int n) const { return n >= min_argc && n <= max_argc; }

	// Type of positional arguments.
	std::vector<Handle<Class>> signature;

	// Indicates whether a parameter is a reference (1) or a value (0). Bit 0 refers to the return value, whereas bits 1
	// to 63 refer to parameter.
	ParamBitset ref_flags;

	// Minimum and maximum number of arguments.
	int min_argc = 0, max_argc = 0;
};

// A routine implemented in C++
struct NativeRoutine : public Routine
{
	NativeRoutine(NativeCallback cb, std::vector<Handle<Class>> sig, ParamBitset ref_flags, int min_arg, int max_arg);

	void call(Runtime &, CallInfo &) override;

	NativeCallback callback;
};


struct ScriptRoutine : public Routine
{
	ScriptRoutine();

	ScriptRoutine(std::vector<Handle<Class>> sig, ParamBitset ref_flags, int min_arg, int max_arg);

	void call(Runtime &, CallInfo &) override;

	Code code;
};


//----------------------------------------------------------------------------------------------------------------------

class Function
{

};

} // namespace calao

#endif // CALAO_FUNCTION_HPP
