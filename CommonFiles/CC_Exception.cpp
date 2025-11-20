#include "CC_Exception.h"

namespace Cc
{
	Exception::Exception(std::source_location loc)
		: m_File(loc.file_name()), m_Func(loc.function_name()), m_Line(loc.line())
	{}

	const char* Exception::what() const noexcept
	{
		std::ostringstream oss;
		oss << "Exception caught!\n"
			<< "[LINE] " << m_Line << "\n"
			<< "[FUNC] " << m_Func << "\n"
			<< "[FILE] " << m_File << "\n";

		m_WhatBuffer = oss.str();

		return m_WhatBuffer.c_str();
	}
}