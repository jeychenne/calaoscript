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
 * Purpose: standalone interpreter.                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include <iostream>
#include <calao/runtime.hpp>
#include <calao/internal/code.hpp>

using namespace calao;

int main()
{
	Runtime rt;

	try
	{
		rt.do_file("/home/julien/Temp/hello.calao");
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}

//	Code code;
//	auto i = code.add_float_constant(3.14);
//	code.emit(1, Opcode::PushFloat, i);
//	code.add_string_constant("hello");
//	i = code.add_string_constant("world");
//	code.emit(124, Opcode::PushString, i);
//	code.emit(124, Opcode::PushBoolean, 1);
//	code.emit(125, Opcode::PushSmallInt, 413);
//	code.emit(125, Opcode::Negate);
//	i = code.add_integer_constant(8);
//	code.emit(125, Opcode::PushInteger, i);
//	code.emit(125, Opcode::Add);
//	code.emit(126, Opcode::PushSmallInt, 0);
//	code.emit(126, Opcode::Divide);
//	code.emit(130, Opcode::Return);
//
//	try
//	{
//		rt.disassemble(code, "test");
//		rt.interpret(code);
//		auto &v = rt.peek();
//		std::cout << cast<intptr_t>(v) << std::endl;
//	}
//	catch (std::exception &e) {
//		std::cerr << e.what() << std::endl;
//	}

//	Variant v("hello, world!");
//	intptr_t i = 141;
//	Variant n(i);
//	Variant pi(3.14);
//	Handle<List> lst = rt.create<List>(8);
//	std::cout << "cap: " << lst->capacity() << std::endl;
//	Handle<File> f = rt.create<File>("/home/julien/TODO_11");
//	std::cout << f->read_line() << std::endl;
//
//	try {
//		std::cout << cast<String>(pi) << std::endl;
//	}
//	catch (std::exception &e) {
//		std::cerr << e.what() << std::endl;
//	}

	return 0;
}
