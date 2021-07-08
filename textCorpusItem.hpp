/**************************************************************************
A class to contain a part or sentence in a textCorpus object.

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

#ifndef _TEXT_CORPUS_ITEM_HPP_
#define _TEXT_CORPUS_ITEM_HPP_

#include <ostream>
#include "stringUtils.hpp"

namespace fsl::text
{
    class textCorpusItem
    {
    public:
        enum class itemType
        {
            title,
            text,
            sentence,
            paragraph,
            listItem,
        };

    private:
        std::wstring _payload;
        itemType _type;
    public:
        textCorpusItem()
        {
            _type = itemType::paragraph;
        }

        textCorpusItem(const textCorpusItem &other)
        {
            _payload = other._payload;
            _type = other._type;;
        }

        textCorpusItem(textCorpusItem &&other) noexcept
        {
            _payload = std::move(other._payload);
            _type = other._type;
        }

        textCorpusItem(const std::wstring &str, itemType type)
        {
            _type = type;
            auto temp = boost::algorithm::trim_copy_if(str, [](wchar_t wc){ if (wc == 0x000D) return true; return fsl::_private::_wspc_pred(wc); });
            fsl::_private::_prep_string(temp, _payload);
        }

        textCorpusItem(const std::string &str, itemType type)
        {
            _type = type;
            auto wcs = fsl::_private::_fromUTF8(str.c_str());
            std::wstring temp(wcs);
            std::free(wcs);
            auto temp2 = boost::algorithm::trim_copy_if(temp, [](wchar_t wc){ if (wc == 0x000D) return true; return fsl::_private::_wspc_pred(wc); });
            fsl::_private::_prep_string(temp2, _payload);
        }

        [[nodiscard]] std::string stringData() const
        {
            auto cs = fsl::_private::_toUTF8(_payload.c_str());
            std::string outs(cs);

            return outs;
        }

        [[nodiscard]] std::wstring wideStringData() const
        {
            return _payload;
        }

        [[nodiscard]] itemType type() const
        {
            return _type;
        }

        [[nodiscard]] bool empty() const
        {
            return _payload.empty();
        }

        friend std::wostream &operator<<(std::wostream &os, const textCorpusItem &item)
        {
            os << item._payload;
            return os;
        }

        friend std::ostream &operator<<(std::ostream &os, const textCorpusItem &item)
        {

            return os;
        }
    };
}

#endif // _TEXT_CORPUS_ITEM_HPP_
