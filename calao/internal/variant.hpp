/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 04/06/2020                                                                                                *
 *                                                                                                                    *
 * Purpose: The implementation of Variant is placed in a separate file to break circular dependencies between Class   *
 * and cast<T>().                                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_VARIANT_INTERNAL_HPP
#define CALAO_VARIANT_INTERNAL_HPP

namespace calao {

struct Alias;


class Variant final
{
	enum class Datatype
	{
		Null    = 0,
		Boolean = 1 << 0,
		Integer = 1 << 1,
		Float   = 1 << 2,
		String  = 1 << 3,
		Object  = 1 << 4,
		Alias   = 1 << 5
	};

	static constexpr int number_mask = static_cast<unsigned>(Datatype::Integer) | static_cast<unsigned>(Datatype::Float);

public:

	Variant();

	Variant(const Variant &other);

	Variant(Variant &other);

	Variant(Variant &&other) noexcept;

	Variant(nullptr_t) : Variant() { }

	Variant(bool val);

	Variant(intptr_t val);

	Variant(double val);

	Variant(const char *str) : Variant(String(str)) {}

	Variant(Substring str) : Variant(String(str)) { }

	Variant(Object *obj);

	Variant(String s);

	template<class T>
	Variant(T &&val) :
			m_data_type(Datatype::Object)
	{
		using Type = typename std::remove_reference<typename std::remove_cv<T>::type>::type;
		as.object = new TObject<Type>(std::forward<Type>(val));
	}

	// Note: beware of C++'s "most vexing parse" when using this constructor. The following code:
	// 		Variant v(Handle<List>(lst));
	// won't work because it will be interpreted as a function declaration. Instead, use one of
	// the following declarations:
	//		Variant v{Handle<List>(lst)};   // brace-initializer
	//		Variant v((Handle<List>(lst))); // extra pair of parentheses
	// or better still, use the constructor which takes a universal reference and wraps the value
	// automatically, like so:
	//		Variant v(lst);
	template<class T>
	Variant(Handle<T> val) :
			m_data_type(Datatype::Object)
	{
		new(&as.storage) Handle<T>(std::move(val));
	}

	~Variant();

	void swap(Variant &other) noexcept;

	Variant &operator=(Variant other);

	Datatype data_type() const { return m_data_type; }

	Class *get_class() const;

	bool empty() const;

	void clear();

	bool is_object() const;

	bool is_integer() const { return m_data_type == Datatype::Integer; }

	bool is_float() const { return m_data_type == Datatype::Float; }

	bool is_number() const { return static_cast<unsigned >(m_data_type) & number_mask; }

	bool is_alias() const { return m_data_type == Datatype::Alias; }

	bool is_null() const { return m_data_type == Datatype::Null; }

	bool is_string() const { return m_data_type == Datatype::String; }

	const std::type_info *type_info() const;

	Variant & make_alias();

	void unalias();

	Variant &resolve();

	const Variant &resolve() const;

	String class_name() const;

	void traverse(const GCCallback &callback);

	bool operator==(const Variant &other) const;

	bool operator!=(const Variant &other) const;

	bool operator<(const Variant &other) const;

	int compare(const Variant &other) const;

	String to_string(bool quote = false) const;

	bool to_boolean() const;

	intptr_t to_integer() const;

	double to_float() const;

	size_t hash() const;

	double get_number() const;

	template<class T>
	Handle<T> handle() { return reinterpret_cast<Handle<T> &>(as.storage); }

	Variant & unshare();

private:

	friend class Runtime;

	template<class T>
	friend T &cast(Variant &var);

	template<class T>
	friend T &raw_cast(Variant &var);

	template<class T>
	friend bool check_type(const Variant &var);

	void retain();

	void release();

	void zero();

	void finalize(); // for the runtime only

	void copy_fields(const Variant &other);

	String as_string() const;

	using largest_type_t = typename std::conditional<sizeof(void *) >= sizeof(double), void *, double>::type;
	using storage_t = std::aligned_storage<sizeof(largest_type_t), alignof(largest_type_t)>::type;

	// Boxed or unboxed value.
	union Storage
	{
		storage_t storage;
		Object *object;
		Alias *alias;
	} as;

	// Type of the storage.
	Datatype m_data_type;
};

} // namespace calao

#endif // CALAO_VARIANT_INTERNAL_HPP
