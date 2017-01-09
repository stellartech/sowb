/****************************************************************************
 *    Copyright 2010 Andy Kirkham, Stellar Technologies Ltd
 *    
 *    This file is part of the Satellite Observers Workbench (SOWB).
 *
 *    SOWB is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    SOWB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SOWB.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    $Id: main.cpp 5 2010-07-12 20:51:11Z ajk $
 *    
 ***************************************************************************/

/*
    Not even sure where I lifted this code now, but I think it was the
    FreeBSD project. So any copyright that is attributable to that project
    and the BSD license is acknowledged. This module is dual licensed:-
    
    http://www.opensource.org/licenses/bsd-license.php
    
    Copyright (c) 2010, Stellar Technologies Ltd/Andy Kirkham
    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification, 
    are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this 
      list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, 
      this list of conditions and the following disclaimer in the documentation 
      and/or other materials provided with the distribution.
    * Neither the name of the <ORGANIZATION> nor the names of its contributors may 
      be used to endorse or promote products derived from this software without 
      specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
    NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN 
    IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    
    Additionally see http://www.freebsd.org/copyright/license.html
*/

#include <LPC17xx.h>
#include <cmsis_nvic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MD5 context structure.
 */
typedef struct {
    uint32_t lo, hi;
    uint32_t a, b, c, d;
    unsigned char buffer[64];
    uint32_t block[16];
} MD5_CTX;

/* Used to create human text for MD5 */
static const char hexits[17] = "0123456789ABCDEF";

/**
 * Internal function prototypes.
 */
static void make_digest(char *md5str, unsigned char *digest);
static void md5_init(MD5_CTX *);
static void md5_update(MD5_CTX *, const void *, unsigned int);
static void md5_final(unsigned char *, MD5_CTX *);
static const void *body(MD5_CTX *, const void *, unsigned int);

void md5(const char *s, char *md5str) {

    // A local MD5 context.
    MD5_CTX context;

    // Place to store the raw MD5 digest.
    unsigned char digest[16];

    // Clear the output string and initialise the context.
    md5str[0] = '\0';
    md5_init(&context);

    // Make the MD5 and finalise.
    md5_update(&context, s, strlen(s));
    md5_final(digest, &context);

    // Convert the binary digest into human text in the caller's output buffer.
    make_digest(md5str, digest);
}

/**
 * md5_short
 *
 * As MD5 function but selects 5 fixed characters
 * from the 32 generated to return a short name.
 */
void md5_short(const char *s, char *md5str, int d) {
    char temp[33];
    static const int day[7][5] = {
      { 18,  3,  8, 27, 19 }, // 01
      { 12, 11,  6, 21, 15 }, // 02
      { 27, 14, 12,  4, 11 }, // 03
      { 12, 18, 13,  6,  2 }, // 04
      { 29, 13,  8,  1, 15 }, // 05
      { 13, 12,  9, 30, 22 }, // 06
      { 29, 31,  2,  5, 14 }  // 07
    };
        
    // Get the full MD5 string.
    md5(s, temp);
    
    // Ensure we stay within our array boundaries.
    d &= 0x7;
    
    // Get the shortened version of the MD5.
    md5str[0] = temp[day[d][0]];
    md5str[1] = temp[day[d][1]];
    md5str[2] = temp[day[d][2]];
    md5str[3] = temp[day[d][3]];
    md5str[4] = temp[day[d][4]];
    md5str[5] = '\0';
}

static void make_digest(char *md5str, unsigned char *digest) {
    int i;
    
    for (i = 0; i < 16; i++) {
        md5str[i * 2]       = hexits[digest[i] >> 4];
        md5str[(i * 2) + 1] = hexits[digest[i] & 0x0F];
    }
    
    md5str[32] = '\0';
}

/*
 * This is an OpenSSL-compatible implementation of the RSA Data Security,
 * Inc. MD5 Message-Digest Algorithm (RFC 1321).
 *
 * Written by Solar Designer <solar at openwall.com> in 2001, and placed
 * in the public domain.  There's absolutely no warranty.
 *
 * This differs from Colin Plumb's older public domain implementation in
 * that no 32-bit integer data type is required, there's no compile-time
 * endianness configuration, and the function prototypes match OpenSSL's.
 * The primary goals are portability and ease of use.
 *
 * This implementation is meant to be fast, but not as fast as possible.
 * Some known optimizations are not included to reduce source code size
 * and avoid compile-time configuration.
 */


/*
 * The basic MD5 functions.
 *
 * F and G are optimized compared to their RFC 1321 definitions for
 * architectures that lack an AND-NOT instruction, just like in Colin Plumb's
 * implementation.
 */
#define F(x, y, z)            ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z)            ((y) ^ ((z) & ((x) ^ (y))))
#define H(x, y, z)            ((x) ^ (y) ^ (z))
#define I(x, y, z)            ((y) ^ ((x) | ~(z)))

/*
 * The MD5 transformation for all four rounds.
 */
#define STEP(f, a, b, c, d, x, t, s) \
    (a) += f((b), (c), (d)) + (x) + (t); \
    (a) = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s)))); \
    (a) += (b);

# define SET(n) \
    (ctx->block[(n)] = \
    (uint32_t)ptr[(n) * 4] | \
    ((uint32_t)ptr[(n) * 4 + 1] << 8) | \
    ((uint32_t)ptr[(n) * 4 + 2] << 16) | \
    ((uint32_t)ptr[(n) * 4 + 3] << 24))
# define GET(n) \
    (ctx->block[(n)])

/*
 * This processes one or more 64-byte data blocks, but does NOT update
 * the bit counters.  There are no alignment requirements.
 */
static const void *body(MD5_CTX *ctx, const void *data, unsigned int size) {
    const unsigned char *ptr;
    uint32_t a, b, c, d;
    uint32_t saved_a, saved_b, saved_c, saved_d;

    ptr = (const unsigned char *)data;

    a = ctx->a;
    b = ctx->b;
    c = ctx->c;
    d = ctx->d;

    do {
        saved_a = a;
        saved_b = b;
        saved_c = c;
        saved_d = d;

        /* Round 1 */
        STEP(F, a, b, c, d, SET(0), 0xd76aa478, 7)
        STEP(F, d, a, b, c, SET(1), 0xe8c7b756, 12)
        STEP(F, c, d, a, b, SET(2), 0x242070db, 17)
        STEP(F, b, c, d, a, SET(3), 0xc1bdceee, 22)
        STEP(F, a, b, c, d, SET(4), 0xf57c0faf, 7)
        STEP(F, d, a, b, c, SET(5), 0x4787c62a, 12)
        STEP(F, c, d, a, b, SET(6), 0xa8304613, 17)
        STEP(F, b, c, d, a, SET(7), 0xfd469501, 22)
        STEP(F, a, b, c, d, SET(8), 0x698098d8, 7)
        STEP(F, d, a, b, c, SET(9), 0x8b44f7af, 12)
        STEP(F, c, d, a, b, SET(10), 0xffff5bb1, 17)
        STEP(F, b, c, d, a, SET(11), 0x895cd7be, 22)
        STEP(F, a, b, c, d, SET(12), 0x6b901122, 7)
        STEP(F, d, a, b, c, SET(13), 0xfd987193, 12)
        STEP(F, c, d, a, b, SET(14), 0xa679438e, 17)
        STEP(F, b, c, d, a, SET(15), 0x49b40821, 22)

        /* Round 2 */
        STEP(G, a, b, c, d, GET(1), 0xf61e2562, 5)
        STEP(G, d, a, b, c, GET(6), 0xc040b340, 9)
        STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14)
        STEP(G, b, c, d, a, GET(0), 0xe9b6c7aa, 20)
        STEP(G, a, b, c, d, GET(5), 0xd62f105d, 5)
        STEP(G, d, a, b, c, GET(10), 0x02441453, 9)
        STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14)
        STEP(G, b, c, d, a, GET(4), 0xe7d3fbc8, 20)
        STEP(G, a, b, c, d, GET(9), 0x21e1cde6, 5)
        STEP(G, d, a, b, c, GET(14), 0xc33707d6, 9)
        STEP(G, c, d, a, b, GET(3), 0xf4d50d87, 14)
        STEP(G, b, c, d, a, GET(8), 0x455a14ed, 20)
        STEP(G, a, b, c, d, GET(13), 0xa9e3e905, 5)
        STEP(G, d, a, b, c, GET(2), 0xfcefa3f8, 9)
        STEP(G, c, d, a, b, GET(7), 0x676f02d9, 14)
        STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20)

        /* Round 3 */
        STEP(H, a, b, c, d, GET(5), 0xfffa3942, 4)
        STEP(H, d, a, b, c, GET(8), 0x8771f681, 11)
        STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16)
        STEP(H, b, c, d, a, GET(14), 0xfde5380c, 23)
        STEP(H, a, b, c, d, GET(1), 0xa4beea44, 4)
        STEP(H, d, a, b, c, GET(4), 0x4bdecfa9, 11)
        STEP(H, c, d, a, b, GET(7), 0xf6bb4b60, 16)
        STEP(H, b, c, d, a, GET(10), 0xbebfbc70, 23)
        STEP(H, a, b, c, d, GET(13), 0x289b7ec6, 4)
        STEP(H, d, a, b, c, GET(0), 0xeaa127fa, 11)
        STEP(H, c, d, a, b, GET(3), 0xd4ef3085, 16)
        STEP(H, b, c, d, a, GET(6), 0x04881d05, 23)
        STEP(H, a, b, c, d, GET(9), 0xd9d4d039, 4)
        STEP(H, d, a, b, c, GET(12), 0xe6db99e5, 11)
        STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16)
        STEP(H, b, c, d, a, GET(2), 0xc4ac5665, 23)

        /* Round 4 */
        STEP(I, a, b, c, d, GET(0), 0xf4292244, 6)
        STEP(I, d, a, b, c, GET(7), 0x432aff97, 10)
        STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15)
        STEP(I, b, c, d, a, GET(5), 0xfc93a039, 21)
        STEP(I, a, b, c, d, GET(12), 0x655b59c3, 6)
        STEP(I, d, a, b, c, GET(3), 0x8f0ccc92, 10)
        STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15)
        STEP(I, b, c, d, a, GET(1), 0x85845dd1, 21)
        STEP(I, a, b, c, d, GET(8), 0x6fa87e4f, 6)
        STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10)
        STEP(I, c, d, a, b, GET(6), 0xa3014314, 15)
        STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21)
        STEP(I, a, b, c, d, GET(4), 0xf7537e82, 6)
        STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10)
        STEP(I, c, d, a, b, GET(2), 0x2ad7d2bb, 15)
        STEP(I, b, c, d, a, GET(9), 0xeb86d391, 21)

        a += saved_a;
        b += saved_b;
        c += saved_c;
        d += saved_d;

        ptr += 64;
    } while (size -= 64);

    ctx->a = a;
    ctx->b = b;
    ctx->c = c;
    ctx->d = d;

    return ptr;
}

static void md5_init(MD5_CTX *ctx) {
    ctx->a = 0x67452301;
    ctx->b = 0xefcdab89;
    ctx->c = 0x98badcfe;
    ctx->d = 0x10325476;

    ctx->lo = 0;
    ctx->hi = 0;
}

static void md5_update(MD5_CTX *ctx, const void *data, unsigned int size) {
    uint32_t saved_lo;
    uint32_t used, free;
    
    saved_lo = ctx->lo;
    if ((ctx->lo = (saved_lo + size) & 0x1fffffff) < saved_lo) {
        ctx->hi++;
    }
    ctx->hi += size >> 29;

    used = saved_lo & 0x3f;

    if (used) {
        free = 64 - used;

        if (size < free) {
            memcpy(&ctx->buffer[used], data, size);
            return;
        }

        memcpy(&ctx->buffer[used], data, free);
        data = (unsigned char *)data + free;
        size -= free;
        body(ctx, ctx->buffer, 64);
    }

    if (size >= 64) {
        data = body(ctx, data, size & ~(size_t)0x3f);
        size &= 0x3f;
    }

    memcpy(ctx->buffer, data, size);
}

static void md5_final(unsigned char *result, MD5_CTX *ctx) {
    uint32_t used, free;
    
    used = ctx->lo & 0x3f;
    
    ctx->buffer[used++] = 0x80;

    free = 64 - used;

    if (free < 8) {
        memset(&ctx->buffer[used], 0, free);
        body(ctx, ctx->buffer, 64);
        used = 0;
        free = 64;
    }

    memset(&ctx->buffer[used], 0, free - 8);

    ctx->lo <<= 3;
    ctx->buffer[56] = (unsigned char)ctx->lo;
    ctx->buffer[57] = (unsigned char)(ctx->lo >> 8);
    ctx->buffer[58] = (unsigned char)(ctx->lo >> 16);
    ctx->buffer[59] = (unsigned char)(ctx->lo >> 24);
    ctx->buffer[60] = (unsigned char)(ctx->hi);
    ctx->buffer[61] = (unsigned char)(ctx->hi >> 8);
    ctx->buffer[62] = (unsigned char)(ctx->hi >> 16);
    ctx->buffer[63] = (unsigned char)(ctx->hi >> 24);

    body(ctx, ctx->buffer, 64);

    result[0] = (unsigned char)ctx->a;
    result[1] = (unsigned char)(ctx->a >> 8);
    result[2] = (unsigned char)(ctx->a >> 16);
    result[3] = (unsigned char)(ctx->a >> 24);
    result[4] = (unsigned char)ctx->b;
    result[5] = (unsigned char)(ctx->b >> 8);
    result[6] = (unsigned char)(ctx->b >> 16);
    result[7] = (unsigned char)(ctx->b >> 24);
    result[8] = (unsigned char)ctx->c;
    result[9] = (unsigned char)(ctx->c >> 8);
    result[10] = (unsigned char)(ctx->c >> 16);
    result[11] = (unsigned char)(ctx->c >> 24);
    result[12] = (unsigned char)ctx->d;
    result[13] = (unsigned char)(ctx->d >> 8);
    result[14] = (unsigned char)(ctx->d >> 16);
    result[15] = (unsigned char)(ctx->d >> 24);

    memset(ctx, 0, sizeof(*ctx));
}

#ifdef __cplusplus
}
#endif
