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
 * Purpose: List type (dynamic array of variants).                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_LIST_HPP
#define CALAO_LIST_HPP

#include <calao/array.hpp>
#include <calao/variant.hpp>

namespace calao {

class List final
{
public:

	using Storage = Array<Variant>;
	using iterator = Storage::iterator;
	using const_iterator = Storage::const_iterator;

	List() = default;

	explicit List(intptr_t size) : _items(size, Variant()) { }

	List(const List &other);

	List(List &&other) noexcept = default;

	List(Storage items) : _items(std::move(items)) { }

	intptr_t size() const { return _items.size(); }

	Variant *data() { return _items.data(); }

	const Variant *data() const { return _items.data(); }

	Variant &operator[](intptr_t i) { return _items[i]; }

	Variant &at(intptr_t i) { return _items.at(i); }

	iterator begin() { return _items.begin(); }
	const_iterator cbegin() { return _items.begin(); }

	iterator end() { return _items.end(); }
	const_iterator cend() { return _items.end(); }

	void traverse(const GCCallback &callback);

	Storage &items() { return _items; }
	const Storage &items() const { return _items; }

	String to_string() const;

private:

	Storage _items;
	mutable bool seen = false; // for printing
};


//---------------------------------------------------------------------------------------------------------------------

namespace meta {

static inline void traverse(List &lst, const GCCallback &callback)
{
	lst.traverse(callback);
}


static inline String to_string(const List &lst)
{
	return lst.to_string();
}
} // namespace calao::meta

} // namespace calao

#endif // CALAO_LIST_HPP
