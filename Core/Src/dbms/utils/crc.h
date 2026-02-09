/** 
 * 
 * Distributed BMS      CRC Module
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 *                      Eli Nicksic      <eli.n@tamu.edu>
 */
#ifndef _CRC_H_
#define _CRC_H_

#include <stdint.h>
#include <stdlib.h>

/**
 * Calculate a 16-bit CRC (cyclic redundancy check)
 * on a byte buffer buf of size len.
 *
 * What is a CRC?
 *   https://en.wikipedia.org/wiki/Cyclic_redundancy_check
 * 
 * What is this calculation based off?
 *   https://stackoverflow.com/questions/67614732/calculation-of-crc-16
 * 
 */
uint16_t CalcCrc16(uint8_t* buf, size_t len);

#endif