/* 
 * Copyright (C) 2014, Galois, Inc.
 * This sotware is distributed under a standard, three-clause BSD license.
 * Please see the file LICENSE, distributed with this software, for specific
 * terms and conditions.
 */
#ifndef MINLIBC_STDLIB_H
#define MINLIBC_STDLIB_H

#define NULL            0
#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1

typedef struct {
	int quot;		/* quotient */
	int rem;		/* remainder */
} div_t;

typedef struct {
	long quot;		/* quotient */
	long rem;		/* remainder */
} ldiv_t;

typedef struct {
	long long quot;
	long long rem;
} lldiv_t;

typedef unsigned long int size_t;

#ifdef __cplusplus
extern "C" {
#endif
	double    atof(const char *nptr);
	int       atoi(const char *nptr);
	long      atol(const char *nptr);
	long long atoll(const char *nptr);
	long int  strtol(const char *nptr, char **endptr, int base);
	double    strtod(char *str, char **ptr);
	float     strtof(const char *string, char **ptr);
	long double strtold(const char *nptr, char **endptr);
	unsigned long long strtoull(const char *__restrict, char **__restrict, int);
	long long strtoll(const char *nptr, char **endptr, int base);
	unsigned long strtoul(const char *nptr, char **endptr, int base);

	typedef int cmp_t(const void *, const void *);
	void      qsort(void *a, size_t n, size_t es, cmp_t *cmp);

	char     *getenv(const char *name);
	void      abort(void) __attribute__((noreturn));
	void      exit(int status) __attribute__((noreturn));

	void     *malloc(size_t size);
	void     *realloc(void *ptr, size_t size);
	void     *calloc(size_t nmemb, size_t size);
	void      free(void *ptr);

	int       mkstemp(char *templ);

	void     *bsearch(const void *, const void *, size_t, size_t,
					int (*)(const void *, const void *));

	int       putenv(char *str);
	int       unsetenv(const char *name);

	int       abs(int j);
	long int  labs(long int j);
	long long int llabs(long long int j);

	div_t div(int numerator, int denominator);
	ldiv_t ldiv(long numerator, long denominator);
	lldiv_t lldiv(long long numerator, long long denominator);

	int rand (void);
	void srand (unsigned);
#ifdef __cplusplus
}
#endif


#endif
