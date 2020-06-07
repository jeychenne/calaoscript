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
 * Purpose: A polymorphic container for any value, similar to C++17 std::any. Primitive phon types                   *
 * (corresponding to null, bool, Integer, Float, String) are stored unboxed, along with their type. Non-primitive     *
 * types (e.g. Table) are implemented as a managed pointer to a boxed object.                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef PHONOMETRICA_VARIANT_HPP
#define PHONOMETRICA_VARIANT_HPP

#include <phon/string.hpp>
#include <phon/runtime/typed_object.hpp>
#include <phon/runtime/variant-def.hpp>
#include <phon/runtime/class.hpp>

namespace phonometrica {


template<class T>
bool check_type(const Variant &v)
{
	auto &var = v.resolve();
	using Type = typename traits::bare_type<T>::type;
	return var.is_object() and var.type_info() == &typeid(Type);
}

template<>
inline bool check_type<bool>(const Variant &var)
{
	return var.resolve().data_type() == Variant::Datatype::Boolean;
}

template<>
inline bool check_type<intptr_t>(const Variant &var)
{
	return var.resolve().data_type() == Variant::Datatype::Integer;
}

template<>
inline bool check_type<double>(const Variant &var)
{
	return var.resolve().data_type() == Variant::Datatype::Float;
}

template<>
inline bool check_type<String>(const Variant &var)
{
	return var.resolve().data_type() == Variant::Datatype::String;
}

//------------------------------------------------------------------------------------------------------------------

template<class T>
T &raw_cast(Variant &var)
{
	return reinterpret_cast<Handle<T> &>(var.resolve().as.storage).value();
}


template<class T>
const T &raw_cast(const Variant &var)
{
	return raw_cast<T>(const_cast<Variant &>(var));
}

template<>
inline bool &raw_cast<bool>(Variant &var)
{
	return reinterpret_cast<bool &>(var.resolve().as.storage);
}

template<>
inline intptr_t &raw_cast<intptr_t>(Variant &var)
{
	return reinterpret_cast<intptr_t &>(var.resolve().as.storage);
}

template<>
inline double &raw_cast<double>(Variant &var)
{
	return reinterpret_cast<double &>(var.resolve().as.storage);
}

template<>
inline String &raw_cast<String>(Variant &var)
{
	return reinterpret_cast<String &>(var.resolve().as.storage);
}

//------------------------------------------------------------------------------------------------------------------

template<class T>
T &cast(Variant &v)
{
	auto &var = v.resolve();
	using Type = typename traits::bare_type<T>::type;
	assert(var.data_type() == Variant::Datatype::Object);
	auto ptr = reinterpret_cast<TObject<Type> *>(var.as.object);

	if (ptr->type_info() != &typeid(Type))
	{
		throw error("[Cast error] Expected a %, got a %", Class::get_name<Type>(), var.class_name());
	}

	return ptr->value();
}


template<class T>
const T &cast(const Variant &var)
{
	return cast<T>(const_cast<Variant &>(var));
}

template<>
inline bool &cast<bool>(Variant &v)
{
	auto &var = v.resolve();

	if (!check_type<bool>(var))
	{
		throw error("[Cast error] Expected a %, got a %", Class::get_name<bool>(), var.class_name());
	}

	return raw_cast<bool>(var);
}

template<>
inline intptr_t &cast<intptr_t>(Variant &v)
{
	auto &var = v.resolve();

	if (!check_type<intptr_t>(var))
	{
		throw error("[Cast error] Expected a %, got a %", Class::get_name<intptr_t>(), var.class_name());
	}

	return raw_cast<intptr_t>(var);
}

template<>
inline double &cast<double>(Variant &v)
{
	auto &var = v.resolve();

	if (!check_type<double>(var))
	{
		throw error("[Cast error] Expected a %, got a %", Class::get_name<double>(), var.class_name());
	}

	return raw_cast<double>(var);
}

template<>
inline String &cast<String>(Variant &v)
{
	auto &var = v.resolve();

	if (!check_type<String>(var))
	{
		throw error("[Cast error] Expected a %, got a %", Class::get_name<String>(), var.class_name());
	}

	return raw_cast<String>(var);
}

//------------------------------------------------------------------------------------------------------------------

// Aliases are used to implement references. A value that is shared by several references is stored in an alias instead
// of being stored in a variant directly.
struct Alias final
{
	explicit Alias(Variant v) : ref_count(1), variant(std::move(v)) { }

	~Alias() = default;

	void retain() { ++ref_count; }

	void release()
	{
		if (--ref_count == 0) {
			delete this;
		}
	}

	int32_t ref_count;
	Variant variant;
};


inline
std::ostream &operator<<(std::ostream &stream, const Variant &v)
{
	auto s = v.to_string();
	stream.write(s.data(), s.size());

	return stream;
}

} // namespace phonometrica


//----------------------------------------------------------------------------------------------------------------------

namespace std {

template<>
struct hash<phonometrica::Variant>
{
	size_t operator()(const phonometrica::Variant &v) const
	{
		return v.hash();
	}
};

} // namespace std

#endif // PHONOMETRICA_VARIANT_HPP
