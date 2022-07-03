#pragma once

#include "../dfgDefs.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

namespace DFG_DETAIL_NS
{
    // Stream that writes data nowhere.
    // NOTE: Very quick hack, see 'Other implementations" for better alternatives
    // TODO: make better implementation and test.
    //      Should inherit from std::ostream to enable passing to functions expecting std::ostream&.
    // Other implementations: boost::basic_null_sink, Poco::NullOutputStream
    template <class Char_T>
    class NullSink
    {
    public:
        NullSink() = default;

        template <class T> NullSink(const T&) {}

        template <class T>
        NullSink& operator<<(const T&) { return *this; }

        std::streamsize write(const Char_T*, std::streamsize n) { return n; }
    };
} // namespace DFG_DETAIL_NS


using NullOutputStream = DFG_DETAIL_NS::NullSink<char>;

} } // namespace dfg::io
