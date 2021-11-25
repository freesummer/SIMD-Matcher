#ifndef __HOT__COMMONS__ENTRY_VECTOR_INTERFACE___
#define __HOT__COMMONS__ENTRY_VECTOR_INTERFACE___

namespace hot { namespace commons {


 struct entryVector
 {
   intptr_t value;
   uint64_t mask;
   uint64_t pp;

   inline entryVector() {
     value = 0;
     mask = 0;
     pp = 0;
   }

   inline entryVector(entryVector const & other) = default;

   inline entryVector & operator=(entryVector const & other) = default;


 };







}}

#endif
