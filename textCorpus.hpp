/**************************************************************************
A class for containing and working on bodies of text.

Copyright (C) 2021 Chris Morrison (gnosticist@protonmail.com)

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

#ifndef _TEXT_EXTRACTOR_H_
#define _TEXT_EXTRACTOR_H_

#include <cstdio>
#include <stack>
#include <cstring>
#include <sstream>
#include <iostream>
#include <codecvt>
#include <string>
#include <algorithm>
#include <filesystem>

#include "stringUtils.hpp"
#include "textCorpusItem.hpp"

#ifndef MAX_PATH
#define MAX_PATH 512
#endif

namespace fsl::text
{
    class textCorpus
    {
    private:
        std::vector<textCorpusItem> _items;
        bool _splitSentences;
        bool _splitParagraphs;
        bool _removeHtmlTags;
    public:

        textCorpus()
        {
            _splitSentences = false;
            _splitParagraphs = false;
            _removeHtmlTags = true;
        }

        ~textCorpus() = default;

        [[nodiscard]] bool empty() const noexcept
        {
            return _items.empty();
        }

        [[nodiscard]] bool splitSentences() const
        {
            return _splitSentences;
        }

        void setSplitSentences(bool splitSentances)
        {
            _splitSentences = splitSentances;
        }

        [[nodiscard]] bool splitParagraphs() const
        {
            return _splitParagraphs;
        }

        void setSplitParagraphs(bool splitParagraphs)
        {
            _splitParagraphs = splitParagraphs;
        }

        [[nodiscard]] const std::vector<textCorpusItem>& parts() const
        {
            return _items;
        }

        void parseString(const std::string& input, bool append)
        {

        }

        void parseString(const std::wstring& input, bool append)
        {
            std::vector<std::wstring> items;
            std::vector<std::wstring> temp;
            if (append)
            {
                if (_splitParagraphs && !_items.empty() && !_items.back().empty()) _items.emplace_back();
            }
            else
            {
                _items.clear();
            }

            // The parsing of of a string will be carried out in X discrete phases.
            // ---------------------------------------------------------------------------------------------------------
            // Phase 1 - Obtain a copy of the string with all leading and trailing newlines and whitespaces removed.
            // ---------------------------------------------------------------------------------------------------------
            std::wstring trimmed = boost::algorithm::trim_copy_if(input, [](wchar_t wc){ if (wc == 0x000D) return true; return fsl::_private::_wspc_pred(wc); });

            // ---------------------------------------------------------------------------------------------------------
            // Phase 2 - Remove any Windows style line endings.
            // ---------------------------------------------------------------------------------------------------------
            boost::replace_all(trimmed, "\r\n", "\n");

            // ---------------------------------------------------------------------------------------------------------
            // Phase 3 - Call _prep_string() which will:-
            //
            // - Replace all unicode line breaks with '\n'
            // - Replace all unicode paragraph breaks with '\n\n'
            // - Replace all white space characters with ASCII 32.
            // - Ensure that there are no sequences of two or more consecutive white spaces (32) in the string.
            // - Ensure that there are no sequences of more that two consecutive line breaks (\n) in the string.
            // - Convert decorative unicode characters such as curly quotation marks to their ASCII equivalents.
            // ---------------------------------------------------------------------------------------------------------

            std::wstring copy;
            fsl::_private::_prep_string(trimmed, copy);

            // ---------------------------------------------------------------------------------------------------------
            // Phase 4 - If the caller has requested it, remove all the HTML/XML tabs. Replace paragraph ends
            // with '\n\n' and line breaks with '\n'
            // ---------------------------------------------------------------------------------------------------------
            if (_removeHtmlTags)
            {
                boost::ireplace_all(copy, L"</p>", L"\n\n");
                boost::ireplace_all(copy, L"<br>", "\n");
                boost::erase_all_regex(copy, boost::wregex(L"<[^<>]+>"));
            }

            // ---------------------------------------------------------------------------------------------------------
            // Phase 5 - Split the string into items.
            // ---------------------------------------------------------------------------------------------------------
            boost::wregex wrx;
            if (_splitSentences)
            {
                // Remove all single line breaks.
                wrx.assign(L"([^\\n])( *\\n *)([^\\n])");
                copy = boost::regex_replace(copy, wrx, [](const boost::wsmatch& m)->std::wstring
                {
                    return m[1].str() + L" " + m[3].str();
                });

                // Replace them with breaks at sentences.
                wrx.assign(L"!(\\s{1})");
                copy = boost::regex_replace(copy, wrx, L"!\n");
                wrx.assign(L"\\?(\\s{1})");
                copy = boost::regex_replace(copy, wrx, L"!\n");
                wrx.assign(L"!\"(\\s{1})");
                copy = boost::regex_replace(copy, wrx, L"!\"\n");
                wrx.assign(L"\\?\"(\\s{1})");
                copy = boost::regex_replace(copy, wrx, L"?\"\n");
                wrx.assign(L"\\.\"(\\s{1})");
                copy = boost::regex_replace(copy, wrx, L".\"\n");
                wrx.assign(L"!'(\\s{1})");
                copy = boost::regex_replace(copy, wrx, L"!'\n");
                wrx.assign(L"\\?'(\\s{1})");
                copy = boost::regex_replace(copy, wrx, L"?'\n");
                wrx.assign(L"\\.'(\\s{1})");
                copy = boost::regex_replace(copy, wrx, L".'\n");
                // The case of sentences that end with a period is considerably more complex and will require careful handling.
                wrx.assign(L"[A-Za-z]{2,}\\.+)([[:space:]]+)(?=[A-Z])"); // Letter at end only.
                copy = boost::regex_replace(copy, wrx, [](const boost::wsmatch& m)->std::wstring
                {
                    return m[1] + L"\n";
                });
            }
            // If we are not splitting paragraphs when remove all the paragraph breaks and replace them with ordinary line breaks.
            if (!_splitParagraphs)
            {
                wrx.assign(L"\\n{2,}");
                copy = boost::regex_replace(copy, wrx, L"\n");
            }

            // If we are splitting by paragraph
            if (_splitParagraphs)
            {
                wrx.assign(L"\\n{2,}");
                boost::split_regex(items, copy, wrx);
                // loop through each paragraph.
                for (const auto& s : items)
                {
                    if (_splitSentences)
                    {
                        std::vector<std::wstring> sentences;
                        boost::split(sentences, s, [](wchar_t wc) { if (wc == L'\n') return true; return false; });
                        for (const auto& st : sentences)
                        {
                            if (st.empty()) continue;
                            textCorpusItem itm(st, textCorpusItem::itemType::sentence);
                            _items.push_back(std::move(itm));
                        }
                    }
                    else
                    {
                        if (s.empty()) continue;
                        textCorpusItem itm(s, textCorpusItem::itemType::paragraph);
                        _items.push_back(std::move(itm));
                    }

                    // Add empty item to delimit the paragraphs.
                    _items.emplace_back();
                }
            }
            else
            {
                // In all other cases, just split each line by line breaks and add to the list.
                boost::split(temp, copy, [](wchar_t wc) { if (wc == L'\n') return true; return false; });
                for (const auto& st : temp)
                {
                    if (st.empty()) continue;
                    textCorpusItem itm(st, textCorpusItem::itemType::text);
                    _items.push_back(std::move(itm));
                }
            }


        }

        [[nodiscard]] bool removeHtmlTags() const
        {
            return _removeHtmlTags;
        }

        void setRemoveHtmlTags(bool removeHtmlTags)
        {
            _removeHtmlTags = removeHtmlTags;
        }
    };
}

#endif // _TEXT_EXTRACTOR_H_
