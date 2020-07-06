/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-07-06
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Console
{
    using Logline_t = std::pair<std::string, COLORREF>;

    // Fetch a copy of the internal strings.
    std::vector<Logline_t> getLoglines(size_t Count, std::string_view Filter);

    // Threadsafe injection of strings into the global log.
    void addConsolemessage(const std::string &Message, COLORREF Colour);

    // Which user-selected filter is active?
    std::string_view getCurrentfilter();




}
