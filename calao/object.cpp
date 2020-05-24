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
 * Purpose: see header.                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include <calao/object.hpp>
#include <calao/class.hpp>
#include <calao/runtime.hpp>

namespace calao {

Object::Object(Class *klass, bool collectable) :
		klass(klass), ref_count(1), gc_color(collectable ? GCColor::Black : GCColor::Green)
{

}

void Object::release()
{
	if (remove_reference())
	{
		if (klass->destroy) {
			klass->destroy(this);
		}
	}
	else if (this->collectable())
	{
		// This might be garbage.
		reinterpret_cast<Collectable*>(this)->mark_purple();
	}
}

bool Object::remove_reference() noexcept
{
	return --ref_count == 0;
}

bool Object::shared() const noexcept
{
	return ref_count > 1;
}

bool Object::unique() const noexcept
{
	return ref_count == 1;
}

int32_t Object::use_count() const noexcept
{
	return ref_count;
}

bool Object::collectable() const noexcept
{
	return gc_color != GCColor::Green;
}

bool Object::clonable() const noexcept
{
	return klass->clone != nullptr;
}

bool Object::comparable() const noexcept
{
	return klass->compare != nullptr;
}

bool Object::equatable() const noexcept
{
	return klass->equal != nullptr;
}

bool Object::hashable() const noexcept
{
	return klass->hash != nullptr;
}

bool Object::traversable() const noexcept
{
	return klass->traverse != nullptr;
}

bool Object::printable() const noexcept
{
	return klass->to_string != nullptr;
}

size_t Object::hash() const
{
	if (this->hashable()) {
		return klass->hash(this);
	}

	throw error("[Type error] Type % is not hashable", class_name());
}

String Object::to_string(bool quote, bool seen) const
{
	if (this->printable()) {
		return klass->to_string(this, quote, seen);
	}

	throw error("[Type error] Type % cannot be converted to string", class_name());
}

Object *Object::clone() const
{
	if (this->clonable())
	{
		klass->clone(this);
	}

	throw error("[Type error] Type % is not cloneable", class_name());
}

void Object::traverse(const GCCallback &callback)
{
	if (this->traversable())
	{
		klass->traverse(reinterpret_cast<Collectable*>(this), callback);
	}
}

bool Object::equal(const Object *other) const
{
	if (this->equatable())
	{
		return klass->equal(this, other);
	}

	// Try to compare values instead. This will throw an error if comparison is not supported.
	return compare(other) == 0;
}

int Object::compare(const Object *other) const
{
	if (this->comparable())
	{
		return klass->compare(this, other);
	}

	throw error("[Type error] Type % does not support comparison", class_name());
}

String Object::class_name() const
{
	return klass->name();
}

const std::type_info *Object::type_info() const
{
	return klass->type_info();
}

//---------------------------------------------------------------------------------------------------------------------

Atomic::Atomic(Class *klass) :
	Object(klass, false)
{

}


//---------------------------------------------------------------------------------------------------------------------

Collectable::Collectable(Class *klass, Runtime *runtime) :
	Object(klass, runtime != nullptr), runtime(runtime), previous(nullptr), next(nullptr)
{
	if (runtime)
	{
		runtime->add_candidate(this);
	}
}

Collectable::~Collectable()
{
	if (runtime)
	{
		runtime->remove_candidate(this);
	}
}


} // namespace calao
