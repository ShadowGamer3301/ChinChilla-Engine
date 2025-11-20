#pragma once
#include "CC_Core.h"
#include "CC_Window.h"
#include "CC_Graphics.h"

namespace Cc
{
#ifdef PLAT_WIN32
	class CCAPI Application;
#endif

	class Application
	{
	public:
		Application();
		virtual ~Application();

		virtual void Run() = 0;

		inline Window* GetWindow() const noexcept { return mp_Window; }
		inline Graphics* GetGraphics() const noexcept { return mp_Graphics; }

	private:
		Window* mp_Window;
		Graphics* mp_Graphics;
	};

	Application* NewApplicationInterface(std::vector<const char*>& v_args);
}