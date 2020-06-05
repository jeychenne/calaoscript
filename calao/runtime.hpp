/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 23/05/2020                                                                                                *
 *                                                                                                                    *
 * Purpose: a runtime encapsulates a virtual machine that can execute Calao code. There can be several runtimes in an *
 * OS thread, but runtimes must not be shared across threads.                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_RUNTIME_HPP
#define CALAO_RUNTIME_HPP

#include <type_traits>
#include <unordered_set>
#include <calao/string.hpp>
#include <calao/class.hpp>
#include <calao/meta.hpp>
#include <calao/typed_object.hpp>
#include <calao/internal/recycler.hpp>
#include <calao/internal/iterator.hpp>
#include <calao/class.hpp>
#include <calao/list.hpp>
#include <calao/table.hpp>
#include <calao/function.hpp>
#include <calao/module.hpp>
#include <calao/internal/parser.hpp>
#include <calao/internal/code.hpp>
#include <calao/internal/compiler.hpp>

namespace calao {

class Class;
class Object;
class Collectable;


class Runtime final
{
	// Provide generic methods for each type registered with the runtime. The following methods will be added
	// to a Class when it is created. Most of these methods are proxies that call templated functions
	// defined in meta.hpp.

	// VTable for non-primitive types.
	template<typename T>
	struct VTable
	{
		static void destroy(Object *o)
		{
			delete reinterpret_cast<TObject<T>*>(o);
		}

		static void traverse(Collectable *o, const GCCallback &callback)
		{
			auto obj = reinterpret_cast<TObject<T>*>(o);
			meta::traverse(obj->value(), callback);
		}

		static Object *clone(const Object *o)
		{
			auto obj = reinterpret_cast<const TObject<T> *>(o);

			if constexpr (traits::is_collectable<T>::value) {
				return new TObject<T>(reinterpret_cast<const Collectable*>(obj)->runtime, obj->value());
			}
			else {
				return new TObject<T>(obj->value());
			}
		}

		static String to_string(const Object *o)
		{
			auto obj = reinterpret_cast<const TObject<T> *>(o);
			return meta::to_string(obj->value());
		}

		static int compare(const Object *o1, const Object *o2)
		{
			assert(o1->get_class() == o2->get_class());
			auto obj1 = reinterpret_cast<const TObject<T> *>(o1);
			auto obj2 = reinterpret_cast<const TObject<T> *>(o2);

			return meta::compare(obj1->value(), obj2->value());
		}

		static bool equal(const Object *o1, const Object *o2)
		{
			assert(o1->get_class() == o2->get_class());
			auto obj1 = reinterpret_cast<const TObject<T> *>(o1);
			auto obj2 = reinterpret_cast<const TObject<T> *>(o2);

			return meta::equal(obj1->value(), obj2->value());
		}
	};

public:

	Runtime();

	~Runtime();

	template<typename T>
	Handle<Class> create_type(const char *name, Class *base, Class::Index index = Class::Index::Foreign)
	{
		// Sanity checks.
		static_assert(!(std::is_same<T, String>::value && traits::is_collectable<T>::value), "String is not collectable");
		static_assert(!(std::is_same<T, File>::value && traits::is_collectable<T>::value), "File is not collectable");

		using Type = typename traits::bare_type<T>::type;
		auto klass = make_handle<Class>(this, name, base, &typeid(Type), index);
		classes.push_back(klass);
		klass->set_object(classes.back().object());

		// Register statically known type so that we can call Class::get<T>() to retrieve a type's class.
		detail::ClassDescriptor<T>::set(klass.get());

		// Object is an abstract type
		if constexpr (traits::is_boxed<T>::value && !std::is_same<T, Object>::value)
		{
			// Add generic methods
			klass->destroy   = &VTable<T>::destroy;
			klass->to_string = &VTable<T>::to_string;
			klass->compare   = &VTable<T>::compare;
			klass->equal     = &VTable<T>::equal;

			if constexpr (traits::is_collectable<T>::value)
			{
				klass->traverse  = &VTable<T>::traverse;
			}

			if constexpr (traits::is_clonable<T>::value)
			{
				klass->clone  = &VTable<T>::clone;
			}
		}

		return klass;
	}

	template<class T, class... Args>
	Handle<T> create(Args... args)
	{
		if constexpr (traits::is_collectable<T>::value) {
			return Handle<T>(this, std::forward<Args>(args)...);
		}
		else {
			return Handle<T>(std::forward<Args>(args)...);
		}
	}

	void push_null();

	// Push null on stack, return address.
	Variant &push();

	// Push number on stack.
	void push(double n);

	// Push an integer onto the stack. This method doesn't overload push() because there would be an
	// ambiguity between double and intptr_t when pushing an integer literal, on platforms where int != intptr_t.
	void push_int(intptr_t n);

	// Push boolean onto the stack.
	void push(bool b);

	// Push a variant onto the stack.
	void push(const Variant &v);
	void push(Variant &&v);

	// Push a string onto the stack.
	void push(String s);

	void pop(int n = 1);

	template<class T>
	void push(Handle<T> value)
	{
		new(var()) Variant(std::move(value));
	}

	Variant & peek(int n = -1);

	Variant interpret(Closure &closure);

	// Call a user-defined function in Opcode::Call.
	Variant interpret(Closure &closure, std::span<Variant> args);

	void disassemble(const Closure &closure, const String &name);

	void disassemble(const Routine &routine, const String &name);

	Variant do_file(const String &path);

	Handle<Closure> compile_file(const String &path);

	String intern_string(const String &s);

	void add_global(String name, Variant value);

	void add_global(const String &name, NativeCallback cb, std::initializer_list<Handle<Class>> sig, ParamBitset ref = ParamBitset());

	bool needs_reference() const;

	Variant &operator[](const String &key);

	bool debug_mode() const;

	void set_debug_mode(bool value);

private:

	struct CallFrame
	{
		// Return address in the caller.
		const Instruction *ip = nullptr;

		// Routine being called
		const Routine *previous_routine = nullptr;

		// Arguments and local variables on the stack.
		Variant *locals = nullptr;

		// Reference flags (only used to prepare a call).
		ParamBitset ref_flags;

		// Number of local variables.
		int nlocal = -1;
	};

	friend class Collectable;

	void clear();

	void add_candidate(Collectable *obj);

	void remove_candidate(Collectable *obj);

	void create_builtins();

	void set_global_namespace();

	void check_capacity();

	void ensure_capacity(int n);

	void resize_stack();

	void check_underflow();

	Variant *var();

	size_t disassemble_instruction(const Routine &routine, size_t offset);

	static size_t print_simple_instruction(const char *name);

	void negate();

	void math_op(char op);

	static void check_float_error();

	int get_current_line() const;

	void push_call_frame(int nlocal);

	Variant pop_call_frame();

	void get_index(bool by_ref);

	// Garbage collector.
	Recycler gc;

	// Builtin classes (known at compile time).
	std::vector<Handle<Class>> classes;

	// Runtime stack.
	Array<Variant> stack;

	// Top of the stack. This is one slot past the last element currently on the stack.
	Variant *top;

	// End of the stack array.
	Variant *limit;

	// Routine which is being executed.
	const Routine *current_routine = nullptr;

	// Instruction pointer.
	const Instruction *ip = nullptr;

	// Currently executing code chunk.
	const Code *code = nullptr;

	// Parses source code to an AST.
	Parser parser;

	// Compiles source code to byte code for the runtime.
	Compiler compiler;

	// Interned strings.
	std::unordered_set<String> strings;

	// Global variables.
	Handle<Module> globals;

	// Stack of call frames.
	std::vector<std::unique_ptr<CallFrame>> frames;

	// Current call frame.
	CallFrame *current_frame = nullptr;

	// Runtime options.
	bool debugging = true;

	// Flag to let functions know whether a reference is requested.
	bool needs_ref = false;

	// Global initialization.
	static bool initialized;
};

} // namespace calao

#endif // CALAO_RUNTIME_HPP
