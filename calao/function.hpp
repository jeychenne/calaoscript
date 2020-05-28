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
#include <optional>
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

// A Callable is an internal abstract bate type type used to represent one particular signature for a function. Each function has at least
// one callable, and each callable is owned by at least one function. Callable has two subclasses: NativeRoutine, which is implemented in C++,
// and Routine, which a user-defined function.
struct Callable
{
	Callable() = default; // routine without parameters

	Callable(std::vector<Handle<Class>> sig, ParamBitset ref_flags, int min_arg, int max_arg);

	virtual ~Callable() = default;

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


//----------------------------------------------------------------------------------------------------------------------

// A routine implemented in C++.
struct NativeRoutine : public Callable
{
	NativeRoutine(NativeCallback cb, std::vector<Handle<Class>> sig, ParamBitset ref_flags, int min_arg, int max_arg);

	void call(Runtime &, CallInfo &) override;

	NativeCallback callback;
};


//----------------------------------------------------------------------------------------------------------------------

// A user-defined routine.
struct Routine : public Callable
{
	struct Local
	{
		String name;
		int scope, depth;
	};

	Routine();

	Routine(std::vector<Handle<Class>> sig, ParamBitset ref_flags, int min_arg, int max_arg);

	void call(Runtime &, CallInfo &) override;

	Instruction add_integer_constant(intptr_t i);

	Instruction add_float_constant(double n);

	Instruction add_string_constant(String s);

	Instruction add_local(const String &name, int scope, int depth);

	std::optional<Instruction> find_local(const String &name, int scope) const;

	double get_float(intptr_t i) const { return float_pool[i]; }

	intptr_t get_integer(intptr_t i) const { return integer_pool[i]; }

	String get_string(intptr_t i) const { return string_pool[i]; }

	String get_local_name(intptr_t i) const { return locals[i].name; }

	int local_count() const;

	// Bytecode.
	Code code;

private:

	template<class T>
	Instruction add_constant(std::vector<T> &vec, T value)
	{
		auto it = std::find(vec.begin(), vec.end(), value);

		if (it == vec.end())
		{
			if (unlikely(vec.size() == (std::numeric_limits<Instruction>::max)())) {
				throw error("Maximum number of constants exceeded");
			}
			vec.push_back(std::move(value));
			return Instruction(vec.size() - 1);
		}

		return Instruction(std::distance(vec.begin(), it));
	}

	// Constant pools.
	std::vector<double> float_pool;
	std::vector<intptr_t> integer_pool;
	std::vector<String> string_pool;

	// Local variables.
	std::vector<Local> locals;
};


//----------------------------------------------------------------------------------------------------------------------

class Function
{

};

} // namespace calao

#endif // CALAO_FUNCTION_HPP
