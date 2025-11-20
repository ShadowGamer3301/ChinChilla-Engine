#include "CC_Application.h"

Cc::Application::Application()
{
	mp_Window = new Window(800, 600, "ChinChilla Engine", false);
	mp_Graphics = new Graphics(mp_Window);
}

Cc::Application::~Application()
{
	if (mp_Graphics) delete mp_Graphics;
	if (mp_Window) delete mp_Window;
}
