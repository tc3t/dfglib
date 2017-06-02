#include "dfgBase.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Begin: Static map
// Description: Helps in static mapping of non-index values(could for example be enums) to index values 
//				that can be used to access data stored in an array.
// Rationale: Mapping enums to strings may sometimes be needed, but the mapping doesn't seem 
//				readily doable especially without runtime data structures.
//				Assuming an array with values(strings) assosiated with enums,
//				static map macros help in creating mapping from enums to array index so that
//				value corresponding to given enum can be accessed with 
//				valuesArray[someName<enum_value>::value].
// Note:		Static map can't be constructed inside a function. (2013)
// Example:
//				static const char* szNames = {"ten", "twelve", "fourty five"};
//				enum {Ten = 10, Twenty = 20, FourtyFive = 45};
//				DFG_BEGIN_STATIC_MAP(mapExample, szNames)
//					DFG_ADD_MAPPING(mapExample, Ten, 0);
//					DFG_ADD_MAPPING(mapExample, Twenty, 1);
//					DFG_ADD_MAPPING(mapExample, FourtyFive, 2);
//				DFG_END_STATIC_MAP(mapExample)
//				std::cout << szNames[mapExample<Ten>::index] << std::endl;
//				std::cout << szNames[mapExample<Twenty>::index] << std::endl;
//				std::cout << szNames[mapExample<FourtyFive>::index] << std::endl;
//
// Related code: Frozen-library (https://github.com/serge-sans-paille/frozen)
//

#define DFG_BEGIN_STATIC_MAP(className, array) \
	template <size_t key> struct className; \
	const size_t className##MaxIndex = DFG_COUNTOF(array);
#define DFG_END_STATIC_MAP(className)

#define DFG_ADD_MAPPING(className, key, val) \
	static_assert(val < className##MaxIndex, "Bad static map index"); \
	template <> struct className<key> {static const size_t index = val;};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// End: static map.