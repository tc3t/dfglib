#pragma once

#include "../dfgDefs.hpp"

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
#ifndef BOOST_IOSTREAMS_NO_LIB
	#define BOOST_IOSTREAMS_NO_LIB
	#include <boost/iostreams/device/mapped_file.hpp>
	#undef BOOST_IOSTREAMS_NO_LIB
#else
	#include <boost/iostreams/device/mapped_file.hpp>
#endif
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(os) {

	class DFG_CLASS_NAME(MemoryMappedFile) : public boost::iostreams::mapped_file_source
	{

	};

} }
