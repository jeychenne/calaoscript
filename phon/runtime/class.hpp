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
 * Purpose: Class object. Each object stores a pointer to its class, which provides runtime type information (RTTI)   *
 * and basic polymorphic methods.                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef PHONOMETRICA_CLASS_HPP
#define PHONOMETRICA_CLASS_HPP

#include <typeinfo>
#include <vector>
#include <phon/string.hpp>
#include <phon/runtime/typed_object.hpp>
#include <phon/runtime/function.hpp>
#include <phon/dictionary.hpp>
#include <phon/runtime/variant_def.hpp>

namespace phonometrica {

// Classes are objects too.
class Class final
{

public:

	enum class Index
	{
		Object,
		Class,
		Null,
		Boolean,
		Number,
		Integer,
		Float,
		String,
		Regex,
		List,
		Array,
		Table,
		Set,
		File,
		Function,
		Closure,
		Module,
		Iterator,
		ListIterator,
		TableIterator,
		StringIterator,
		FileIterator,
		RegexIterator,
		Foreign
	};


	Class(String name, Class *parent, const std::type_info *info, Index index = Index::Foreign);

	Class(const Class &) = delete;

	Class(Class &&) = delete;

	~Class() = default;

	String name() const { return _name; }

	size_t depth() const { return _depth; }

	bool inherits(const Class *base) const;

	int get_distance(const Class *base) const;

	const std::type_info *type_info() const { return _info; }

	template<class T>
	static Class *get()
	{
		using Type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
		return detail::ClassDescriptor<Type>::get();
	}

	template<class T>
	static String get_name()
	{
		return get<T>()->name();
	}

	bool operator==(const Class &other) const { return this == &other; }

	Object *object() { return _object; }

	Handle<Function> get_constructor();

	Handle<Function> get_method(const String &name);

	void add_initializer(NativeCallback cb, std::initializer_list<Handle<Class>> sig, ParamBitset ref = ParamBitset());

	void add_initializer(Handle<Function> f);

	void add_method(const String &name, NativeCallback cb, std::initializer_list<Handle<Class>> sig, ParamBitset ref = ParamBitset());

	void add_method(const String &name, Handle<Function> f);

private:

	friend class Runtime;
	friend class Object;

	// Polymorphic methods for type erasure.
	void(*destroy)(Object*) = nullptr;
	size_t (*hash)(const Object*) = nullptr;
	void (*traverse)(Collectable*, const GCCallback&) = nullptr;
	Object *(*clone)(const Object*) = nullptr;
	String (*to_string)(const Object*) = nullptr;
	int (*compare)(const Object*, const Object*) = nullptr;
	bool (*equal)(const Object*, const Object*) = nullptr;

	void set_object(Object *o) { _object = o; }

	// We need to manually finalize members that refer to a class before classes are finalized by the runtime's destructor.
	void finalize();

	// Name given to the class when it was created
	String _name;

	// Pointer to the object the class is wrapped in.
	Object *_object = nullptr;

	// Inheritance depth (0 for Object, 1 for direct subclasses of Object, etc.).
	size_t _depth;

	// C++ type for builtin types (null for user-defined types). This is used to safely downcast
	// objects to TObject<T>. Since [object] doesn't use C++'s virtual inheritance, we can't
	// use dynamic_cast for that purpose.
	const std::type_info *_info;

	// Non-owning array of Class pointers, representing the class's inheritance hierarchy. The first element represents
	// the top-most class, and is always Object. The last element is the class itself. This allows constant-time lookup
	// using the class's inheritance depth.
	std::vector<Class*> _bases;

	Dictionary<Variant> members;

	// For debugging.
	Index index;

	static String init_string;
};


namespace meta {

static inline
String to_string(const Class &klass)
{
	return String::format("<class %s>", klass.name().data());
}

} // namespace phonometrica::meta

} // namespace phonometrica

#endif // PHONOMETRICA_CLASS_HPP
