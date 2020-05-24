/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 20/02/2019                                                                                                *
 *                                                                                                                    *
 * Purpose: Table type (dynamic hash table).                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_TABLE_HPP
#define CALAO_TABLE_HPP

#include <calao/hashmap.hpp>
#include <calao/variant.hpp>

namespace calao {

using Table = Hashmap<Variant, Variant>;

} // namespace calao

#endif // CALAO_TABLE_HPP
