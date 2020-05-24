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
#include <calao/file.hpp>

using namespace calao;

int main()
{
	Runtime rt;
	Variant v("hello, world!");
	intptr_t i = 141;
	Variant n(i);
	Variant pi(3.14);
	Handle<List> lst = rt.create<List>(8);
	std::cout << "cap: " << lst->capacity() << std::endl;
	Handle<File> f = rt.create<File>("/home/julien/TODO_11");
	std::cout << f->read_line() << std::endl;
	
	try {
		std::cout << cast<String>(pi) << std::endl;
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
