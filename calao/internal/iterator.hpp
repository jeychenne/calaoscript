/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 02/06/2020                                                                                                *
 *                                                                                                                    *
 * Purpose: Iterator object, to iterate over sequences such as List and Table.                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_ITERATOR_HPP
#define CALAO_ITERATOR_HPP

#include <calao/list.hpp>
#include <calao/table.hpp>

namespace calao {

// We can't make this an abstract interface because we need to store iterators in handles, and Handle<T> would complain that it can't
// instantiate an abstract class.
class Iterator
{
public:

	Iterator(Variant v, bool with_val, bool ref_val) : object(std::move(v)), with_val(with_val), ref_val(ref_val) { }

	virtual ~Iterator() = default;

	virtual Variant get_key() { return Variant(); }

	virtual Variant get_value();

	virtual bool at_end() const { return true; };

protected:

	Variant object;
	bool with_val;
	bool ref_val;
};

//---------------------------------------------------------------------------------------------------------------------

class ListIterator : public Iterator
{
public:

	ListIterator(Variant v, bool with_val, bool ref_val);

	Variant get_key() override;

	Variant get_value() override;

	bool at_end() const override;

private:

	List::Storage *lst;
	intptr_t pos;
};

//---------------------------------------------------------------------------------------------------------------------

class TableIterator : public Iterator
{
public:

	TableIterator(Variant v, bool with_val, bool ref_val);

	Variant get_key() override;

	Variant get_value() override;

	bool at_end() const override;

private:

	Table::Storage *map;
	Table::iterator it;
};

//---------------------------------------------------------------------------------------------------------------------

class StringIterator : public Iterator
{
public:

	StringIterator(Variant v, bool with_val, bool ref_val);

	Variant get_key() override;

	Variant get_value() override;

	bool at_end() const override;

private:

	String *str;
	intptr_t pos = 1;
};


//---------------------------------------------------------------------------------------------------------------------

class RegexIterator : public Iterator
{
public:

	RegexIterator(Variant v, bool with_val, bool ref_val);

	Variant get_key() override;

	Variant get_value() override;

	bool at_end() const override;

private:

	Regex *re;
	intptr_t pos = 1;
};


//---------------------------------------------------------------------------------------------------------------------

class FileIterator : public Iterator
{
public:

	FileIterator(Variant v, bool with_val, bool ref_val);

	Variant get_key() override;

	Variant get_value() override;

	bool at_end() const override;

private:

	File *file;
	intptr_t pos = 1;
};

} // namespace calao

#endif // CALAO_ITERATOR_HPP
