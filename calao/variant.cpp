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
		unsafe_cast<String>(*this).impl->retain();
	}
	else if (this->is_object())
	{
		as.obj->retain();
	}
}

void Variant::release()
{
	if (check_type<String>(*this))
	{
		unsafe_cast<String>(*this).~String();
	}
	else if (this->is_object())
	{
		as.obj->release();
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
			return as.obj->type_info();
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
	static String null("null");

	switch (data_type())
	{
		case Datatype::String:
			return Class::get_name<String>();
		case Datatype::Object:
			return as.obj->class_name();
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
	if (this->is_object() and as.obj->collectable())
	{
		callback(reinterpret_cast<Collectable*>(as.obj));
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
				auto &s1 = unsafe_cast<String>(v1);
				auto &s2 = unsafe_cast<String>(v2);

				return s1 == s2;
			}
			case Datatype::Object:
			{
				auto o1 = v1.as.obj;
				auto o2 = v2.as.obj;

				if (o1->get_class() != o2->get_class()) {
					break;
				}

				return o1->equal(o2);
			}
			case Datatype::Integer:
			{
				auto x = unsafe_cast<intptr_t>(v1);
				auto y = unsafe_cast<intptr_t>(v2);

				return x == y;
			}
			case Datatype::Float:
			{
				auto x = unsafe_cast<double>(v1);
				auto y = unsafe_cast<double>(v2);

				return meta::equal(x, y);
			}
			case Datatype::Boolean:
			{
				auto x = unsafe_cast<bool>(v1);
				auto y = unsafe_cast<bool>(v2);

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
				auto &s1 = unsafe_cast<String>(v1);
				auto &s2 = unsafe_cast<String>(v2);

				return s1.compare(s2);
			}
			case Datatype::Object:
			{
				auto o1 = v1.as.obj;
				auto o2 = v2.as.obj;

				// TODO: handle subclasses in comparison
				if (o1->get_class() != o2->get_class()) {
					break;
				}

				return o1->compare(o2);
			}
			case Datatype::Integer:
			{
				auto x = unsafe_cast<intptr_t>(v1);
				auto y = unsafe_cast<intptr_t>(v2);

				return meta::compare(x, y);
			}
			case Datatype::Float:
			{
				auto x = unsafe_cast<double>(v1);
				auto y = unsafe_cast<double>(v2);

				return meta::compare(x, y);
			}
			case Datatype::Boolean:
			{
				auto x = unsafe_cast<bool>(v1);
				auto y = unsafe_cast<bool>(v2);

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
		return unsafe_cast<double>(*this);
	assert(data_type() == Datatype::Integer);
	auto i = unsafe_cast<intptr_t>(*this);

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

String Variant::to_string(bool quote) const
{
	switch (m_data_type)
	{
		case Datatype::String:
		{
			return unsafe_cast<String>(*this);
		}
		case Datatype::Object:
		{
			bool seen = as.obj->is_seen();
			as.obj->mark_seen(true);
			auto s = as.obj->to_string(quote, seen);
			as.obj->mark_seen(seen);

			return s;
		}
		case Datatype::Integer:
		{
			intptr_t num = unsafe_cast<intptr_t>(*this);
			return meta::to_string(num, quote);
		}
		case Datatype::Float:
		{
			double num = unsafe_cast<double>(*this);
			return meta::to_string(num, quote);
		}
		case Datatype::Boolean:
		{
			bool b = unsafe_cast<bool>(*this);
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

Variant &Variant::operator=(Variant &&other) noexcept
{
	swap(other);
	return *this;
}

Variant &Variant::operator=(const Variant &other) noexcept
{
	if (this != &other)
	{
		Variant tmp(other);
		swap(tmp);
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
			return as.obj->get_class();
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
			return unsafe_cast<String>(*this).hash();
		case Datatype::Integer:
			return meta::hash(static_cast<uint64_t>((unsafe_cast<intptr_t>(*this))));
		case Datatype::Float:
			return meta::hash(static_cast<uint64_t>((unsafe_cast<double>(*this))));
		case Datatype::Object:
			return as.obj->hash();
		case Datatype::Boolean:
			return unsafe_cast<bool>(*this) ? 3 : 7;
		case Datatype::Alias:
			return resolve().hash();
		case Datatype::Null:
			throw error("[Type error] Null value is not hashable");
	}

	return 0; // please GCC
}

void Variant::make_alias()
{
	auto alias = new Alias(std::move(*this));
	as.alias = alias;
	m_data_type = Datatype::Alias;
}

Variant &Variant::resolve()
{
	Variant *v = this;

	while (this->is_alias())
	{
		v = &as.alias->variant;
	}

	return *v;
}

const Variant &Variant::resolve() const
{
	return const_cast<Variant*>(this)->resolve();
}


} // namespace calao