#pragma once
#include "CC_Application.h"
#include "CC_Exception.h"

extern Cc::Application* Cc::NewApplicationInterface(std::vector<const char*>& v_args);

#ifdef NDEBUG

#else

int main(int argc, char** argv) try
{
	std::vector<const char*> v_args(argc);

	for (int i = 0; i < argc; i++)
		v_args[i] = argv[i];

	auto app = Cc::NewApplicationInterface(v_args);

	app->Run();

	if (app) delete app;

	return 0;
}
catch (const Cc::Exception& ce)
{
	return 1;
}

#endif