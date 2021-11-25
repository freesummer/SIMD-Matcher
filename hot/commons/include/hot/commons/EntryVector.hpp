#ifndef __HOT__COMMONS__ENTRY_VECTOR___
#define __HOT__COMMONS__ENTRY_VECTOR___

#include <hot/commons/include/hot/commons/DiscriminativeBit.hpp>

#include <hot/commons/include/hot/commons/EntryVectorInterface.hpp>


namespace hot { namespace commons {



inline entryVector(intptr_t const value_t, uint64_t const mask_t, uint64_t const priority) : value(value_t), mask(mask_t), pp(priority) {
}

 inline entryVector(entryVector const & other) : value(other.value), mask(other.mask), pp(other.pp){

 }



}}

#endif
