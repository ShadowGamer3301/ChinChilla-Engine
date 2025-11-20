#pragma once
#include "CC_Core.h"
#include "CC_Exception.h"

namespace Cc
{
#ifdef PLAT_WIN32
	class CCAPI WindowException;
	class CCAPI Window;
#endif

	class WindowException : public Exception
	{
	public:
		WindowException(int code, std::source_location loc = std::source_location::current());
		const char* what() const noexcept override;

	private:
		int m_Code;
	};

	class Window
	{
	public:
		Window(uint32_t width, uint32_t height, const char* title, bool fullscreen);
		~Window();

		bool UpdateWindow();

		int GetWidth() const noexcept;
		int GetHeight() const noexcept;

		inline bool IsFullscreen() const noexcept { return m_Fullscreen; }

#ifdef PLAT_WIN32
		HWND GetWindowHandle() const noexcept;
#endif

	private:
		GLFWwindow* mp_Window = nullptr;
		bool m_Fullscreen = false;
	};
}