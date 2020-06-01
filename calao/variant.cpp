/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 12/07/2019                                                                                                *
 *                                                                                                                    *
 * Purpose: see header.                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include <calao/variant.hpp>
#include <calao/meta.hpp>
#include <calao/class.hpp>
#include <calao/function.hpp>

namespace calao {

Variant::Variant() :
		m_data_type(Datatype::Null)
{

}

Variant::Variant(bool val) :
		m_data_type(Datatype::Boolean)
{
	new (&as.storage) bool(val);
}

Variant::Variant(intptr_t val) :
		m_data_type(Datatype::Integer)
{
	new (&as.storage) intptr_t(val);
}

Variant::Variant(Object *obj) :
		m_data_type(Datatype::Object)
{
	obj->retain();
	as.object = obj;
}


Variant::Variant(double val) :
		m_data_type(Datatype::Float)
{
	new (&as.storage) double(val);
}

Variant::Variant(const Variant &other)
{
	copy_fields(other);
	retain();
}

Variant::Variant(Variant &other)
{
	copy_fields(other);
	retain();
}

Variant::Variant(Variant &&other) noexcept
{
	copy_fields(other);
	other.zero();
}

Variant::~Variant()
{
	this->release();
}

void Variant::retain()
{
	if (check_type<String>(*this))
	{
		raw_cast<String>(*this).impl->retain();
	}
	else if (this->is_object())
	{
		as.object->retain();
	}
	else if (this->is_alias())
	{
		as.alias->retain();
	}
}

void Variant::release()
{
	if (check_type<String>(*this))
	{
		raw_cast<String>(*this).~String();
	}
	else if (this->is_object())
	{
		as.object->release();
	}
	else if (this->is_alias())
	{
		as.alias->release();
	}
}

void Variant::swap(Variant &other) noexcept
{
	std::swap(m_data_type, other.m_data_type);
	std::swap(as, other.as);
}

bool Variant::empty() const
{
	return m_data_type == Datatype::Null;
}

void Variant::zero()
{
	m_data_type = Datatype::Null;
}

void Variant::clear()
{
	release();
	zero();
}

void Variant::copy_fields(const Variant &other)
{
	m_data_type = other.m_data_type;
	as = other.as;
}

bool Variant::is_object() const
{
	return m_data_type == Datatype::Object;
}

const std::type_info *Variant::type_info() const
{
	switch (m_data_type)
	{
		case Datatype::String:
			return &typeid(String);
		case Datatype::Object:
			return as.object->type_info();
		case Datatype::Integer:
			return &typeid(intptr_t);
		case Datatype::Float:
			return &typeid(double);
		case Datatype::Boolean:
			return &typeid(bool);
		case Datatype::Alias:
			return resolve().type_info();
		case Datatype::Null:
			return &typeid(nullptr_t);
		default:
			break;
	}

	throw error("[Internal error] Invalid type ID in type_info function");
}

String Variant::class_name() const
{
	static String null("Null");

	switch (data_type())
	{
		case Datatype::String:
			return Class::get_name<String>();
		case Datatype::Object:
			return as.object->class_name();
		case Datatype::Integer:
			return Class::get_name<intptr_t>();
		case Datatype::Float:
			return Class::get_name<double>();
		case Datatype::Boolean:
			return Class::get_name<bool>();
		case Datatype::Alias:
			return resolve().class_name();
		case Datatype::Null:
			return null;
		default:
			break;
	}

	throw error("[Internal error] Invalid type ID in class_name function");
}

void Variant::traverse(const GCCallback &callback)
{
	if (this->is_object() and as.object->collectable())
	{
		callback(reinterpret_cast<Collectable*>(as.object));
	}
}

bool Variant::operator==(const Variant &other) const
{
	auto &v1 = this->resolve();
	auto &v2 = other.resolve();

	if (v1.data_type() == v2.data_type())
	{
		switch (v1.data_type())
		{
			case Datatype::String:
			{
				auto &s1 = raw_cast<String>(v1);
				auto &s2 = raw_cast<String>(v2);

				return s1 == s2;
			}
			case Datatype::Object:
			{
				auto o1 = v1.as.object;
				auto o2 = v2.as.object;

				if (o1->get_class() != o2->get_class()) {
					break;
				}

				return o1->equal(o2);
			}
			case Datatype::Integer:
			{
				auto x = raw_cast<intptr_t>(v1);
				auto y = raw_cast<intptr_t>(v2);

				return x == y;
			}
			case Datatype::Float:
			{
				auto x = raw_cast<double>(v1);
				auto y = raw_cast<double>(v2);

				return meta::equal(x, y);
			}
			case Datatype::Boolean:
			{
				auto x = raw_cast<bool>(v1);
				auto y = raw_cast<bool>(v2);

				return x == y;
			}
			case Datatype::Null:
				return true;
			default:
				break;
		}
	}
	else if (v1.is_number() && v2.is_number())
	{
		auto x = v1.get_number();
		auto y = v2.get_number();

		return meta::equal(x, y);
	}

	throw error("[Type error] Cannot compare values of type % and %", this->class_name(), other.class_name());
}


int Variant::compare(const Variant &other) const
{
	auto &v1 = this->resolve();
	auto &v2 = other.resolve();

	if (v1.data_type() == v2.data_type())
	{
		switch (v1.data_type())
		{
			case Datatype::String:
			{
				auto &s1 = raw_cast<String>(v1);
				auto &s2 = raw_cast<String>(v2);

				return s1.compare(s2);
			}
			case Datatype::Object:
			{
				auto o1 = v1.as.object;
				auto o2 = v2.as.object;

				// TODO: handle subclasses in comparison
				if (o1->get_class() != o2->get_class()) {
					break;
				}

				return o1->compare(o2);
			}
			case Datatype::Integer:
			{
				auto x = raw_cast<intptr_t>(v1);
				auto y = raw_cast<intptr_t>(v2);

				return meta::compare(x, y);
			}
			case Datatype::Float:
			{
				auto x = raw_cast<double>(v1);
				auto y = raw_cast<double>(v2);

				return meta::compare(x, y);
			}
			case Datatype::Boolean:
			{
				auto x = raw_cast<bool>(v1);
				auto y = raw_cast<bool>(v2);

				return meta::compare(x, y);
			}
			case Datatype::Null:
				return 0;
			default:
				break;
		}
	}
	else if (v1.is_number() && v2.is_number())
	{
		auto x = v1.get_number();
		auto y = v2.get_number();

		return meta::compare(x, y);
	}

	throw error("[Type error] Cannot compare values of type % and %", this->class_name(), other.class_name());
}

double Variant::get_number() const
{
	if (data_type() == Datatype::Float)
		return raw_cast<double>(*this);
	assert(data_type() == Datatype::Integer);
	auto i = raw_cast<intptr_t>(*this);

	if constexpr (meta::is_arch32)
	{
		return i; // 32 bit integers can be safely converted to double.
	}
	if (unlikely(i < smallest_integer || i > largest_integer))
	{
		throw error("[Cast error] Integer value cannot be converted to Float: magnitude too large");
	}

	return double(i);
}

bool Variant::operator!=(const Variant &other) const
{
	return not (*this == other);
}

bool Variant::to_boolean() const
{
	// There are only 3 values that evaluate to false: null, false and nan. Everything else is true.
	switch (m_data_type)
	{
		case Datatype::Boolean:
			return raw_cast<bool>(*this);
		case Datatype::Null:
			return false;
		case Datatype::Float:
			return !std::isnan(raw_cast<double>(*this));
		case Datatype::Alias:
			return resolve().to_boolean();
		default:
			return true;
	}
}

String Variant::to_string(bool quote) const
{
	switch (m_data_type)
	{
		case Datatype::String:
		{
			auto s = raw_cast<String>(*this);
			if (quote) { s.prepend('"'); s.append('"'); }

			return s;
		}
		case Datatype::Object:
		{
			bool seen = as.object->is_seen();
			as.object->mark_seen(true);
			auto s = as.object->to_string(quote, seen);
			as.object->mark_seen(seen);

			return s;
		}
		case Datatype::Integer:
		{
			intptr_t num = raw_cast<intptr_t>(*this);
			return meta::to_string(num, quote);
		}
		case Datatype::Float:
		{
			double num = raw_cast<double>(*this);
			return meta::to_string(num, quote);
		}
		case Datatype::Boolean:
		{
			bool b = raw_cast<bool>(*this);
			return meta::to_string(b, quote);
		}
		case Datatype::Alias:
		{
			return resolve().to_string(quote);
		}
		case Datatype::Null:
		{
			static String null("null");
			return null;
		}
	}

	throw error("[Internal error] Invalid type ID in to_string function");
}

Variant &Variant::operator=(Variant other)
{
	auto &self = resolve();

	if (check_type<Function>(self) && check_type<Function>(other))
	{
		auto &f1 = raw_cast<Function>(self);
		auto &f2 = raw_cast<Function>(other);

		if (&f1 != &f2)
		{
			for (auto &r : f2.routines) {
				f1.add_routine(r, false);
			}
		}
	}
	else
	{
		self.swap(other);
	}

	return *this;
}

Variant::Variant(String s) :
	m_data_type(Datatype::String)
{
	new (&as.storage) String(std::move(s));
}

Class *Variant::get_class() const
{
	switch (data_type())
	{
		case Datatype::String:
			return Class::get<String>();
		case Datatype::Integer:
			return Class::get<intptr_t>();
		case Datatype::Float:
			return Class::get<double>();
		case Datatype::Boolean:
			return Class::get<bool>();
		case Datatype::Null:
			return Class::get<void>();
		case Datatype::Object:
			return as.object->get_class();
		case Datatype::Alias:
			return resolve().get_class();
	}

	throw error("[Internal error] Invalid variant ID in type method");
}

size_t Variant::hash() const
{
	switch (data_type())
	{
		case Datatype::String:
			return raw_cast<String>(*this).hash();
		case Datatype::Integer:
			return meta::hash(static_cast<uint64_t>((raw_cast<intptr_t>(*this))));
		case Datatype::Float:
			return meta::hash(static_cast<uint64_t>((raw_cast<double>(*this))));
		case Datatype::Object:
			return as.object->hash();
		case Datatype::Boolean:
			return raw_cast<bool>(*this) ? 3 : 7;
		case Datatype::Alias:
			return resolve().hash();
		case Datatype::Null:
			throw error("[Type error] Null value is not hashable");
	}

	return 0; // please GCC
}

Variant & Variant::make_alias()
{
	if (!this->is_alias())
	{
		auto alias = new Alias(std::move(*this));
		as.alias = alias;
		m_data_type = Datatype::Alias;
	}

	return *this;
}

Variant &Variant::resolve()
{
	Variant *v = this;

	while (v->is_alias())
	{
		v = &as.alias->variant;
	}

	return *v;
}

const Variant &Variant::resolve() const
{
	return const_cast<Variant*>(this)->resolve();
}

void Variant::finalize()
{
	release();
	zero();
}


} // namespace calao