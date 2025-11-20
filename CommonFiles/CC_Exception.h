#pragma once
#include "CC_Core.h"

namespace Cc
{
#ifdef PLAT_WIN32
	class CCAPI Exception;
#endif

	class Exception : public std::exception
	{
	public:
		Exception(std::source_location loc = std::source_location::current());
		const char* what() const noexcept override;

	protected:
		std::string m_File, m_Func;
		uint32_t m_Line;
		mutable std::string m_WhatBuffer;
	};
}