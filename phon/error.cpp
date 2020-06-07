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
 * Purpose: see header.                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include <phon/error.hpp>
#include <phon/string.hpp>

namespace phonometrica {

std::runtime_error error(const String &msg)
{
	return error(msg.data());
}

RuntimeError::RuntimeError(intptr_t line, const String &s) :
	std::runtime_error(s.data()), line(line)
{

}
} // namespace phonometrica
