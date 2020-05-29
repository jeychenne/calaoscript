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
#include <calao/utils/span.hpp>

namespace calao {

class Runtime;
class Function;

// Flags used to distinguish references and values in function signatures.
static constexpr size_t PARAM_BITSET_SIZE = 64;
using ParamBitset = std::bitset<PARAM_BITSET_SIZE>;

// A native C++ callback.
using NativeCallback = std::function<Variant(std::span<Variant>)>;

// A Callable is an internal abstract bate type type used to represent one particular signature for a function. Each function has at least
// one callable, and each callable is owned by at least one function. Callable has two subclasses: NativeRoutine, which is implemented in C++,
// and Routine, which a user-defined function. Callables are an implementation detail and are not visible to users.
class Callable
{
public:

	Callable() = default; // routine without parameters

	Callable(std::vector<Handle<Class>> sig, ParamBitset ref_flags);

	virtual ~Callable() = default;

	int arg_count() const { return int(signature.size()); }

	bool check_arg_count(int n) const { return arg_count() == n; }

	bool check_ref(ParamBitset ref) { return ref_flags == ref; }

	void add_parameter_type(Handle<Class> cls) { signature.push_back(std::move(cls)); }

protected:
	// Type of positional arguments.
	std::vector<Handle<Class>> signature;

	// Indicates whether a parameter is a reference (1) or a value (0). Bit 0 refers to the return value, whereas bits 1
	// to 63 refer to parameter.
	ParamBitset ref_flags;
};


//----------------------------------------------------------------------------------------------------------------------

// A routine implemented in C++.
struct NativeRoutine final : public Callable
{
	NativeRoutine(NativeCallback cb, std::vector<Handle<Class>> sig, ParamBitset ref_flags);

	NativeCallback callback;
};


//----------------------------------------------------------------------------------------------------------------------

// A user-defined routine.
class Routine final : public Callable
{
public:

	struct Local
	{
		String name;
		int scope, depth;
	};

	Routine();

	Routine(std::vector<Handle<Class>> sig, ParamBitset ref_flags);

	Instruction add_integer_constant(intptr_t i);

	Instruction add_float_constant(double n);

	Instruction add_string_constant(String s);
	
	Instruction add_function(Handle<Function> f);

	Instruction add_routine(std::shared_ptr<Routine> r);

	Instruction add_local(const String &name, int scope, int depth);

	std::optional<Instruction> find_local(const String &name, int scope) const;

	double get_float(intptr_t i) const { return float_pool[i]; }

	intptr_t get_integer(intptr_t i) const { return integer_pool[i]; }

	String get_string(intptr_t i) const { return string_pool[i]; }
	
	Handle<Function> get_function(intptr_t i) const { return function_pool[i]; }

	std::shared_ptr<Routine> get_routine(intptr_t i) const { return routine_pool[i]; }

	String get_local_name(intptr_t i) const { return locals[i].name; }

	int local_count() const;

private:

	friend class Runtime;
	friend class Compiler;

	// Bytecode.
	Code code;


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

	/*
	 * The following piece of code is valid:
	 *
	 * #------------------------------------------------
	 * function outer()
	 *
	 *     function inner()
	 *         print "inner without arguments"
	 *     end
	 *
	 *     function inner(arg as String)
	 *         print "inner with one argument: " & arg
	 *     end
	 *
	 *     return inner
	 * end
	 *
	 * var f = outer()
	 * f()
	 * f("test")
	 * #------------------------------------------------
	 *
	 * Inside outer(), we define one "inner" function and two routines (one for each signature). We must be able to
	 * reference each routine at runtime in order to set their signature, and we must also be able to reference each
	 * function since functions are first-class values. As a result, each routine redundantly stores a list of routines
	 * and a list of functions.
	 */
	std::vector<Handle<Function>> function_pool;
	std::vector<std::shared_ptr<Routine>> routine_pool;

	// Local variables.
	std::vector<Local> locals;
};


//----------------------------------------------------------------------------------------------------------------------

// Functions are first-class objects. They can have several signatures, each of which is represented by a routine.
// Whenever a function is called, the appropriate routine is selected based on the number and type of the arguments.
// Note that all functions are wrapped in a Closure, which may optionally capture the function's lexical environment.
// However, this is an implementation detail and from the user's perspective functions and closures are indistinguishable.
class Function final
{
public:

	explicit Function(String name) : _name(std::move(name)) { }

	explicit Function(String name, std::shared_ptr<Callable> r);

	String name() const { return _name; }

	void add_routine(std::shared_ptr<Callable> r);

private:

	// Name provided when the function was declared. Anonymous functions don't have a name.
	String _name;

	// Each function signature is represented by a different routine, which may be native or user-defined.
	std::vector<std::shared_ptr<Callable>> routines;
};

} // namespace calao

#endif // CALAO_FUNCTION_HPP
