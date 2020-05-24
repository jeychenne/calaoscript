/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 29/11/2019                                                                                                *
 *                                                                                                                    *
 * Purpose: hash table specialized for String keys.                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_DICTIONARY_HPP
#define CALAO_DICTIONARY_HPP

#include "hashmap.hpp"
#include "string.hpp"

namespace calao {

template <typename Val>
using Dictionary = Hashmap<String, Val>;

}

#endif // CALAO_DICTIONARY_HPP
