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
 * Purpose: Object type. The mother of all Phonometrica objects.                                                      *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef PHONOMETRICA_OBJECT_HPP
#define PHONOMETRICA_OBJECT_HPP

#include <phon/string.hpp>

namespace phonometrica {

class Class;
class Runtime;

// Flag for the backup garbage collector.  The algorithm implements the synchronous Recycler
// described in _The Garbage Collection Handbook. The Art of Automatic Memory Management_, by R. Jones,
// A. Hosking & E. Moss. (See p. 66 ff.). This algorithm was first proposed by Bacon & Rajan (2001). It is
// used to break reference cycles.
// Objects that are acyclic (i.e. contain no cyclic reference) are marked as green and are not attached to
// the GC chain. Base types such as String and Regex are considered acyclic because there is no way they
// can store a reference to themselves. Collections (List, Table, etc.) are GC candidates because they are
// considered potentially cyclic. Collectable objects start their life as white, and are marked as black
// during the mark phase. The remaining white objects are deleted during the sweep phase.
enum class GCColor : uint8_t
{
	Green,      // object which is not collectable
	Black,      // Assumed to be alive
	Grey,       // Possible member of a GC cycle
	White,      // Possibly dead
	Purple      // Root candidate for a GC cycle
};


// Primitive values and strings are always unboxed. Non-primitive types (roughly speaking objects which are larger than
// the size of a double), are boxed in a Handle<T> template, which behaves pretty much like a shared_ptr. Internally, a
// Handle<T> contains a pointer to a heap-allocated subclass of the abstract base class [Object].
//
// All non-primitive Phonometrica values derive from Object. The inheritance graph is given below:
//
//                                                         Object
//                                             (base class for all boxed values)
//                                                   |               |
//                                                Atomic          Collectable
//                                (base for acyclic objects)  (base for possibly cyclic objects)
//                                (e.g. File, Regex.       )  (e.g. List, Table.               )
//                                                      |        |
//                                                      TObject<T>
//                                       ("typed object": wrapper for boxed values, defined in typed_object.hpp)
//
// Boxed objects pointers are never manipulated explicitly in the user-visible API. Instead, they are wrapped in a
// stack-allocated Handle<T>, which manages reference counting automatically.


//----------------------------------------------------------------------------------------------------------------------

// Abstract base class for all Phonometrica objects. This class doesn't use C++'s virtual inheritance; instead, each object
// contains a pointer to a Class object which contains, among other things, function pointers that dispatch calls to
// "virtual" methods to a templated function, which can be specialized for each type. This approach gives us much more
// flexibility since we can check for the availability of a method at runtime, and we can add more information in
// a Class (in particular, an inheritance graph).
class Object
{
public:

	bool collectable() const noexcept;

	bool gc_candidate() const noexcept;

	bool clonable() const noexcept;

	bool comparable() const noexcept;

	bool equatable() const noexcept;

	bool hashable() const noexcept;

	bool traversable() const noexcept;

	bool printable() const noexcept;

	void retain() noexcept { add_reference(); }

	void release();

	void add_reference() noexcept { ++ref_count; }

	bool remove_reference() noexcept;

	bool shared() const noexcept;

	bool unique() const noexcept;

	int32_t use_count() const noexcept;

	bool is_used() const { return use_count() > 0; }

	Class *get_class() const { return klass; }

	size_t hash() const;

	String to_string() const;

	int compare(const Object *other) const;

	Object *clone() const;

	void traverse(const GCCallback &callback);

	bool equal(const Object *other) const;

	String class_name() const;

	const std::type_info *type_info() const;

protected:

	friend class Runtime;

	// Only used to bootstrap the class system.
	void set_class(Class *klass) { this->klass = klass; }

	Object(Class *klass, bool collectable);

	void destroy();

	bool is_black() const
	{
		return gc_color == GCColor::Black;
	}

	bool is_grey() const
	{
		return gc_color == GCColor::Grey;
	}

	bool is_white() const
	{
		return gc_color == GCColor::White;
	}

	bool is_purple() const
	{
		return gc_color == GCColor::Purple;
	}

	bool is_green() const
	{
		return gc_color == GCColor::Green;
	}

	void mark_black()
	{
		assert(gc_color != GCColor::Green);
		gc_color = GCColor::Black;
	}

	void mark_grey()
	{
		assert(gc_color != GCColor::Green);
		gc_color = GCColor::Grey;
	}

	void mark_white()
	{
		assert(gc_color != GCColor::Green);
		gc_color = GCColor::White;
	}

	void mark_purple()
	{
		assert(gc_color != GCColor::Green);
		gc_color = GCColor::Purple;
	}

	// Pointer to the object's class. This provides runtime type information and polymorphic methods.
	Class *klass;

	// Reference count. The object is automatically destroyed when this reaches 0.
	int32_t ref_count;

	// Information for the garbage collector.
	GCColor gc_color;
};


//---------------------------------------------------------------------------------------------------------------------

// Abstract base class for all non-collectable objects (e.g. Regex, File). An atomic object cannot contain cyclic
// references (i.e. references to itself).
class Atomic : public Object
{
protected:

	Atomic(Class *klass);
};


//---------------------------------------------------------------------------------------------------------------------

// Abstract base class for all collectable objects (e.g. List, Table). Such objects may contain cyclic references.
class Collectable : public Object
{
public:

	Collectable(Class *klass, Runtime *runtime);

	~Collectable();

	bool is_candidate() const { return next != nullptr || previous != nullptr; }

private:

	friend class Runtime;
	friend class Class;
	friend class Object;

	// Runtime this object is attached to. If this is null, the object is not considered for garbage collection and
	// must not contain cyclic references.
	Runtime *runtime;

	// Doubly linked list for the GC.
	Collectable *previous = nullptr;
	Collectable *next = nullptr;
};

} // namespace phonometrica

#endif // PHONOMETRICA_OBJECT_HPP
