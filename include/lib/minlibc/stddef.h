/* 
 * Copyright (C) 2014, Galois, Inc.
 * This sotware is distributed under a standard, three-clause BSD license.
 * Please see the file LICENSE, distributed with this software, for specific
 * terms and conditions.
 */
#ifndef MINLIBC_STDDEF_H
#define MINLIBC_STDDEF_H

typedef int ptrdiff_t;

typedef unsigned long int size_t;
typedef signed   long int ssize_t;

typedef struct {
    long long   nonce1 __attribute__((__aligned__(__alignof__(long long))));
    long double nonce2 __attribute__((__aligned__(__alignof__(long double))));
} max_align_t;

#endif
