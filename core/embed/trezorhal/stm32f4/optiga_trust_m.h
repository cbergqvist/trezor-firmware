
#ifndef CORE_OPTIGA_TRUST_M_H
#define CORE_OPTIGA_TRUST_M_H

#include <stdint.h>

void optiga_init(void);

int test_optiga_read_UID(uint8_t* buffer);

#endif  // CORE_OPTIGA_TRUST_M_H
