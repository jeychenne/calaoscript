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

#ifndef CALAO_CLASS_HPP
#define CALAO_CLASS_HPP

#include <typeinfo>
#include <vector>
#include <calao/string.hpp>

namespace calao {

// Classes are objects too.
class Class final
{
	// A template to keep track of classes known at compile time. This should not be accessed directly: use
	// Class::get<T>() instead.
	template<typename T>
	struct Descriptor
	{
		static Class *get()
		{
			// Class is null while we are bootstrapping the class system. The runtime will check that we have a valid pointer for Class.
			assert(isa || (std::is_same_v<T, Class>));
			return isa;
		}

		static void set(Class *cls)
		{
			assert(isa == nullptr);
			isa = cls;
		}

	private:

		static Class *isa;
	};

public:


	Class(String name, Class *parent, const std::type_info *info);

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
		return Descriptor<Type>::get();
	}

	template<class T>
	static String get_name()
	{
		return get<T>()->name();
	}

	bool operator==(const Class &other) const { return this == &other; }

	Object *object() { return _object; }

private:

	friend class Runtime;
	friend class Object;

	// Polymorphic methods for type erasure.
	void(*destroy)(Object*) = nullptr;
	size_t (*hash)(const Object*) = nullptr;
	void (*traverse)(Collectable*, const GCCallback&) = nullptr;
	Object *(*clone)(const Object*) = nullptr;
	String (*to_string)(const Object*, bool, bool) = nullptr;
	int (*compare)(const Object*, const Object*) = nullptr;
	bool (*equal)(const Object*, const Object*) = nullptr;

	void set_object(Object *o) { _object = o; }

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
};


template<class T>
Class *Class::Descriptor<T>::isa = nullptr;

namespace meta {

static inline
String to_string(const Class &klass, bool quote, bool)
{
	auto s = String::format("<class %s>", klass.name().data());
	if (quote) { s.prepend('"'); s.append('"'); }

	return s;
}

} // namespace calao::meta

} // namespace calao

#endif // CALAO_CLASS_HPP
