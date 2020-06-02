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
 * Purpose: see header.                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include <calao/internal/iterator.hpp>

namespace calao {

ListIterator::ListIterator(Variant v) : Iterator(std::move(v))
{
	it = raw_cast<List>(object).items().begin();
}

Variant ListIterator::get_key()
{
	return *it++;
}

bool ListIterator::at_end() const
{
	return it == raw_cast<List>(object).items().end();
}

} // namespace calao