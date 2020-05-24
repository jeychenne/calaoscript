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

using List = Array<Variant>;

} // namespace calao

#endif // CALAO_LIST_HPP
