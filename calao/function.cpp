/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 22/05/2020                                                                                                *
 *                                                                                                                    *
 * Purpose: see header.                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include <calao/function.hpp>
#include <calao/runtime.hpp>

namespace calao {

Routine::Routine(std::vector<Handle<Class>> sig, ParamBitset ref_flags, int min_arg, int max_arg) :
	signature(std::move(sig)), ref_flags(ref_flags), min_argc(min_arg), max_argc(max_arg)
{

}

NativeRoutine::NativeRoutine(NativeCallback cb, std::vector<Handle<Class>> sig, ParamBitset ref_flags, int min_arg,
							 int max_arg) : Routine(std::move(sig), ref_flags, min_arg, max_arg), callback(std::move(cb))
{

}

void NativeRoutine::call(Runtime &, CallInfo &)
{

}

//----------------------------------------------------------------------------------------------------------------------



ScriptRoutine::ScriptRoutine() : Routine()
{

}

ScriptRoutine::ScriptRoutine(std::vector<Handle<Class>> sig, ParamBitset ref_flags, int min_arg, int max_arg) :
	Routine(std::move(sig), ref_flags, min_arg, max_arg)
{

}

void ScriptRoutine::call(Runtime &rt, CallInfo &)
{
	rt.interpret(code);
}
} // namespace calao