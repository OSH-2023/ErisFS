/*
 * @Description: 
 * @Version: 
 * @Author: Tyrion Huu
 * @Date: 2023-07-05 09:39:28
 * @LastEditors: Tyrion Huu
 * @LastEditTime: 2023-07-05 17:12:25
 */
#include "headers.h"

/* 
    * plaintext or ciphertext 32-bit block number (4, 128 bits), 
        1 block is divided into 4 sub_blocks
    * Each sub_block is 8 bits = 1 character
    * In AES, the specification only allows for 128-bit input, 
        where each block is defined as representing a column (a column with 4 small sub_blocks, 
        each sub_block being 8 bits)
* The number of blocks is fixed (NB = 4) (4 * block size = 128)
*/
#define NB 4 

#define Multiply(x,y)   ((y      & 0x01) * x) \
                      ^ ((y >> 1 & 0x01) * xtime(x)) \
                      ^ ((y >> 2 & 0x01) * xtime(xtime(x))) \
                      ^ ((y >> 3 & 0x01) * xtime(xtime(xtime(x)))) \
                      ^ ((y >> 4 & 0x01) * xtime(xtime(xtime(xtime(x)))))

/**
 *  xtime macro: (input * {02}) mod {1b}  GF(2^8)
 *  02 = x = 00000010(binary) over GF(2^8)
 *  1b = x^8 + x^4 + x^3 + x^1 + 1 = 00011011(binary) over GF(2^8) 
 *  
 *  
 *  (x << 1) -- 代表 input * {02}  = shift 1 bit
 *  (x >> 7) -- input / 2^7
 *  ((x >> 7) & 1) * 0x1b ----
 */
#define xtime(x)   ((x << 1) ^ (((x >> 7) & 0x01) * 0x1b))

int encryptAES(const char *);

int decryptAES(const char *);

