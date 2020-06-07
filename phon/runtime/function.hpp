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
 * Purpose: Function object. Functions support multiple dispatch: a function may have several overloads, and the      *
 * correct routine is selected at runtime based on the number and types of arguments.                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef PHONOMETRICA_FUNCTION_HPP
#define PHONOMETRICA_FUNCTION_HPP

#include <bitset>
#include <functional>
#include <optional>
#include <vector>
#include <phon/runtime/typed_object.hpp>
#include <phon/runtime/variant_def.hpp>
#include <phon/runtime/code.hpp>
#include <phon/utils/span.hpp>

namespace phonometrica {

class Runtime;
class Function;
class Class;

// Flags used to distinguish references and values in function signatures.
static constexpr size_t PARAM_BITSET_SIZE = 64;
using ParamBitset = std::bitset<PARAM_BITSET_SIZE>;


//---------------------------------------------------------------------------------------------------------------------


// A native C++ callback.
using NativeCallback = std::function<Variant(Runtime &rt, std::span<Variant> args)>;

// A Callable is an internal abstract base type type used to represent one particular signature for a function. Each function has at least
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

	bool check_ref(ParamBitset ref) { return ref_flags == ref; }

	void add_parameter_type(Handle<Class> cls) { signature.push_back(std::move(cls)); }

	String name() const { return _name; }

	int get_cost(std::span<Variant> args) const;

	String get_definition() const;

protected:

	friend class Function;

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

	Variant operator()(Runtime &rt, std::span<Variant> args);
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

	// Represents a non-local variable referenced by an inner function.
	struct UpvalueSlot
	{
		// Index of the variable in the surrounding function.
		Instruction index;

		// If true, the captured upvalue is a local variable in the surrounding function; otherwise it is itself an upvalue
		// that references a local or another upvalue in its surrounding function. All non-local upvalues eventually resolve to
		// a local one.
		bool is_local;

		bool operator==(const UpvalueSlot &other) noexcept { return this->index == other.index && this->is_local == other.is_local; }
	};

	Routine(Routine *parent, const String &name, int argc);

	Routine(Routine *parent, const String &name, std::vector<Handle<Class>> sig, ParamBitset ref_flags);

	bool is_native() const override { return false; }

	Instruction add_integer_constant(intptr_t i);

	Instruction add_float_constant(double n);

	Instruction add_string_constant(String s);
	
	Instruction add_function(Handle<Function> f);

	Instruction add_routine(std::shared_ptr<Routine> r);

	Instruction add_local(const String &name, int scope, int depth);

	std::optional<Instruction> find_local(const String &name, int scope_depth) const;

	std::optional<Instruction> resolve_upvalue(const String &name, int scope_depth);

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

	void clear_signature() { signature.clear(); }

	Instruction add_upvalue(Instruction index, bool local);

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

	std::vector<UpvalueSlot> upvalues;

	// Enclosing routine (this is used to find upvalues).
	Routine *parent = nullptr;
};

//----------------------------------------------------------------------------------------------------------------------

// Instantiation of a Callable that captures over its environment.
class Closure final
{
public:

	explicit Closure(std::shared_ptr<Callable> r) : routine(std::move(r)) { }

	Variant operator()(Runtime &rt, std::span<Variant> args);

private:

	friend class Runtime;
	friend class Function;

	Variant call_native(Runtime &rt, std::span<Variant> args);

	Variant call_user(Runtime &rt, std::span<Variant> args);

	std::shared_ptr<Callable> routine;
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

	Function(String name, Handle <Closure> c);

	Function(const String &name, NativeCallback cb, std::initializer_list<Handle<Class>> sig, ParamBitset ref_flags = 0);

	String name() const { return _name; }

	void add_closure(Handle<Closure> r, bool create);

	Handle<Closure> find_closure(std::span<Variant> args);

	ParamBitset reference_flags() const { return ref_flags; }

private:

	friend class Variant;
	friend class Runtime;

	// Name provided when the function was declared. Anonymous functions don't have a name.
	String _name;

	// Each function signature is represented by a different routine, which may be native or user-defined.
	std::vector<Handle<Closure>> closures;

	ParamBitset ref_flags;

	// Maximum number of arguments that this function allows.
	int max_argc = 0;
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


} // namespace phonometrica::meta


} // namespace phonometrica

#endif // PHONOMETRICA_FUNCTION_HPP