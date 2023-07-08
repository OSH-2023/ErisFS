#include "../include/efs_aes.h"

int NR = 0;     // Number of round(NR), AES-128(10r), AES-192(12), AES-256(14)
int NBK = 0;    // Number of block of key, AES-128(4 block), AES-192(6), AES-256(8)

unsigned char in[16];           // plaintext block input array
unsigned char out[16];          // ciphertext block output array
unsigned char state[4][4];      // temp state array in encrypt state 4 * 4 
unsigned char round_key[240];   // round key array, stored Main key and Expanded key (Ex: AES-128(44words/176 bytes), AES-256(60w/260bytes)), w0(index 0 ~ 3) w1(index 4 ~ 7)....
unsigned char key[32];          // Main key(input key Ex. AES-128(18 char), AES-256(32 char)), 

/* S-box */
int substitution_box[256] =   
{
    //0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76, //0
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, //1
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, //2
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, //3
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, //4
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, //5
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8, //6
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, //7
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73, //8
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, //9
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, //A
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, //B
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, //C
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, //D
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, //E
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16  //F
};

/**
 * round_constant used in KeyExpansion
 * Table generate from GF(2^8) 
 * round_constant[0] will not be used(Easy to code), set any redundant num
 * AES uses up to round constant[10] for AES-128 (as 11 round keys are needed), up to round constant[8] for AES-192, and up to round constant[7] for AES-256.
 */
// round constant word array
int round_constant[11] = 
{
//   0     1     2     3      4    5     6     7     8    9     10
    0x87, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

/* Inverse S-box */
int substitution_box_inverse[256] =   
{
    //0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d 
};

/** 
 *  key Expansion function, 
 *  Input: key[](Main key), NR(round), NB, NBK(AES-128(4 block), AES-192(6), AES-256(8))
 *  Output: round_key[], subkey - AES-128(44), 192(52), 256(60), 
 */
void KeyExpansion(void)
{
    unsigned char temp_byte[4]; // store 4 temp Byte(1 word) when generate subkey
    unsigned char a0;       // temp - store byte when execute RotWord function
    int i;
    int j;
    /**
     * First Round subKey = Main/Input key, Divide to {NBK} block (each 32bits) [w0, w1, w2, w3]
     * each block divide to 4 sub_block(8bit)
     * Ex: AES-128, NBK = 4, 4 block W0 ~ W3
     * Ex: AES-256, NBK = 8, 8 block W0 ~ W7
     */
    for (i = 0; i < NBK; i++)
    {
        round_key[i * 4] = key[i * 4];
        round_key[i * 4 + 1] = key[i * 4 + 1];
        round_key[i * 4 + 2] = key[i * 4 + 2];
        round_key[i * 4 + 3] = key[i * 4 + 3];
    }
    /**
     * Generate other subkey, 
     * Ex: AES-128: i= 4 ~ 43, need 11 4block(128bit), need 44 word (W0 ~ W43).
     * Ex: AES-256: i = 8 ~ 59, need 15 4block(128bit), need 60 word (W0~ W59)
     */
    for (i = NBK; i < (NB * (NR + 1)); i++)
    {
        for (j = 0; j < 4; j++)
        { 
            // process 4 byte(1 word) at a time
            temp_byte[j] = round_key[(i - 1) * 4 + j];  // need to -1, because round_key[i] is not generated yet
        }
        if (i % NBK == 0)
        {
            /**
             * Ex: AES-128 when generate W4, will use W3 do SubWord(RotWord(tempW)) XOR round_constant[4/4]
             *     AES-128 i is 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44...
             */
            // RotWord function, [a0, a1, a2, a3](4byte) left circular shift in a word [a1, a2, a3, a0]
            a0 = temp_byte[0];
            temp_byte[0] = temp_byte[1];
            temp_byte[1] = temp_byte[2];
            temp_byte[2] = temp_byte[3];
            temp_byte[3] = a0;

            // SubWord function (S-Box substitution)
            temp_byte[0] = substitution_box[(int)temp_byte[0]];
            temp_byte[1] = substitution_box[(int)temp_byte[1]];
            temp_byte[2] = substitution_box[(int)temp_byte[2]];
            temp_byte[3] = substitution_box[(int)temp_byte[3]];
            
            // XOR round_constant[i/4], only leftmost byte are changed (只會XOR最左的byte)
            temp_byte[0] = temp_byte[0] ^ round_constant[i / NBK]; 
        }
        else if (NBK == 8 && i % NBK == 4)
        {
            // Only AES-256 used, 
            // when i mod 8 = 4, need to do SubWord function
            temp_byte[0] = substitution_box[(int)temp_byte[0]];
            temp_byte[1] = substitution_box[(int)temp_byte[1]];
            temp_byte[2] = substitution_box[(int)temp_byte[2]];
            temp_byte[3] = substitution_box[(int)temp_byte[3]];
        }
        /**
         * Wn = Wn-1 XOR Wk    k = current word - NBK
         * Ex: AES-128   NBK = 4  when W5 = Wn-1(W4) XOR Wk(W1)
         * Ex: AES-256   NBK = 8  when W10 = Wn-1(W9) XOR Wk(W2) 
         */
        round_key[i * 4 + 0] = round_key[(i - NBK) * 4 + 0] ^ temp_byte[0];
        round_key[i * 4 + 1] = round_key[(i - NBK) * 4 + 1] ^ temp_byte[1];
        round_key[i * 4 + 2] = round_key[(i - NBK) * 4 + 2] ^ temp_byte[2];
        round_key[i * 4 + 3] = round_key[(i - NBK) * 4 + 3] ^ temp_byte[3];   
    }
}

/**
 *  Cipher() AES encrypt function
 *  Input: in[16] plaintext block(128 bits), NR (Number of round), key[]
 *  output: out[16] cipher block(128 bits), 
 */
void AddRoundKey(int round)
{
    /**
     * first key index = round * 16 bytes = round * NB * 4;
     * NB = 4
     */
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            state[j][i] ^= round_key[(i * NB + j) + (round * NB * 4)]; 
        }
    }
    return;
}

// S-Box Substitution
void SubBytes(void)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0;j < 4;j++)
        {
            state[i][j] = substitution_box[state[i][j]];
        }
    }
    return;
}

// left Circular Shift (row), 
void ShiftRows(void)
{
    unsigned char temp_byte;
    
    // 2nd row left Circular Shift 1 byte
    temp_byte   = state[1][0];
    state[1][0] = state[1][1];
    state[1][1] = state[1][2];
    state[1][2] = state[1][3];
    state[1][3] = temp_byte;

    // 3th row left Circular Shift 2 byte
    temp_byte   = state[2][0];
    state[2][0] = state[2][2];
    state[2][2] = temp_byte;

    temp_byte    = state[2][1];
    state[2][1] = state[2][3];
    state[2][3] = temp_byte;

    // 4th row left Circular Shift 3 byte
    temp_byte   = state[3][0];
    state[3][0] = state[3][3];
    state[3][3] = state[3][2];
    state[3][2] = state[3][1];
    state[3][1] = temp_byte;
    return;
}

/** 
 *  MixColumns() 混合行運算函數 
 *  執行4次(4 sub_block) 每次的column執行如下
 *  c0     [2 3 1 1   [b0  
 *  c1      1 2 3 1    b1
 *  c2  =   1 1 2 3    b2
 *  c3      3 1 1 2]   b3]
 * 
 * 此為一線性轉換(linear transform)
 */
void MixColumns(void)
{
    unsigned char Tmp, Tm, t;
    for(int i = 0; i < 4; i++)
    {    
        t   = state[0][i];
        Tmp = state[0][i] ^ state[1][i] ^ state[2][i] ^ state[3][i];
        Tm  = state[0][i] ^ state[1][i]; 
        Tm = xtime(Tm); 
        state[0][i] ^= Tm ^ Tmp ;
        Tm  = state[1][i] ^ state[2][i]; 
        Tm = xtime(Tm); state[1][i] ^= Tm ^ Tmp ;
        Tm  = state[2][i] ^ state[3][i]; 
        Tm = xtime(Tm); state[2][i] ^= Tm ^ Tmp ;
        Tm  = state[3][i] ^ t;           
        Tm = xtime(Tm); state[3][i] ^= Tm ^ Tmp ;
    }
    return;
}

void Cipher(void)
{
    int round = 0;
    
    /**
     *  [b0 b1 ... b15] -> [b0 b4 b8  b12
     *                      b1 b5 b9  b13
     *                      b2 b6 b10 b14
     *                      b3 b7 b11 b15]
     */
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            state[j][i] = in[i * 4 + j]; // transform input(plaintext)
        }
    }

    // round 0 : add round key
    AddRoundKey(0);

    // Round 1 ~ NR-1, 
    for (round = 1; round < NR; round++)
    {
        SubBytes();
        ShiftRows();
        MixColumns();
        AddRoundKey(round);
    }

    // Round NR, no MixColumns()
    SubBytes();
    ShiftRows();
    AddRoundKey(NR);

    /**
     *   [c0 c4 c8  c12
     *    c1 c5 c9  c13    --> [c0 c1 c2 ... c15]
     *    c2 c6 c10 c14
     *    c3 c7 c11 c15]
     */
    for(int i = 0; i < 4; i++) 
    {
        for(int j = 0; j < 4; j++)
        {    
            out[i * 4 + j] = state[j][i];
        }
    }
    return;
}

void printUnsignedCharArrayToInt(unsigned char in[], int size)
{
    for (int i = 0; i < size; i++)
    {
        printf("%d ", in[i]);
    }
    return;
}

// Inverse S-Box Substitution
void SubBytesInverse(void)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            state[i][j] = substitution_box_inverse[state[i][j]];
        }
    }
    return;
}

// right(inverse) Circular Shift (row), 
void ShiftRowsInverse(void)
{
    unsigned char temp_byte;
    
    // 2nd row right Circular Shift 1 byte
    temp_byte   = state[1][3];
    state[1][3] = state[1][2];
    state[1][2] = state[1][1];
    state[1][1] = state[1][0];
    state[1][0] = temp_byte;

    // 3th row right Circular Shift 2 byte
    temp_byte   = state[2][0];
    state[2][0] = state[2][2];
    state[2][2] = temp_byte;

    temp_byte   = state[2][1];
    state[2][1] = state[2][3];
    state[2][3] = temp_byte;

    // 4th row right Circular Shift 3 byte
    temp_byte   = state[3][0];
    state[3][0] = state[3][1];
    state[3][1] = state[3][2];
    state[3][2] = state[3][3];
    state[3][3] = temp_byte;
    return;
}

/** InvMixColumns()  
 * 
 *  執行4次(4 sub_block) 每次的column執行如下
 *  b0     [14 11 13  9   [d0  
 *  b1       9 14 11 13    d1
 *  b2  =   13  9 14 11    d2
 *  b3      11 13  9 14]   d3]
 *  
 *  詳細執行可查看wiki: https://en.wikipedia.org/wiki/Rijndael_MixColumns
 */
void MixColumnsInverse(void)
{
    unsigned char d0, d1, d2, d3;
    for (int i = 0; i < 4; i++)
    {
        d0 = state[0][i];
        d1 = state[1][i];
        d2 = state[2][i];
        d3 = state[3][i];

        state[0][i] =   Multiply(d0, 0x0e) ^ Multiply(d1, 0x0b) ^ 
                        Multiply(d2, 0x0d) ^ Multiply(d3, 0x09);
        state[1][i] =   Multiply(d0, 0x09) ^ Multiply(d1, 0x0e) ^ 
                        Multiply(d2, 0x0b) ^ Multiply(d3, 0x0d);
        state[2][i] =   Multiply(d0, 0x0d) ^ Multiply(d1, 0x09) ^ 
                        Multiply(d2, 0x0e) ^ Multiply(d3, 0x0b);
        state[3][i] =   Multiply(d0, 0x0b) ^ Multiply(d1, 0x0d) ^ 
                        Multiply(d2, 0x09) ^ Multiply(d3, 0x0e);
    }
    return;
}

/**
 *  CipherInverse  AES Decrypt function
 *  Inverse to Encrypt process
 */
void CipherInverse(void)
{
    int round = NR - 1;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            state[j][i] = in[i * 4 + j]; // transform input
        }
    }    
    // round NR : add round key
    AddRoundKey(NR);

    for (round = NR - 1; round > 0; round--)
    {
        ShiftRowsInverse();
        SubBytesInverse();
        AddRoundKey(round);
        MixColumnsInverse();
    }

    // Round NR, no MixColumns()
    ShiftRowsInverse();
    SubBytesInverse();
    AddRoundKey(0);

    /**
     *   [b0 b4 b8  b12
     *    b1 b5 b9  b13    --> [b0 b1 b2 ... b15]
     *    b2 b6 b10 b14
     *    b3 b7 b11 b15]
     */
    for(int i = 0; i < 4; i++) 
    {
        for(int j = 0; j < 4; j++)
        {
            out[i * 4 + j] = state[j][i];
        }
    }
    return;
}

// key_size is 128 or 192 or 256
// input_key is 16 or 24 or 32 bytes    (char)
// file_path is input file path
// new_file is output file path
int encryptAES(const char * file_path, const char * new_file, const int key_size, const char * input_key)
{
    int fd0, fd1;                       // file descriptor
    int EOF_flag = 0;                   // detect end of file flag
    unsigned char plaintext_block[16];  // plaintext, encrypt each block (128bit) once

    //printf("----- AES file encryption -----\r\n");

    NBK = key_size / 32;        // Number of block of key
    NR   = NBK + 6;             // Number of round(NR)

    if (key_size == 128)
    {
        for (int i = 0; i < 16; i++)
        {
            key[i] = input_key[i];
        }
    }
    else if (key_size == 192)
    {
        for (int i = 0; i < 24; i++)
        {
            key[i] = input_key[i];
        }
    }
    else if (key_size == 256)
    {
        for (int i = 0; i < 32; i++)
        {
            key[i] = input_key[i];
        }
    }
    else
    {
        printf("Error input to key_size, Exit Now!");
        return -1;
    }
    /* key Expansion function */
    KeyExpansion(); // Expansion key - AES-128(44words/176 bytes), AES-192(52w/208 bytes), AES-256(60w/260bytes)

    if ((fd0 = efs_open(file_path, O_RDWR, 0)) == -1)
    {
        printf("Open file Error...\r\n");
        return -1;
    }

    /* get output Ciphertext */
    if ((fd1 = efs_open(new_file, O_RDWR | O_CREAT, 0)) == -1)
    {
        printf("Create file Error...\r\n");
        return -1;
    }

    int blockNum = 0; // record processing block number(128 bit)
    char ch;
    EOF_flag = 1;
    struct efs_file * d = fd_get(fd0);
    unsigned long int bytes = d->vnode->size;
    unsigned long int a = bytes/16;
    int i=0;
    //printf("---------------------------------------------\r\n");
    while(i++<= a)
    {
        /**
         *  read file, read 16 char (1block, 128bit) 
         *  if last block not fill 128bit, add 0x00 to fill with last block
         */
        for (int c = 0; c < 16; c++)
        {
            lseek(fd0, blockNum * 16 + c, SEEK_SET);
            read(fd0, &ch, 1);
            plaintext_block[c] = ch;
            if (ch == EOF)
            {
                for (int padding = c; padding < 16; padding++)
                {
                    plaintext_block[padding] = 0x00;
                }
                EOF_flag = 0;
            }
        }

        for (int c = 0; c < 16; c++)
        {
            in[c] = plaintext_block[c];
        }

        /**
         * Call Encrypt  function, encrypt one block (128 bit) once
         * input: in[](plaintext), key[](key)
         * output: out[](cipher) 
         */
        Cipher(); 

        /* Write Ciphertext to file */
        for(int c = 0; c < 16; c++)  
        {
            lseek(fd1, blockNum * 16 + c, SEEK_SET);
            write(fd1, &out[c], 1);
        }
        
        // print plaintext(character format) in Integer Format
        //printf("Block %d(128 bits) - plaintext.txt(Int format) : ", blockNum);
        //printUnsignedCharArrayToInt(in, 16);    
        //printf("\r\n");
        // print Cipher(character format) in Integer Format
        //printf("Block %d(128 bits) - Cipher(Int format) : ", blockNum); // print ciphertext(char) in integer format
        //printUnsignedCharArrayToInt(out, 16);
        //printf("\r\n");
        blockNum++;
    }
    close(fd0);
    lseek(fd1, 0, SEEK_SET);
    close(fd1);
    //printf("------------------------------------------------\r\n");
    //printf("Encryption process complete !! \r\n");
    return 0;
}

// key_size is 128 or 192 or 256
// input_key is 16 or 24 or 32 bytes    (char)
// file_path is input file path
// new_file is output file path
int decryptAES(const char * file_path, const char * new_file, const int key_size, const char * input_key)
{   
    int fd0, fd1; // file descriptor
    int EOF_flag = 0; // detect end of file flag
    unsigned char Ciphertext_block[16]; // plaintext, encrypt each block (128bit) once

    //printf("*** AES decryption System ***\r\n");

    NBK = key_size / 32;     // Number of block of key
    NR   = NBK + 6;         // Number of round(NR)
    
    if (key_size == 128)
    {
        for (int i = 0; i < 16; i++)
        {
            key[i] = input_key[i];
        }
    }
    else if (key_size == 192)
    {
        for (int i = 0; i < 24; i++)
        {
            key[i] = input_key[i];
        }
    }
    else if (key_size == 256)
    {
        for (int i = 0; i < 32; i++)
        {
            key[i] = input_key[i];
        }
    }
    else
    {
        printf("Error input to key_size, Exit Now!");
        return -1;
    }
    /* key Expansion function */
    KeyExpansion(); // Expansion key - AES-128(44words/176 bytes), AES-192(52w/208 bytes), AES-256(60w/260bytes)

    if ((fd0 = efs_open(file_path, O_RDONLY, 0)) == -1)
    {
        printf("Open file Error...\r\n");
        return -1;
    }

    /* get output decrypted plaintext */
    if ((fd1 = efs_open(new_file, O_WRONLY | O_CREAT, 0)) == -1)
    {
        printf("Create file Error...\r\n");
        return -1;
    }

    int blockNum = 0; // record processing block number(128 bit)
    char ch;
    EOF_flag = 1;
    struct efs_file * d = fd_get(fd0);
    unsigned long int bytes = d->vnode->size;
    unsigned long int a = bytes/16;
    int i=0;
    // printf("---------------------------------------------\r\n");
    while(++i<=a)
    {
        /**
         *  read file, read 16 char (1block, 128bit) 
         */
        for (int c = 0; c < 16; c++)
        {
            lseek(fd0, blockNum * 16 + c, SEEK_SET);
            read(fd0, &ch, 1);
            Ciphertext_block[c] = ch;
            in[c] = Ciphertext_block[c];
        }

        /**
         * Call Decrypt  function, Decrypt one block (128 bit) once
         * input: in[](ciphertext), key[](key)
         * output: out[](plaintext) 
         */
        CipherInverse(); 
        
        // write decrypted plaintext to file and discard padding byte
        for (int c = 0; c < 16; c++)
        {
            // discard padding byte
            if (out[c] == 0x00)
            {
                EOF_flag = 0;
                break;
            }
            lseek(fd1, blockNum * 16 + c, SEEK_SET);
            write(fd1, &out[c], 1);
        }

        // print Cipher(character format) in Integer Format
        // print plaintext(character format) in Integer Format
        // printf("Block %d(128 bits) - plaintext.txt(Int format) : ", blockNum);
        //printUnsignedCharArrayToInt(in, 16);    
        // printf("\r\n");
        // print Decrypted plaintext(character format) in Integer Format
        // printf("Block %d(128 bits) - Cipher(Int format) : ", blockNum); 
        //printUnsignedCharArrayToInt(out, 16);
        // printf("\r\n");
        blockNum++;
    }

    close(fd0);
    lseek(fd1, 0, SEEK_SET);
    close(fd1);
    //printf("------------------------------------------------\r\n");
    //printf("Decryption process complete !! \r\n");
    return 0;
}