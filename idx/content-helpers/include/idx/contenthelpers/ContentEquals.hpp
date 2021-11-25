#ifndef __IDX__CONTENTHELPERS__CONTENT_EQUALS__
#define __IDX__CONTENTHELPERS__CONTENT_EQUALS__

/** @author robert.binna@uibk.ac.at */

#include <cstring>

namespace idx { namespace contenthelpers {

/**
 * checks value equality for two given values
 * template specializations allow to adapt its behaviour for specific types.
 * For instance a specialization exist which compares two c-strings by using strcmp.
 */
template<typename Value> __attribute__((always_inline)) inline bool contentEquals(Value value1, Value value2) {
	return value1 == value2;
}

/*
 * value1: the rule
 * value2: the key
 * mask: the mask field for value1
 */
template<typename Value> __attribute__((always_inline)) inline bool contentEquals(Value value1, Value value2, uint64_t mask) {
  return ( (value1 | mask) == (value2 | mask) );
}

template<> __attribute__((always_inline)) inline bool contentEquals<char const*>(char const* value1, char const* value2) {
	return strcmp(value1, value2) == 0;
}

} }

#endif
