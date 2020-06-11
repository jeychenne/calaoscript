/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 05/06/2019                                                                                                *
 *                                                                                                                    *
 * Purpose: Set type (ordered set).                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef PHONOMETRICA_SET_HPP
#define PHONOMETRICA_SET_HPP

#include <set>
#include <phon/runtime/variant.hpp>

namespace phonometrica {

class Set final
{
public:

	using Storage = std::set<Variant>;
	using iterator = Storage::iterator;

	Set() = default;

	Set(Storage items) : _items(std::move(items)) { }

	Set(const Set &other);

	Set(Set &&other) = default;

	bool operator==(const Set &other) const;

	iterator begin() { return _items.begin(); }

	iterator end() { return _items.end(); }

	Storage &items() { return _items; }

	String to_string() const;

	void traverse(const GCCallback &callback);

	bool contains(const Variant &v) const { return _items.find(v) != _items.end(); }

	intptr_t size() const { return intptr_t(_items.size()); }

private:

	Storage _items;
	mutable bool seen = false;
};



//---------------------------------------------------------------------------------------------------------------------

namespace meta {

static inline void traverse(Set &set, const GCCallback &callback)
{
	set.traverse(callback);
}


static inline String to_string(const Set &set)
{
	return set.to_string();
}
} // namespace phonometrica::meta
} // namespace phonometrica

#endif // PHONOMETRICA_SET_HPP
