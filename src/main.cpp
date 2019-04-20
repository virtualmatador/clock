
#include "wall_clock.h"

#include <iostream>

int main(int argc, char* argv[])
{
	try
	{
		wall_clock w_c;
		w_c.run();
	}
	catch(const char* error)
	{
		std::cout << "Exception: " << error << std::endl;
		return -1;
	}
	return 0;
}
