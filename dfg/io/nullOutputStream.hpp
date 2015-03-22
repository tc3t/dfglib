#pragma once

#include "../dfgDefs.hpp"

// Stream that writes data nowhere.
// NOTE: Very quick hack.
// TODO: make better implementation and test.
//		 Should inherit from std::ostream to enable passing to functions expecting std::ostream&.
// Other implementations: Poco::NullOutputStream
class DFG_CLASS_NAME(NullOutputStream)
{
public:
	DFG_CLASS_NAME(NullOutputStream)()
	{}

	template <class T>
	DFG_CLASS_NAME(NullOutputStream)& operator<<(const T&) { return *this; }
};
