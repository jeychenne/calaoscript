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

#ifndef CALAO_SET_HPP
#define CALAO_SET_HPP

#include <set>
#include <calao/variant.hpp>

namespace calao {

class Set final
{
public:

	using Storage = std::set<Variant>;
	using iterator = Storage::iterator;

	Set() = default;

	Set(Storage items) : _items(std::move(items)) { }

	Set(const Set &other);

	Set(Set &&other) = default;

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
} // namespace calao::meta
} // namespace calao

#endif // CALAO_SET_HPP
