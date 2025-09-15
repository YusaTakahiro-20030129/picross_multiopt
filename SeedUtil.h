#ifndef SEED_UTIL_H
#define SEED_UTIL_H

#include "Parameters.h" 

inline int generate_seed(int loop, int operation_id, int individual_id) {
    return BASE_SEED
         + loop * LOOP_OFFSET
         + operation_id * OPERATION_OFFSET
         + individual_id * INDIVIDUAL_OFFSET;
}

#endif