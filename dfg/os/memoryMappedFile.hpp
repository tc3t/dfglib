#pragma once

#include "../dfgDefs.hpp"

#ifndef BOOST_IOSTREAMS_NO_LIB
	#define BOOST_IOSTREAMS_NO_LIB
	#include <boost/iostreams/device/mapped_file.hpp>
	#undef BOOST_IOSTREAMS_NO_LIB
#else
	#include <boost/iostreams/device/mapped_file.hpp>
#endif


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(os) {

	class DFG_CLASS_NAME(MemoryMappedFile) : public boost::iostreams::mapped_file_source
	{

	};

} }
