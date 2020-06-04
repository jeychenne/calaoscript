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

int main(int argc, char *argv[])
{
	try
	{
		Runtime rt;

		if (argc > 2)
		{
			String option(argv[1]), path(argv[2]);

			if (option == "-l") // list
			{
				auto closure = rt.compile_file(path);
				rt.disassemble(*closure, "main");
			}
			else if (option == "-r") // run
			{
				rt.do_file(path);
			}
			else if (option == "-a") // all
			{
				auto closure = rt.compile_file(path);
				rt.disassemble(*closure, "main");
				puts("-------------------------------------------------------------------\n");
				rt.interpret(*closure);
//				auto &map = cast<Table>(v).map();
//				std::cout << "name: " << map["name"] << std::endl;
//				std::cout << "age: " << map["age"] << std::endl;
			}
			else
			{
				throw error("Unrecognized option '%'\n", option);
			}

		}
		else if (argc > 1)
		{
			String path(argv[1]);
			rt.do_file(path);
		}
		else
		{
			std::cout << "Usage: program [option] file" << std::endl;
			std::cout << "Options: " << std::endl;
			std::cout << " -l\t(list)\tlist bytecode (disassemble) file" << std::endl;
			std::cout << " -r\t(run)\texecute file" << std::endl;
			std::cout << " -a\t(all)\tdisassemble and execute file" << std::endl;
		}

	}
	catch (RuntimeError &e) {
		std::cerr << "Line " << e.line_no() << ": " << e.what() << std::endl;
		return 1;
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
