#include "CC_Window.h"

namespace Cc
{
	WindowException::WindowException(int code, std::source_location loc)
		: m_Code(code), Exception(loc)
	{}

	const char* WindowException::what() const noexcept
	{
		std::ostringstream oss;
		oss << "Exception caught!\n"
			<< "[LINE] " << m_Line << "\n"
			<< "[FUNC] " << m_Func << "\n"
			<< "[FILE] " << m_File << "\n"
			<< "[CODE] " << m_Code << "\n";

		m_WhatBuffer = oss.str();

		return m_WhatBuffer.c_str();
	}


	Window::Window(uint32_t width, uint32_t height, const char* title, bool fullscreen)
		: m_Fullscreen(fullscreen)
	{
		LOG_F(INFO, "Creating window...");

		if (!glfwInit())
		{
			int ec = glfwGetError(nullptr);
			throw WindowException(ec);
		}

		LOG_F(INFO, "Setting GLFW_RESIZABLE and GLFW_CLIENT_API flags to 0");
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		if (fullscreen)
			mp_Window = glfwCreateWindow(width, height, title, glfwGetPrimaryMonitor(), nullptr);
		else
			mp_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);

		if (!mp_Window)
		{
			int ec = glfwGetError(nullptr);
			throw WindowException(ec);
		}

		LOG_F(INFO, "Window created");
	}

	Window::~Window()
	{
		glfwDestroyWindow(mp_Window);
	}

	bool Window::UpdateWindow()
	{
		glfwPollEvents();

		return !glfwWindowShouldClose(mp_Window);
	}

	int Window::GetWidth() const noexcept
	{
		int width = 0;
		glfwGetWindowSize(mp_Window, &width, nullptr);
		return width;
	}

	int Window::GetHeight() const noexcept
	{
		int height = 0;
		glfwGetWindowSize(mp_Window, nullptr, &height);
		return height;
	}

#ifdef PLAT_WIN32
	HWND Window::GetWindowHandle() const noexcept
	{
		return glfwGetWin32Window(mp_Window);
	}
#endif
}