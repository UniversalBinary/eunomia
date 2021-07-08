/**************************************************************************
Microsoft Windows platform specific code.

Copyright (C) 2020 Chris Morrison (gnosticist@protonmail.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/
#ifndef _STRING_UTILS_
#define _STRING_UTILS_

#ifdef _MSC_VER
#include <windows.h>
#endif

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.h>

namespace fsl::_private
{
#ifdef _MSC_VER

	inline wchar_t* _fromUTF8(const char* src, size_t src_length = 0, size_t* out_length = nullptr)
	{
		if (!src) return nullptr;

		if (src_length == 0) src_length = strlen(src);
		int length = MultiByteToWideChar(CP_UTF8, 0, src, src_length, 0, 0);
		wchar_t* output_buffer = (wchar_t*)std::malloc((length + 1) * sizeof(wchar_t));
		if (output_buffer)
		{
			MultiByteToWideChar(CP_UTF8, 0, src, src_length, output_buffer, length);
			output_buffer[length] = L'\0';
		}
		if (out_length) *out_length = length;

		return output_buffer;
	}

	inline char* _toUTF8(const wchar_t* src, size_t src_length = 0, size_t* out_length = nullptr)
	{
		if (!src) return nullptr;

		if (src_length == 0) src_length = wcslen(src);
		int length = WideCharToMultiByte(CP_UTF8, 0, src, src_length, 0, 0, NULL, NULL);
		char* output_buffer = (char*)std::malloc((length + 1) * sizeof(char));
		if (output_buffer)
		{
			WideCharToMultiByte(CP_UTF8, 0, src, src_length, output_buffer, length, NULL, NULL);
			output_buffer[length] = '\0';
		}
		if (out_length) *out_length = length;

		return output_buffer;
	}

#else

	inline wchar_t* _fromUTF8(const char* src, size_t src_length = 0, size_t* out_length = nullptr)
	{
        return nullptr;
	}

	inline char* _toUTF8(const wchar_t* src, size_t src_length = 0, size_t* out_length = nullptr)
	{
        return nullptr;
	}

#endif

    inline std::wstring _utf8_to_wstring(const std::string& str)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
        return myconv.from_bytes(str);
    }

    // Convert wstring to UTF-8 string
    inline std::string _wstring_to_utf8(const std::wstring& str)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
        return myconv.to_bytes(str);
    }

    inline bool _wspc_pred(wchar_t c)
    {
        if (c == 0x0009) return true;
        if (c == 0x000A) return true;
        if (c == 0x000B) return true;
        if (c == 0x000C) return true;
        if (c == 0x0020) return true;
        if (c == 0x00A0) return true;
        if (c == 0x1680) return true;
        if (c == 0x2000) return true;
        if (c == 0x2001) return true;
        if (c == 0x2002) return true;
        if (c == 0x2003) return true;
        if (c == 0x2004) return true;
        if (c == 0x2005) return true;
        if (c == 0x2006) return true;
        if (c == 0x2007) return true;
        if (c == 0x2008) return true;
        if (c == 0x2009) return true;
        if (c == 0x200A) return true;
        if (c == 0x202F) return true;
        if (c == 0x205F) return true;
        if (c == 0x3000) return true;

        return false;
    }

    inline bool _spc_pred(char c)
    {
        return _wspc_pred(static_cast<wchar_t>(c));
    }

    inline std::wstring& _prep_string(const std::wstring& in, std::wstring& out)
    {
        bool space_seen = false;
        for (const auto& c : in)
        {
            if (c == 0x2029)
            {
                out.push_back('\n');
                out.push_back('\n');
                continue;
            }
            if ((c == 0x2028) || (c == 0x0A) || (c == 0x0D))
            {
                out.push_back('\n');
                continue;
            }
            if (_wspc_pred(c))
            {
                if (space_seen) continue;
                out.push_back(' ');
                space_seen = true;
                continue;
            }
            else
            {
                space_seen = false;
            }
            if (c == 0x0085) { out.append(L"\n"); continue; } // Next line.
            if (c == 0x2028) { out.append(L"\n"); continue; } // Line separator.
            if (c == 0x2029) { out.append(L"\n\n"); continue; } // Paragraph separator.
            if (c == 0x00AB) { out.append(L"\""); continue; } // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
            if (c == 0x00AD) { out.append(L"-"); continue; }  // SOFT HYPHEN
            if (c == 0x00B4) { out.append(L"'"); continue; }  // ACUTE ACCENT
            if (c == 0x00BB) { out.append(L"\""); continue; } // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
            if (c == 0x00F7) { out.append(L"/"); continue; }  // DIVISION SIGN
            if (c == 0x01C0) { out.append(L"|"); continue; }  // LATIN LETTER DENTAL CLICK
            if (c == 0x01C3) { out.append(L"!"); continue; }  // LATIN LETTER RETROFLEX CLICK
            if (c == 0x02B9) { out.append(L"'"); continue; }  // MODIFIER LETTER PRIME
            if (c == 0x02BA) { out.append(L"\""); continue; } // MODIFIER LETTER DOUBLE PRIME
            if (c == 0x02BC) { out.append(L"'"); continue; }  // MODIFIER LETTER APOSTROPHE
            if (c == 0x02C4) { out.append(L"^"); continue; }  // MODIFIER LETTER UP ARROWHEAD
            if (c == 0x02C6) { out.append(L"^"); continue; }  // MODIFIER LETTER CIRCUMFLEX ACCENT
            if (c == 0x02C8) { out.append(L"'"); continue; }  // MODIFIER LETTER VERTICAL LINE
            if (c == 0x02CB) { out.append(L"`"); continue; }  // MODIFIER LETTER GRAVE ACCENT
            if (c == 0x02CD) { out.append(L"_"); continue; }  // MODIFIER LETTER LOW MACRON
            if (c == 0x02DC) { out.append(L"~"); continue; }  // SMALL TILDE
            if (c == 0x0300) { out.append(L"`"); continue; }  // COMBINING GRAVE ACCENT
            if (c == 0x0301) { out.append(L"'"); continue; }  // COMBINING ACUTE ACCENT
            if (c == 0x0302) { out.append(L"^"); continue; }  // COMBINING CIRCUMFLEX ACCENT
            if (c == 0x0303) { out.append(L"~"); continue; }  // COMBINING TILDE
            if (c == 0x030B) { out.append(L"\""); continue; } // COMBINING DOUBLE ACUTE ACCENT
            if (c == 0x030E) { out.append(L"\""); continue; } // COMBINING DOUBLE VERTICAL LINE ABOVE
            if (c == 0x0331) { out.append(L"_"); continue; }  // COMBINING MACRON BELOW
            if (c == 0x0332) { out.append(L"_"); continue; }  // COMBINING LOW LINE
            if (c == 0x0338) { out.append(L"/"); continue; }  // COMBINING LONG SOLIDUS OVERLAY
            if (c == 0x0589) { out.append(L":"); continue; }  // ARMENIAN FULL STOP
            if (c == 0x05C0) { out.append(L"|"); continue; }  // HEBREW PUNCTUATION PASEQ
            if (c == 0x05C3) { out.append(L":"); continue; }  // HEBREW PUNCTUATION SOF PASUQ
            if (c == 0x066A) { out.append(L"%"); continue; }  // ARABIC PERCENT SIGN
            if (c == 0x066D) { out.append(L"*"); continue; }  // ARABIC FIVE POINTED STAR
            if (c == 0x2010) { out.append(L"-"); continue; }  // HYPHEN
            if (c == 0x2011) { out.append(L"-"); continue; }  // NON-BREAKING HYPHEN
            if (c == 0x2012) { out.append(L"-"); continue; }  // FIGURE DASH
            if (c == 0x2013) { out.append(L"-"); continue; }  // EN DASH
            if (c == 0x2014) { out.append(L"-"); continue; }  // EM DASH
            if (c == 0x2015) { out.append(L"--"); continue; } // HORIZONTAL BAR
            if (c == 0x2016) { out.append(L"||"); continue; } // DOUBLE VERTICAL LINE
            if (c == 0x2017) { out.append(L"_"); continue; }  // DOUBLE LOW LINE
            if (c == 0x2018) { out.append(L"'"); continue; }  // LEFT SINGLE QUOTATION MARK
            if (c == 0x2019) { out.append(L"'"); continue; }  // RIGHT SINGLE QUOTATION MARK
            if (c == 0x201A) { out.append(L","); continue; }  // SINGLE LOW-9 QUOTATION MARK
            if (c == 0x201B) { out.append(L"'"); continue; }  // SINGLE HIGH-REVERSED-9 QUOTATION MARK
            if (c == 0x201C) { out.append(L"\""); continue; } // LEFT DOUBLE QUOTATION MARK
            if (c == 0x201D) { out.append(L"\""); continue; } // RIGHT DOUBLE QUOTATION MARK
            if (c == 0x201E) { out.append(L"\""); continue; } // DOUBLE LOW-9 QUOTATION MARK
            if (c == 0x201F) { out.append(L"\""); continue; } // DOUBLE HIGH-REVERSED-9 QUOTATION MARK
            if (c == 0x2032) { out.append(L"'"); continue; }  // PRIME
            if (c == 0x2033) { out.append(L"\""); continue; } // DOUBLE PRIME
            if (c == 0x2034) { out.append(L"'"); continue; }  // TRIPLE PRIME
            if (c == 0x2035) { out.append(L"`"); continue; }  // REVERSED PRIME
            if (c == 0x2036) { out.append(L"\""); continue; } // REVERSED DOUBLE PRIME
            if (c == 0x2037) { out.append(L"'"); continue; }  // REVERSED TRIPLE PRIME
            if (c == 0x2038) { out.append(L"^"); continue; }  // CARET
            if (c == 0x2039) { out.append(L"<"); continue; }  // SINGLE LEFT-POINTING ANGLE QUOTATION MARK
            if (c == 0x203A) { out.append(L">"); continue; }  // SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
            if (c == 0x203D) { out.append(L"?"); continue; }  // INTERROBANG
            if (c == 0x2044) { out.append(L"/"); continue; }  // FRACTION SLASH
            if (c == 0x204E) { out.append(L"*"); continue; }  // LOW ASTERISK
            if (c == 0x2052) { out.append(L"%"); continue; }  // COMMERCIAL MINUS SIGN
            if (c == 0x2053) { out.append(L"~"); continue; }  // SWUNG DASH
            if (c == 0x20E5) { out.append(L"\\"); continue; }  // COMBINING REVERSE SOLIDUS OVERLAY
            if (c == 0x2212) { out.append(L"-"); continue; }  // MINUS SIGN
            if (c == 0x2215) { out.append(L"/"); continue; }  // DIVISION SLASH
            if (c == 0x2216) { out.append(L"\\"); continue; }  // SET MINUS
            if (c == 0x2217) { out.append(L"*"); continue; }  // ASTERISK OPERATOR
            if (c == 0x2223) { out.append(L"|"); continue; }  // DIVIDES
            if (c == 0x2236) { out.append(L":"); continue; }  // RATIO
            if (c == 0x223C) { out.append(L"~"); continue; }  // TILDE OPERATOR
            if (c == 0x2264) { out.append(L"<="); continue; } // LESS-THAN OR EQUAL TO
            if (c == 0x2265) { out.append(L">="); continue; } // GREATER-THAN OR EQUAL TO
            if (c == 0x2266) { out.append(L"<="); continue; } // LESS-THAN OVER EQUAL TO
            if (c == 0x2267) { out.append(L">="); continue; } // GREATER-THAN OVER EQUAL TO
            if (c == 0x2303) { out.append(L"^"); continue; }  // UP ARROWHEAD
            if (c == 0x2329) { out.append(L"<"); continue; }  // LEFT-POINTING ANGLE BRACKET
            if (c == 0x232A) { out.append(L">"); continue; }  // RIGHT-POINTING ANGLE BRACKET
            if (c == 0x266F) { out.append(L"#"); continue; }  // MUSIC SHARP SIGN
            if (c == 0x2731) { out.append(L"*"); continue; }  // HEAVY ASTERISK
            if (c == 0x2758) { out.append(L"|"); continue; }  // LIGHT VERTICAL BAR
            if (c == 0x2762) { out.append(L"!"); continue; }  // HEAVY EXCLAMATION MARK ORNAMENT
            if (c == 0x27E6) { out.append(L"["); continue; }  // MATHEMATICAL LEFT WHITE SQUARE BRACKET
            if (c == 0x27E8) { out.append(L"<"); continue; }  // MATHEMATICAL LEFT ANGLE BRACKET
            if (c == 0x27E9) { out.append(L">"); continue; }  // MATHEMATICAL RIGHT ANGLE BRACKET
            if (c == 0x2983) { out.append(L"{"); continue; }  // LEFT WHITE CURLY BRACKET
            if (c == 0x2984) { out.append(L"}"); continue; }  // RIGHT WHITE CURLY BRACKET
            if (c == 0x3003) { out.append(L"\""); continue; } // DITTO MARK
            if (c == 0x3008) { out.append(L"<"); continue; }  // LEFT ANGLE BRACKET
            if (c == 0x3009) { out.append(L">"); continue; }  // RIGHT ANGLE BRACKET
            if (c == 0x301B) { out.append(L"]"); continue; }  // RIGHT WHITE SQUARE BRACKET
            if (c == 0x301C) { out.append(L"~"); continue; }  // WAVE DASH
            if (c == 0x301D) { out.append(L"\""); continue; } // REVERSED DOUBLE PRIME QUOTATION MARK
            if (c == 0x301E) { out.append(L"\""); continue; } // DOUBLE PRIME QUOTATION MARK

            out.push_back(c);
        }

        // Replace all instances of more than two newlines with two.
        boost::replace_all_regex(out, boost::wregex(L"\\n{2,}"), std::wstring(L"\n\n"));

        return out;
    }
}

#endif // _STRING_UTILS_
