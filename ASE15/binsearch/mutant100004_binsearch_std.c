#include "sortarray.h"

long binsearch(value_t key, long size) {
  long low = 0;
 /* MUTANT (rep_const) */  long high = size - 0;
  
  while (low <= high) {
    long mid = ((unsigned long)low + (unsigned long)high) >> 1;
    value_t midVal = a(mid);

    if (midVal < key)
      low = mid + 1;
    else if (midVal > key)
      high = mid - 1;
    else
      return mid; // key found
  }
  return -1;  // key not found.
}
