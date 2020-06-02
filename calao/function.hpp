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
 * Purpose: Function object. Calao functions support multiple dispatch: a function may have several overloads, and    *
 * the correct routine is selected at runtime based on the type of the arguments.                                     *
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


//---------------------------------------------------------------------------------------------------------------------

// A structure to encapsulate a list of arguments passed to a function.
class ArgumentList final
{
public:

	ArgumentList(Runtime &rt, std::span<Variant> args) : _args(args), _rt(rt) { }

	ArgumentList(const ArgumentList &) = delete;

	Variant &operator[](size_t i) { return _args[i].resolve(); }

	template<class T>
	T &get(size_t i) { return cast<T>(_args[i].resolve()); }

	template<class T>
	T &raw_get(size_t i) { return raw_cast<T>(_args[i].resolve()); }

	template<class T>
	bool check_type(size_t i) const { return check_type<T>(_args[i].resolve()); }

	size_t count() const { return _args.size(); }

	Runtime &runtime() { return _rt; }

private:

	std::span<Variant> _args;

	Runtime &_rt;
};


//---------------------------------------------------------------------------------------------------------------------


// A native C++ callback.
using NativeCallback = std::function<Variant(ArgumentList&)>;

// A Callable is an internal abstract bate type type used to represent one particular signature for a function. Each function has at least
// one callable, and each callable is owned by at least one function. Callable has two subclasses: NativeRoutine, which is implemented in C++,
// and Routine, which a user-defined function. Callables are an implementation detail and are not visible to users.
class Callable
{
public:

	Callable(const String &name, int argc) : _name(name), _argc(argc) { } // routine without parameters

	Callable(const String &name, std::vector<Handle<Class>> sig, ParamBitset ref_flags);

	virtual ~Callable() = default;

	virtual bool is_native() const = 0;

	int arg_count() const { return _argc; }

	bool check_arg_count(int n) const { return arg_count() == n; }

	bool check_ref(ParamBitset ref) { return ref_flags == ref; }

	void add_parameter_type(Handle<Class> cls) { signature.push_back(std::move(cls)); }

	String name() const { return _name; }

	Variant operator()(ArgumentList &args) { return call(args); }

	int get_cost(std::span<Variant> args) const;

	String get_definition() const;

protected:

	friend class Function;

	virtual Variant call(ArgumentList &args) = 0;

	// Type of positional arguments.
	std::vector<Handle<Class>> signature;

	// Indicates whether a parameter is a reference (1) or a value (0).
	ParamBitset ref_flags;

	// For debugging and stack traces.
	String _name;

	// We store the argument count because user-defined routines only know the number of parameters after their signature is computed.
	int _argc;
};


//----------------------------------------------------------------------------------------------------------------------

// A routine implemented in C++.
struct NativeRoutine final : public Callable
{
	NativeRoutine(const String &name, NativeCallback cb, std::initializer_list<Handle<Class>> sig, ParamBitset ref_flags = 0);

	bool is_native() const override { return true; }

	NativeCallback callback;

	Variant call(ArgumentList &args) override;
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

	Routine(const String &name, int argc);

	Routine(const String &name, std::vector<Handle<Class>> sig, ParamBitset ref_flags);

	bool is_native() const override { return false; }

	Instruction add_integer_constant(intptr_t i);

	Instruction add_float_constant(double n);

	Instruction add_string_constant(String s);
	
	Instruction add_function(Handle<Function> f);

	Instruction add_routine(std::shared_ptr<Routine> r);

	Instruction add_local(const String &name, int scope, int depth);

	std::optional<Instruction> find_local(const String &name) const;

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

	Variant call(ArgumentList &args) override;

	void clear_signature() { signature.clear(); }

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

// A Function object (also known as 'generic function' in languages with multiple dispatch).
// Functions are first-class objects. They can have several signatures, each of which is represented by a routine.
// Whenever a function is called, the appropriate routine is selected based on the number and type of the arguments.
// Note that all functions are wrapped in a Closure, which may optionally capture the function's lexical environment.
// However, this is an implementation detail and from the user's perspective functions and closures are indistinguishable.
class Function final
{
public:

	explicit Function(String name) : _name(std::move(name)) { }

	Function(const Function &) = delete;

	Function(Function &&) noexcept = default;

	Function(String name, std::shared_ptr<Callable> r);

	Function(const String &name, NativeCallback cb, std::initializer_list<Handle<Class>> sig, ParamBitset ref_flags = 0);

	String name() const { return _name; }

	void add_routine(std::shared_ptr<Callable> r, bool create);

	std::shared_ptr<Callable> find_routine(std::span<Variant> args);

	ParamBitset reference_flags() const { return ref_flags; }

private:

	friend class Variant;
	friend class Runtime;

	// Name provided when the function was declared. Anonymous functions don't have a name.
	String _name;

	// Each function signature is represented by a different routine, which may be native or user-defined.
	std::vector<std::shared_ptr<Callable>> routines;

	ParamBitset ref_flags;

	// Maximum number of arguments that this function allows.
	int max_argc = 0;
};


//----------------------------------------------------------------------------------------------------------------------

// Instantiation of a Function that capture over its environment.
class Closure final
{
public:

private:

};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

namespace meta {

static inline
String to_string(const Function &f, bool quote, bool)
{
	auto s = String::format("<function %s at %p>", f.name().data(), std::addressof(f));
	if (quote) { s.prepend('"'); s.append('"'); }

	return s;
}

static inline
String to_string(const Callable &c, bool quote, bool)
{
	auto s = String::format("<function %s at %p>", c.name().data(), std::addressof(c));
	if (quote) { s.prepend('"'); s.append('"'); }

	return s;
}


} // namespace calao::meta


} // namespace calao

#endif // CALAO_FUNCTION_HPP
