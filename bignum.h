#ifndef _BIGNUM_H_
#define _BIGNUM_H_

#define KSPACE

#include <stdbool.h>
#include <stddef.h>

typedef struct BLT {
    char value;
    struct BLT *prev;
    struct BLT *next;
} bnlist_t;


typedef struct {
    size_t cnt_d;
    char sign;
    bnlist_t *msd;
    bnlist_t *lsd;
} bignum_t;

//----------------------------------------------------------------
// Memory space operation and carry / borrow operation

/* Create a big number with initialize value zero */
bignum_t *bn_create(void);

/* Free created space after usage */
void bn_free(bignum_t **);

/* MSD carry - including allocation */
int bn_msd_carry(bignum_t **, char);

/* MSD borrow - including space free */
void bn_msd_borrow(bignum_t **, char *);

/* Clean leading zero digit of big number */
void bn_clean_leading_zero(bignum_t **);

//----------------------------------------------------------------
// Arithmetic operation

/* Add two big number */
int bn_add(bignum_t **, bignum_t *, bignum_t *);

/* Subtract two big number */
int bn_sub_for_fib(bignum_t **, bignum_t *, bignum_t *);

/* Multiply two big number */
int bn_mul(bignum_t **, bignum_t *, bignum_t *);

//----------------------------------------------------------------
// Sequence operation

/* Return fibonacci number, store with big number structure */
bignum_t *bn_fibonacci(long long);

/* Return fibonacci number via fast doubling method */
bignum_t *bn_fibonacci_fd(long long);

//----------------------------------------------------------------
// Big number service operation

/* Print big number in Big-endian(MSD first) */
#ifndef KSPACE
void bn_print(bignum_t *);
#endif

/* Transfer big number to string and free */
char *bn_tostring(bignum_t **);

/* Cast long long int to big number */
int bn_cast_from_ll(bignum_t **, long long);

int bn_copy(bignum_t **, bignum_t *);

#endif