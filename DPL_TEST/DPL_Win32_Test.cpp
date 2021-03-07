#include "stdafx.h"
#include "dplvm.h"
#include <windows.h>

using namespace DPL;

DPLVM dplvm;

int _tmain(int argc, _TCHAR* argv[]) {
	std::string file("");
	if (argc == 1) {
		std::cout << "MicroDPL Virtual Machine Win32 Shell" << std::endl;
		std::cout << "Copyright 2021 Alan Kubiak" << std::endl;
		std::cout << "File>";
		std::cin >> file;
	}
	else if (argc == 2) {
		file = argv[1];
	}
	else {
		std::cout << "Invalid arguments." << std::endl;
	}
	if (file != "") {
		try {
			dplvm.Initialize();
			DWORD start = ::GetTickCount();
			dplvm.Load(file);
			dplvm.Run();
			DWORD end = ::GetTickCount();
			DWORD time = end - start;
			std::cout << std::endl << "Execution spent time: " << time << std::endl;
			system("pause");
		}
		catch (std::exception e) {
			std::cout << e.what() << std::endl;
		}
	}
	return 0;
}
