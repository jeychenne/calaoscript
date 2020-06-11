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

#include <phon/runtime/variant.hpp>
#include <phon/runtime/meta.hpp>
#include <phon/runtime/class.hpp>
#include <phon/runtime/function.hpp>
#include <phon/runtime/list.hpp>

namespace phonometrica {

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
	if (this->is_string())
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
	if (this->is_string())
	{
		raw_cast<String>(*this).~String();
	}
	else if (this->is_object())
	{
		as.object->release();
	}
	else if (this->is_alias())
	{
		auto count = as.alias->ref_count - 1;
		as.alias->release();
		if (count == 1)
		{
			*this = std::move(as.alias->variant);
		}
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
	if (this->is_object() && as.object->collectable())
	{
		callback(reinterpret_cast<Collectable*>(as.object));
	}
	else if (this->is_alias())
	{
		resolve().traverse(callback);
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
	else if (v1.is_null() || v2.is_null()) {
		return false;
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
	auto s = as_string();
	if (quote && resolve().is_string()) { s.prepend('"'); s.append('"'); }

	return s;
}

String Variant::as_string() const
{
	switch (m_data_type)
	{
		case Datatype::String:
		{
			return raw_cast<String>(*this);
		}
		case Datatype::Object:
		{
			return as.object->to_string();
		}
		case Datatype::Integer:
		{
			intptr_t num = raw_cast<intptr_t>(*this);
			return meta::to_string(num);
		}
		case Datatype::Float:
		{
			double num = raw_cast<double>(*this);
			return meta::to_string(num);
		}
		case Datatype::Boolean:
		{
			bool b = raw_cast<bool>(*this);
			return meta::to_string(b);
		}
		case Datatype::Alias:
		{
			return resolve().to_string();
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
			for (auto &r : f2.closures) {
				f1.add_closure(r);
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
			return Class::get<nullptr_t>();
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
		v = &v->as.alias->variant;
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

void Variant::unalias()
{
	if (is_alias())
	{
		Variant tmp(resolve());
		as.alias->release();
		zero();
		swap(tmp);
	}
}

Variant & Variant::unshare()
{
	// Note: we don't need to handle String here because it will be unshared automatically.
	switch (data_type())
	{
		case Datatype::Object:
		{
			if (as.object->shared() && as.object->clonable())
			{
				auto obj = as.object->clone();
				as.object->release();
				as.object = obj;
			}
			// Non-clonable objects are unaffected.
			break;
		}
		case Datatype::Alias:
			resolve().unshare();
			break;
		default:
			break;
	}

	return *this;
}

bool Variant::operator<(const Variant &other) const
{
	return compare(other) < 0;
}

intptr_t Variant::to_integer() const
{
	switch (data_type())
	{
		case Datatype::Integer:
			return raw_cast<intptr_t>(*this);
		case Datatype::Float:
			return intptr_t(raw_cast<double>(*this));
		case Datatype::Boolean:
			return intptr_t(raw_cast<bool>(*this));
		case Datatype::Alias:
			return resolve().to_integer();
		default:
			throw error("[Cast error] Value of type % cannot be converted to Integer", class_name());
	}

	return 0;
}

double Variant::to_float() const
{
	switch (data_type())
	{
		case Datatype::Integer:
		case Datatype::Float:
			return get_number();
		case Datatype::Boolean:
			return double(raw_cast<bool>(*this));
		case Datatype::Alias:
			return resolve().to_float();
		default:
			throw error("[Cast error] Value of type % cannot be converted to Float", class_name());
	}

	return 0.0;
}

} // namespace phonometrica