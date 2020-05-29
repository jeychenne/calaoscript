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
 * Purpose: template that wraps a value in an object. In general, this should not be used directly. Use               *
 * Runtime::create<T>() instead.                                                                                      *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_TYPED_OBJECT_HPP
#define CALAO_TYPED_OBJECT_HPP

#include <calao/object.hpp>
#include <calao/class.hpp>
#include <calao/traits.hpp>

namespace calao {

namespace detail {

// Select base class at compile time depending on whether the type is collectable or not.
template<typename T>
using object_base = typename std::conditional<traits::is_collectable<T>::value, Collectable, Atomic>::type;

} // namespace detail


//----------------------------------------------------------------------------------------------------------------------

// All non-primitive types exposed to Calao are wrapped in a typed TObject.

template<class T>
class TObject final : public detail::object_base<T>
{
public:

	using base_type = detail::object_base<T>;

	// Constructor for non collectable objects
	template<typename ...Params>
	explicit TObject(Params &&... params) :
			base_type(Class::get<T>()), m_value(std::forward<Params>(params)...)
	{ }

	// Constructor for collectable objects
	template<typename ...Params>
	explicit TObject(Runtime *rt, Params &&... params) :
			base_type(Class::get<T>(), rt), m_value(std::forward<Params>(params)...)
	{ }

	~TObject() = default;

	T &value()
	{ return m_value; }

	const T &value() const
	{ return m_value; }


private:

	T m_value;
};


//---------------------------------------------------------------------------------------------------------------------

// Smart pointer for TObject.
template<typename T>
class Handle
{
public:

	// Dummy struct to construct a handle from a raw pointer without retaining it.
	struct Retain { };

	Handle()
	{ ptr = nullptr; }

	template<typename... Args>
	Handle(Args... args)
	{
		ptr = new TObject<T>(std::forward<Args>(args)...);
	}

	// When constructing from a raw pointer, the object is initialized with a count of 1 so we don't retain by default...
	explicit Handle(TObject<T> *value) {
		ptr = value;
	}

	// ... but we can explicitly retain if needed.
	Handle(TObject<T> *value, Retain) {
		ptr = value;
		retain();
	}

	Handle(const Handle &other) {
		ptr = other.ptr;
		retain();
	}

	Handle(Handle &&other) noexcept {
		ptr = other.ptr;
		other.zero();
	}

	~Handle() noexcept {
		release();
	}

	Handle &operator=(const Handle &other) noexcept
	{
		if (this != &other)
		{
			release();
			ptr = other.ptr;
			retain();
		}

		return *this;
	}

	Handle &operator=(Handle &&other) noexcept
	{
		std::swap(ptr, other.ptr);
		return *this;
	}

	T* get() const {
		return &ptr->value();
	}

	T& operator*() const {
		return ptr->value();
	}

	T* operator->() const {
		return &ptr->value();
	}

	operator T*() const {
		return ptr;
	}

	operator bool() const {
		return ptr != nullptr;
	}

	bool operator==(const Handle &other) const {
		return ptr == other.ptr;
	}

	bool operator!=(const Handle &other) const {
		return ptr != other.ptr;
	}

	void swap(Handle &other) noexcept {
		std::swap(ptr, other.ptr);
	}

	TObject<T> *drop()
	{
		auto tmp = ptr;
		this->zero();
		return tmp;
	}

	void zero() noexcept {
		ptr = nullptr;
	}

	T *object() {
		return ptr;
	}

	const T *object() const {
		return ptr;
	}

	T &value() {
		return ptr->value();
	}

	const T &value() const {
		return ptr->value();
	}

private:

	template<typename> friend class Handle;

	void retain() noexcept {
		if (ptr) ptr->retain();
	}

	void release() noexcept {
		if (ptr) ptr->release();
	}

	TObject<T> *ptr;

};


//---------------------------------------------------------------------------------------------------------------------

template<class T, class... Args>
Handle<T> make_handle(Args... args)
{
	return Handle<T>(new TObject<T>(std::forward<Args>(args)...));
}

} // namespace calao

#endif // CALAO_TYPED_OBJECT_HPP
