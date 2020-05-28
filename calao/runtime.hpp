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
#include <calao/variant.hpp>
#include <calao/list.hpp>
#include <calao/table.hpp>
#include <calao/dictionary.hpp>
#include <calao/internal/parser.hpp>
#include <calao/internal/code.hpp>
#include <calao/internal/compiler.hpp>
#include <calao/internal/call.hpp>

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

		static String to_string(const Object *o, bool quote, bool seen)
		{
			auto obj = reinterpret_cast<const TObject<T> *>(o);
			return meta::to_string(obj->value(), quote, seen);
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
	Class *create_type(const char *name, Class *base)
	{
		// Sanity checks.
		static_assert(!(std::is_same<T, String>::value && traits::is_collectable<T>::value), "String is not collectable");
		static_assert(!(std::is_same<T, Class>::value && traits::is_collectable<T>::value), "Class is not collectable");

		using Type = typename traits::bare_type<T>::type;
		Class *klass = new Class(name, base, &typeid(Type));
		classes.push_back(klass);


		// Register statically known type so that we can call Class::get<T>() to retrieve a type's class.
		Class::Descriptor<T>::set(klass);

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

	void interpret(const Routine &routine);

	void disassemble(const Routine &routine, const String &name);

	void do_file(const String &path);

	String intern_string(const String &s);

private:

	struct StackFrame
	{
		// Beginning of the frame.
		Variant *base;

		// Arguments and local variables on the stack.
		Variant *locals;
	};

	friend class Collectable;

	void add_candidate(Collectable *obj);

	void remove_candidate(Collectable *obj);

	void create_builtins();

	void check_capacity();

	void ensure_capacity(int n);

	void resize_stack();

	void check_underflow();

	Variant *var();

	size_t disassemble_instruction(const Routine &routine, size_t offset);

	size_t print_simple_instruction(const char *name);

	void negate();

	void math_op(char op);

	static void check_float_error();

	int get_current_line() const;

	void push_stack_frame(int nlocal);

	void pop_stack_frame();

	// Garbage collector.
	Recycler gc;

	// Builtin classes.
	std::vector<Class*> classes;

	// Runtime stack.
	Array<Variant> stack;

	// Top of the stack. This is one slot past the last element currently on the stack.
	Variant *top;

	// End of the stack array.
	Variant *limit;

	// Instruction pointer.
	const Instruction *ip;

	// Currently executing code chunk.
	const Code *code = nullptr;

	// Parses source code to an AST.
	Parser parser;

	// Compiles source code to byte code for the runtime.
	Compiler compiler;

	// Interned strings.
	std::unordered_set<String> strings;

	// Global variables.
	Dictionary<Variant> globals;

	// Stack of stack frames.
	std::vector<std::unique_ptr<StackFrame>> frames;

	StackFrame *current_frame = nullptr;

	// Global initialization.
	static bool initialized;
};

} // namespace calao

#endif // CALAO_RUNTIME_HPP
