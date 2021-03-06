/*
 * Copyright (c) 2018,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  SWAN256.h
 *
 *  Description: SWAN256 with a clear structure. The key schedule is on-the-fly computed.
 *  Created on: 2018-12-24
 *  Last modified: 2019-01-13
 *  Author: Zheng Gong, Weijie Li, Guohong Liao, Bing Sun, Siwei Sun, Tao Sun, Guojun Tang.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include<SWAN.h>

/*
    Phi = 1.61803398874989484820458683436563811772
    DELTA = int(2**128 / Phi)
*/
#define DELTA 0x9e3779b97f4a7c15f39cc0605cedc834
/*
    DELTA_VER = DELTA
    for i in range(0,ROUNDS256):
        DELTA_VER = (DELTA_VER + DELTA) % 2**128
*/

#define DELTA_VER 0x2c15e81951e98192daccd877985fd534

#define DELTA1 0x9e3779b9
#define DELTA2 0x7f4a7c15
#define DELTA3 0xf39cc060
#define DELTA4 0x5cedc834

#define DELTA_VER1 0xb9f45679
#define DELTA_VER2 0x2488870f
#define DELTA_VER3 0xc1fcf08e
#define DELTA_VER4 0xd3d1e234


#define SWAN256_encrypt(plain, key, cipher) SWAN256_encrypt_rounds((plain), (key), ROUNDS_256, (cipher))
#define SWAN256_decrypt(cipher, key, plain) SWAN256_decrypt_rounds((cipher), (key), ROUNDS_256, (plain))

const uint32_t delta[4] = {DELTA4, DELTA3, DELTA2, DELTA1};
const uint32_t delta_ver[4] = {DELTA_VER4, DELTA_VER3, DELTA_VER2, DELTA_VER1};

void ADD128(uint32_t a[4], const uint32_t b[4])
{
    uint64_t *a1 = (uint64_t *)a;
    uint64_t *b1 = (uint64_t *)b;
    uint64_t M = (((a1[0] & b1[0]) & 1) + (a1[0] >> 1) + (b1[0] >> 1)) >> 63;
    a1[1] = a1[1] + b1[1] + M;
    a1[0] = b1[0] + a1[0];
}
// a -=b
void MINUS128(uint32_t a[4], const uint32_t b[4])
{
    uint64_t *a1 = (uint64_t *)a;
    uint64_t *b1 = (uint64_t *)b;
    uint64_t M;
    a1[0] = a1[0] - b1[0];

    M = (((a1[0] & b1[0]) & 1) + (b1[0] >> 1) + (a1[0] >> 1)) >> 63;

    a1[1] = a1[1] - (b1[1] + M);
}

//The bitslicing of SWAN Sboxes for the Beta function, a[0] and b[0] is the lsb bit of sbox input;
//SWAN_SBox = {0x01, 0x02, 0x0C, 0x05, 0x07, 0x08, 0x0A, 0x0F, 0x04, 0x0D, 0x0B, 0x0E, 0x09, 0x06, 0x00, 0x03}
//y[0] = 1 + x[0] + x[1] + x[3] + x[2]x[3])
//y[1] = x[0] + x[0]x1 + x[2] + x[0]x[3] + x[1]x[3] + x[0]x[1]x[3] + x[2]x[3] + x[0]x[2]x[3] + x[1]x[2]x[3]
//y[2] = x[1] + x[2] + x[0]x[2] + x[3] + x[0]x[1]x[3] + x[1]x[2]x[3]
//y[3] = x[1] + x[0]x[1] + x[0]x[2] + x[0]x[3] + x[2]x[3] + x[0]x[2]x[3]




//SwitchLanes:The second affine function after the Beta function;

//d = 15 bytes

void SWAN256_encrypt_rounds(const uint32_t *plain, const uint32_t *masterkey, const uint8_t rounds, uint32_t *cipher)
{
    uint8_t i;
    uint32_t L[4];
    uint32_t R[4];
    uint32_t tempL[4];
    uint32_t tempR[4];
    uint32_t key[KEY256 / 32];
    uint32_t subkey[4];
    uint32_t round_constant[4];
    memcpy(key, masterkey, KEY256 / 8);

    //initialize the plaintext as the first round input;
    L[0] = plain[0];
    L[1] = plain[1];
    L[2] = plain[2];
    L[3] = plain[3];

    R[0] = plain[4];
    R[1] = plain[5];
    R[2] = plain[6];
    R[3] = plain[7];

    //init round_constant
    memset(round_constant, 0, sizeof(round_constant));

    for (i = 1; i <= rounds; i++)
    {
        //first half round encryption;
        tempL[0] = L[0];
        tempL[1] = L[1];
        tempL[2] = L[2];
        tempL[3] = L[3];

        ShiftLanes(tempL,BLOCK256);

        RotateKeyByte(key, KEY256,ROTATE_256);

        subkey[0] = key[0];
        subkey[1] = key[1];
        subkey[2] = key[2];
        subkey[3] = key[3];

        //Modular add the subkey with the delta value;
        //round_constant = round_constant + DELTA;
        ADD128(round_constant, delta);
        //AddRoundConstant(subkey, round_constant);
        ADD128(subkey, round_constant);
        //update the round key K_i with the subkey+delta_i
        key[0] = subkey[0];
        key[1] = subkey[1];
        key[2] = subkey[2];
        key[3] = subkey[3];

        tempL[0] = tempL[0] ^ subkey[0];
        tempL[1] = tempL[1] ^ subkey[1];
        tempL[2] = tempL[2] ^ subkey[2];
        tempL[3] = tempL[3] ^ subkey[3];

        Beta(tempL,BLOCK256);

        SwitchLanes(tempL,BLOCK256);

        R[0] = R[0] ^ tempL[0];
        R[1] = R[1] ^ tempL[1];
        R[2] = R[2] ^ tempL[2];
        R[3] = R[3] ^ tempL[3];

        //Second half round encryption
        tempR[0] = R[0];
        tempR[1] = R[1];
        tempR[2] = R[2];
        tempR[3] = R[3];

        ShiftLanes(tempR,BLOCK256);

        RotateKeyByte(key, KEY256,ROTATE_256);
        subkey[0] = key[0];
        subkey[1] = key[1];
        subkey[2] = key[2];
        subkey[3] = key[3];
        ADD128(round_constant, delta);
        //AddRoundConstant(subkey, round_constant);
        ADD128(subkey, round_constant);
        //update the round key K_i with the subkey+delta_i
        key[0] = subkey[0];
        key[1] = subkey[1];
        key[2] = subkey[2];
        key[3] = subkey[3];

        tempR[0] = tempR[0] ^ subkey[0];
        tempR[1] = tempR[1] ^ subkey[1];
        tempR[2] = tempR[2] ^ subkey[2];
        tempR[3] = tempR[3] ^ subkey[3];

        Beta(tempR,BLOCK256);

        SwitchLanes(tempR,BLOCK256);

        L[0] = L[0] ^ tempR[0];
        L[1] = L[1] ^ tempR[1];
        L[2] = L[2] ^ tempR[2];
        L[3] = L[3] ^ tempR[3];
    }

    //output the ciphertext;
    cipher[0] = L[0];
    cipher[1] = L[1];
    cipher[2] = L[2];
    cipher[3] = L[3];

    cipher[4] = R[0];
    cipher[5] = R[1];
    cipher[6] = R[2];
    cipher[7] = R[3];
}

void SWAN256_decrypt_rounds(const uint32_t *cipher, const uint32_t *masterkey, const uint8_t rounds, uint32_t *plain)
{
    uint8_t i;
    uint32_t L[4];
    uint32_t R[4];
    uint32_t tempL[4];
    uint32_t tempR[4];
    uint32_t key[KEY256 / 32];
    uint32_t subkey[4];
    uint32_t temp_constant[4];
    uint32_t round_constant[4] = {DELTA_VER4, DELTA_VER3, DELTA_VER2, DELTA_VER1};
    memcpy(key, masterkey, KEY256 / 8);

    memset(temp_constant, 0, sizeof(temp_constant));
    //Rotate the key to the final round state;
    for (i = 1; i <= 2 * rounds; i++)
    {
        RotateKeyByte(key, KEY256,ROTATE_256);

        subkey[0] = key[0];
        subkey[1] = key[1];
        subkey[2] = key[2];
        subkey[3] = key[3];

        ADD128(temp_constant, delta);
        ADD128(subkey, temp_constant);
        key[0] = subkey[0];
        key[1] = subkey[1];
        key[2] = subkey[2];
        key[3] = subkey[3];
    }
    RotateKeyByte(key, KEY256,ROTATE_256);

    //initialize the ciphertext as the first decryption round input;
    L[0] = cipher[0];
    L[1] = cipher[1];
    L[2] = cipher[2];
    L[3] = cipher[3];

    R[0] = cipher[4];
    R[1] = cipher[5];
    R[2] = cipher[6];
    R[3] = cipher[7];

    for (i = 1; i <= rounds; i++)
    {
        //First half round decryption;
        tempR[0] = R[0];
        tempR[1] = R[1];
        tempR[2] = R[2];
        tempR[3] = R[3];

        ShiftLanes(tempR,BLOCK256);

        //Generate the final round decryption subkey;
        InvRotateKeyByte(key, KEY256,ROTATE_256);
        subkey[0] = key[0];
        subkey[1] = key[1];
        subkey[2] = key[2];
        subkey[3] = key[3];

        tempR[0] = tempR[0] ^ subkey[0];
        tempR[1] = tempR[1] ^ subkey[1];
        tempR[2] = tempR[2] ^ subkey[2];
        tempR[3] = tempR[3] ^ subkey[3];

        //Modular minus the subkey with the delta value;

        MINUS128(round_constant, delta);

        MINUS128(subkey, round_constant);

        //update the round key K_i with the subkey+delta_i
        key[0] = subkey[0];
        key[1] = subkey[1];
        key[2] = subkey[2];
        key[3] = subkey[3];

        Beta(tempR,BLOCK256);

        SwitchLanes(tempR,BLOCK256);

        L[0] = L[0] ^ tempR[0];
        L[1] = L[1] ^ tempR[1];
        L[2] = L[2] ^ tempR[2];
        L[3] = L[3] ^ tempR[3];

        tempL[0] = L[0];
        tempL[1] = L[1];
        tempL[2] = L[2];
        tempL[3] = L[3];

        //Second half round decryption
        ShiftLanes(tempL,BLOCK256);

        //inverse rotate the key for subkey;

        InvRotateKeyByte(key, KEY256,ROTATE_256);

        subkey[0] = key[0];
        subkey[1] = key[1];
        subkey[2] = key[2];
        subkey[3] = key[3];

        tempL[0] = tempL[0] ^ subkey[0];
        tempL[1] = tempL[1] ^ subkey[1];
        tempL[2] = tempL[2] ^ subkey[2];
        tempL[3] = tempL[3] ^ subkey[3];

        //Modular minus the subkey with the delta value;

        MINUS128(round_constant, delta);
        MINUS128(subkey, round_constant);
        //update the round key K_i with the subkey+delta_i
        key[0] = subkey[0];
        key[1] = subkey[1];
        key[2] = subkey[2];
        key[3] = subkey[3];

        Beta(tempL,BLOCK256);

        SwitchLanes(tempL,BLOCK256);

        R[0] = tempL[0] ^ R[0];
        R[1] = tempL[1] ^ R[1];
        R[2] = tempL[2] ^ R[2];
        R[3] = tempL[3] ^ R[3];
    }

    //output the plaintext;
    plain[0] = L[0];
    plain[1] = L[1];
    plain[2] = L[2];
    plain[3] = L[3];

    plain[4] = R[0];
    plain[5] = R[1];
    plain[6] = R[2];
    plain[7] = R[3];
}
