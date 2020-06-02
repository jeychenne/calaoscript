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

class Iterator
{
public:

	explicit Iterator(Variant v) : object(std::move(v)) { }

	virtual ~Iterator() = default;

	virtual Variant get_key() = 0;

	virtual Variant get_value() { return Variant(); };

	virtual bool at_end() const = 0;

protected:

	Variant object;
};

//---------------------------------------------------------------------------------------------------------------------

class ListIterator : public Iterator
{
public:

	explicit ListIterator(Variant v);

	Variant get_key() override;

	bool at_end() const override;

private:

	List::iterator it;
};


} // namespace calao

#endif // CALAO_ITERATOR_HPP
