#ifndef __XBASE_JSON_UTILS_H__
#define __XBASE_JSON_UTILS_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "xbase/x_debug.h"

namespace xcore
{
    namespace json
    {
		// UTF-8; read a character and return the unicode codepoint (UTF-32)
		struct uchar8_t
		{
			u32 c;
			s32 l;
		};

		char const* ParseNumber(char const* str, char const* end, f64* out_number);
		bool WriteChar(u32 ch, char*& cursor, char const* end);
		s32 SizeOfChar(u32 ch);
		uchar8_t PeekChar(const char* str, const char* end);

    } // namespace json
} // namespace xcore

#endif // __XBASE_JSON_H__