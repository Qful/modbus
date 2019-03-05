/**
@file
CRC Computations

@defgroup util_crc16 "util/crc16.h": CRC Computations
@code#include "util/crc16.h"@endcode

This header file provides functions for calculating
cyclic redundancy checks (CRC) using common polynomials.
Modified by Doc Walker to be processor-independent (removed inline
assembler to allow it to compile on SAM3X8E processors).

@par References:
Jack Crenshaw's "Implementing CRCs" article in the January 1992 issue of @e
Embedded @e Systems @e Programming. This may be difficult to find, but it
explains CRC's in very clear and concise terms. Well worth the effort to
obtain a copy.

*/


#ifndef _UTIL_CRC16_H_
#define _UTIL_CRC16_H_


/** @ingroup util_crc16
    Processor-independent CRC-16 calculation.

    Polynomial: x^16 + x^15 + x^2 + 1 (0xA001)<br>
    Initial value: 0xFFFF

    This CRC is normally used in disk-drive controllers.

    @param uint16_t crc (0x0000..0xFFFF)
    @param uint8_t a (0x00..0xFF)
    @return calculated CRC (0x0000..0xFFFF)
*/
static uint16_t crc16_update(uint16_t crc, uint8_t a)
{
  int i;

  crc ^= a;
  for (i = 0; i < 8; ++i)
  {
    if (crc & 1)
      crc = (crc >> 1) ^ 0xA001;
    else
      crc = (crc >> 1);
  }

  return crc;
}


#endif /* _UTIL_CRC16_H_ */
