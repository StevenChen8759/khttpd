#include <linux/slab.h>

#include "bignum.h"

/* Create a big number with initialize value zero */
bignum_t *bn_create(void)
{
    /* Local variable declaration */
    bignum_t *bn_new;

    /* Allocate big number`s memory space */
    bn_new = (bignum_t *) kcalloc(1, sizeof(bignum_t), GFP_KERNEL);

    /* Check if allocated space is null */
    if (bn_new == NULL)
        return NULL;

    /* Allocate space for first number (as list) */
    bn_new->lsd = kcalloc(1, sizeof(bnlist_t), GFP_KERNEL);

    /* Allocate node fail, free allocated space and return NULL */
    if (bn_new->lsd == NULL)
        goto bncreate_FAA;

    /* Assign cnt_d value */
    bn_new->cnt_d = 1;

    /* Pointer operation for big number */
    bn_new->msd = bn_new->lsd;

    /* Result return */
    return bn_new;

bncreate_FAA:

    // Free allocated space and return
    kfree(bn_new);
    return NULL;
}

/* Free created space after usage */
void bn_free(bignum_t **bnum)
{
    /* Local variable declaration */
    bnlist_t *ptr;

    /* Return while bnum is NULL */
    if (*bnum == NULL)
        return;

    /* Pointer assignment*/
    ptr = (*bnum)->msd;

    /* free the list */
    while (ptr != NULL) {
        (*bnum)->msd = (*bnum)->msd->next;
        kfree(ptr);
        ptr = (*bnum)->msd;
    }

    /* free the number structure */
    kfree(*bnum);
    *bnum = NULL;  // Points to NULL for safety consideration
}

/* MSD carry - including allocation */
int bn_msd_carry(bignum_t **ptr, char carryval)
{
    // local variable declaration
    bnlist_t *newdigit;

    // Input NULL, return error code -1
    if (*ptr == NULL)
        return -1;

    // Allocate new node for carry digit
    newdigit = (bnlist_t *) kcalloc(1, sizeof(bnlist_t), GFP_KERNEL);

    // Return while failed allocation
    if (newdigit == NULL)
        return -2;

    // Assign carry value
    newdigit->value = carryval;

    // Pointer assignment for list node linking
    newdigit->next = (*ptr)->msd;
    (*ptr)->msd->prev = newdigit;

    // Move msd pointer and add digit count value
    (*ptr)->msd = newdigit;
    (*ptr)->cnt_d++;

    return 0;
}

/* MSD borrow - including space free */
/*void bn_msd_borrow(bignum_t **ptr, char *borrowval)
{
    // TODO: Process with msd = lsd case!

    // Input NULL, return error code -1
    if (*ptr == NULL)
        return;

    // Assign borrow value
    *borrowval = (*ptr)->msd->value;

    // Move MSD pointer
    (*ptr)->msd = (*ptr)->msd->next;

    // Free MSD and assign pointers and parameters
    kfree((*ptr)->msd->prev);
    (*ptr)->msd->prev = NULL;
    (*ptr)->cnt_d--;
}*/

/* Clean leading zero digit of big number */
void bn_clean_leading_zero(bignum_t **ptr)
{
    // Input NULL, return error code -1
    if (*ptr == NULL)
        return;

    // Clear leading zero in big number
    while ((*ptr)->msd->value == 0 && (*ptr)->cnt_d != 1) {
        // Move MSD pointer
        (*ptr)->msd = (*ptr)->msd->next;

        // Free MSD and assign pointers and parameters
        kfree((*ptr)->msd->prev);
        (*ptr)->msd->prev = NULL;
        (*ptr)->cnt_d--;
    }
}

//----------------------------------------------------------------
// Arithmetic operation

/* Add two big number */
int bn_add(bignum_t **dst, bignum_t *src_1, bignum_t *src_2)
{
    /* Variable declaration */
    bignum_t *bn_dst;
    bnlist_t *ptr_dst, *ptr_src1, *ptr_src2;
    char carry;

    /* Reject NULL source input */
    if (src_1 == NULL || src_2 == NULL)
        return -1;  // NULL source input

    /* Create temperally bignum space */
    bn_dst = bn_create();

    if (bn_dst == NULL)
        return -2;  // Failed Allocation

    /* Assign initial pointer */
    ptr_dst = bn_dst->lsd;
    ptr_src1 = src_1->lsd;
    ptr_src2 = src_2->lsd;

    /* Run addition */
    while (ptr_src1 != NULL || ptr_src2 != NULL) {
        // For dst mod 10 = 0 case, allocate new digit if needed
        if (ptr_dst == NULL) {
            if (bn_msd_carry(&bn_dst, 0) != 0) {
                bn_free(&bn_dst);
                return -2;
            }

            // move to MSD position
            ptr_dst = bn_dst->msd;
        }

        if (ptr_src1 != NULL)
            ptr_dst->value += ptr_src1->value;

        if (ptr_src2 != NULL)
            ptr_dst->value += ptr_src2->value;

        /* Do digit carry to next higher one directly */
        if (ptr_dst->value >= 10) {
            carry = ptr_dst->value / 10;
            ptr_dst->value %= 10;
        } else
            carry = 0;

        /* Move to upper digit */
        ptr_dst = ptr_dst->prev;

        /* Add carry value and move to MSD */
        if (carry) {
            if (ptr_dst == NULL) {
                bn_msd_carry(&bn_dst, carry);

                // Re-assign ptr_dst to correct position
                ptr_dst = bn_dst->msd;
            } else {
                // Setup carry value
                ptr_dst->value = carry;
            }
        }

        /* Move source pointer */
        if (ptr_src1 != NULL)
            ptr_src1 = ptr_src1->prev;
        if (ptr_src2 != NULL)
            ptr_src2 = ptr_src2->prev;
    }

    /* Free original source */
    if (*dst != NULL) {
        bn_free(dst);
    }

    /* Pointer assignment for destination storage */
    *dst = bn_dst;

    return 0;
}

/* Subtract two big number - for Fibonacci number calculation usage only! */
int bn_sub_for_fib(bignum_t **dst, bignum_t *src_1, bignum_t *src_2)
{
    // In fast doubling method, src_1 is always bigger than src_2
    // For developing convenience, this subtraction must keep that src_1 is
    // always
    // bigger than src_2

    /* Variable declaration */
    bignum_t *bn_dst = NULL;
    bnlist_t *ptr_dst = NULL, /* *ptr_src1 = NULL,*/ *ptr_src2 = NULL;
    int retn;
    char borrow;

    /* Reject NULL source input */
    if (src_1 == NULL || src_2 == NULL)
        return -1;  // NULL source input

    /* Copy bignum from src1 to dst */
    retn = bn_copy(&bn_dst, src_1);

    if (retn != 0)
        return -2;  // Failed Allocation

    /* Assign initial pointer */
    ptr_dst = bn_dst->lsd;
    ptr_src2 = src_2->lsd;

    /* Run addition */
    borrow = 0;
    while (ptr_dst != NULL || ptr_src2 != NULL) {
        // Perform digit subtraction
        if (ptr_src2 != NULL)
            ptr_dst->value -= ptr_src2->value;

        // Borrow subtraction
        ptr_dst->value -= borrow;

        // Refresh borrow value for next calculation usage
        if (ptr_dst->value & 0x80) {  // borrow occured
            borrow = 1;

            // Return to original value
            ptr_dst->value += 10;
        } else
            borrow = 0;

        /* Move source and destination pointer */

        if (ptr_dst != NULL)
            ptr_dst = ptr_dst->prev;
        if (ptr_src2 != NULL)
            ptr_src2 = ptr_src2->prev;
    }

    // Clear leading zero
    bn_clean_leading_zero(&bn_dst);

    /* Free original source */
    if (*dst != NULL) {
        bn_free(dst);
    }

    /* Pointer assignment for destination storage */
    *dst = bn_dst;

    return 0;
}

/* Multiply two big number */
int bn_mul(bignum_t **dst, bignum_t *src_1, bignum_t *src_2)
{
    bignum_t *bn_dst;
    bnlist_t *ptr_dst, *ptr_dst_base, *ptr_src_big, *ptr_src_big_base,
        *ptr_src_small;
    char carry;

    if (src_1 == NULL || src_2 == NULL)
        return -1;

    /* Allocate new  bignum */
    bn_dst = bn_create();

    /* Compare digit count, and assign bigger or smaller number pointer */
    if (src_1->cnt_d >= src_2->cnt_d) {
        ptr_src_big_base = ptr_src_big = src_1->lsd;
        ptr_src_small = src_2->lsd;
    } else {
        ptr_src_big_base = ptr_src_big = src_2->lsd;
        ptr_src_small = src_1->lsd;
    }
    // Destination bignum pointer assignment
    ptr_dst_base = ptr_dst = bn_dst->lsd;


    /* Multiple and add number */
    while (ptr_src_small != NULL) {
        while (ptr_src_big != NULL) {
            // For dst mod 10 = 0 case, allocate new digit if needed
            if (ptr_dst == NULL) {
                if (bn_msd_carry(&bn_dst, 0) != 0) {
                    bn_free(&bn_dst);
                    return -2;
                }

                // move to MSD position
                ptr_dst = bn_dst->msd;
            }

            // Do digit value multiplication
            ptr_dst->value += ptr_src_big->value * ptr_src_small->value;

            // Extract carry value
            if (ptr_dst->value >= 10) {
                carry = ptr_dst->value / 10;
                ptr_dst->value %= 10;
            } else
                carry = 0;

            /* Move to upper digit */
            ptr_dst = ptr_dst->prev;

            // Carry set to next digit
            if (carry) {
                if (ptr_dst == NULL) {
                    if (bn_msd_carry(&bn_dst, carry) != 0) {
                        bn_free(&bn_dst);
                        return -2;
                    }

                    // Re-assign ptr_dst to correct position
                    ptr_dst = bn_dst->msd;
                } else
                    ptr_dst->value += carry;
            }

            // Move to next digit of big number
            ptr_src_big = ptr_src_big->prev;
        }

        // Destination add move forward
        ptr_dst_base = ptr_dst_base->prev;
        ptr_dst = ptr_dst_base;

        // Big number move back to LSD
        ptr_src_big = ptr_src_big_base;

        // Small number move to next digit
        ptr_src_small = ptr_src_small->prev;
    }

    if (*dst != NULL)
        bn_free(dst);

    *dst = bn_dst;

    return 0;
}


//----------------------------------------------------------------
// Sequence operation

/* Return fibonacci number, store with big number structure */
/*bignum_t *bn_fibonacci(long long n)
{
    // Declare dynamic array for big number
    bignum_t **bn_fib = NULL;
    bignum_t *ptr_retn = NULL;
    long long i;
    // unsigned long long temp;

    // Return NULL while input is not non-negative value
    if (n < 0)
        return NULL;

    // Allocate n bignum_t* pointer spaces for fibonacci number storage
    bn_fib = kcalloc(n + 1, sizeof(bignum_t *),
                     GFP_KERNEL);  // Allocate pointer spaces...

    // Return NULL while failed allocation
    if (bn_fib == NULL)
        return NULL;

    // Allocate bignum_t spaces for initial value F[0] and F[1]
    bn_fib[0] = bn_create();  // Allocate F[0]
    bn_fib[1] = bn_create();  // Allocate F[1]

    // Return NULL while failed allocation
    if (bn_fib[0] == NULL || bn_fib[1] == NULL) {
        if (bn_fib[0] != NULL)
            bn_free(&bn_fib[0]);
        if (bn_fib[1] != NULL)
            bn_free(&bn_fib[1]);
        kfree(bn_fib);
        return NULL;
    }

    // Fibonacci number initial value assignment
    bn_fib[0]->lsd->value = 0;  // F[0] = 0
    bn_fib[1]->lsd->value = 1;  // F[1] = 1

    // Calculate Fibonacci Number (Start from 2)
    for (i = 2; i <= n; i++) {
        // Failed during calculation
        if (bn_add(&bn_fib[i], bn_fib[i - 1], bn_fib[i - 2]) != 0) {
            // Free allocated space and return
            bn_free(&bn_fib[i - 2]);
            bn_free(&bn_fib[i - 1]);
            bn_free(&bn_fib[i]);
            kfree(bn_fib);

            return NULL;
        }

        bn_free(&bn_fib[i - 2]);

        if (i == n)
            ptr_retn = bn_fib[i];  // Return nth sequence
    }

    if (n == 0) {
        ptr_retn = bn_fib[0];
        bn_free(&bn_fib[1]);
    } else if (n == 1) {
        ptr_retn = bn_fib[1];
        bn_free(&bn_fib[0]);
    } else
        bn_free(&bn_fib[n - 1]);

    kfree(bn_fib);

    return ptr_retn;
}*/

/* Return fibonacci number via fast doubling method */
bignum_t *bn_fibonacci_fd(long long n)
{
    bignum_t *t0 = NULL, *t1 = NULL;  // For F[n], F[n+1]
    bignum_t *t3 = NULL, *t4 = NULL;  // For F[2n], F[2n+1]
    bignum_t *temp1 = NULL, *temp2 = NULL;

    int i = 1, retn = 0;

    /* Initial value assignment*/
    retn = bn_cast_from_ll(&t0, 1);
    if (retn != 0)
        goto bn_fib_fd_FAIL;

    retn = bn_cast_from_ll(&t1, 1);
    if (retn != 0)
        goto bn_fib_fd_FAIL;

    retn = bn_cast_from_ll(&t3, 1);
    if (retn != 0)
        goto bn_fib_fd_FAIL;

    retn = bn_cast_from_ll(&t4, 0);
    if (retn != 0)
        goto bn_fib_fd_FAIL;

    /* Response F[0] */
    if (n <= 0) {
        bn_free(&t0);
        bn_free(&t1);
        bn_free(&t3);
        return t4;
    }
    /* Response F[1] */
    else if (n == 1) {
        bn_free(&t1);
        bn_free(&t3);
        bn_free(&t4);
        return t0;
    }


    while (i < n) {
        if ((i << 1) <= n) {
            /* Task:
                   1. t4 = t1 * t1 + t0 * t0
                   2. t3 = t0 * (2 * t1 - t0)
                   3. t0 = t3
                   4. t1 = t4
            */

            /* Perform t4 = t1 * t1 + t0 * t0 */
            retn = bn_mul(&temp1, t1, t1);
            if (retn != 0)
                goto bn_fib_fd_FAIL;

            retn = bn_mul(&temp2, t0, t0);
            if (retn != 0)
                goto bn_fib_fd_FAIL;

            retn = bn_add(&t4, temp1, temp2);
            if (retn != 0)
                goto bn_fib_fd_FAIL;

            /* Perform t3 = t0 * (2 * t1 - t0)*/
            retn = bn_cast_from_ll(&temp1, 2);
            if (retn != 0)
                goto bn_fib_fd_FAIL;

            retn = bn_mul(&temp1, temp1, t1);  // 2*t1
            if (retn != 0)
                goto bn_fib_fd_FAIL;

            retn = bn_sub_for_fib(&temp2, temp1, t0);  // 2*t1 - t0
            if (retn != 0)
                goto bn_fib_fd_FAIL;

            retn = bn_mul(&t3, t0, temp2);
            if (retn != 0)
                goto bn_fib_fd_FAIL;

            // Perform t0 = t3 and t1 = t4
            bn_copy(&t0, t3);
            bn_copy(&t1, t4);

            i = i << 1;
        } else {
            /* Task:
                   1. t0 = t3
                   2. t3 = t4
                   3. t4 = t0 + t4
            */

            retn = bn_copy(&t0, t3);
            if (retn != 0)
                goto bn_fib_fd_FAIL;

            retn = bn_copy(&t3, t4);
            if (retn != 0)
                goto bn_fib_fd_FAIL;

            retn = bn_add(&t4, t0, t4);
            if (retn != 0)
                goto bn_fib_fd_FAIL;

            i++;
        }
    }

    return t3;

bn_fib_fd_FAIL:

    /* TODO: improve mechanism for alerting failed allocation in memory space */

    bn_free(&t0);
    bn_free(&t1);
    bn_free(&t3);
    bn_free(&t4);

    return NULL;
}


//----------------------------------------------------------------
// Big number service operation

/* Print big number in Big-endian(MSD first) */
#ifndef KSPACE
void bn_print(bignum_t *bnum)
{
    /* Local Variable Declaration */
    bnlist_t *ptr;

    /* Return while bnum is NULL */
    if (bnum == NULL)
        return;

    /* Pointer Assignment, print by big-endian(MSD first)*/
    ptr = bnum->msd;

    /* Traverse each node and print */
    while (ptr != NULL) {
        printf("%c", ptr->value + 0x30);  // Add 0x30 for ASCII encoding

        ptr = ptr->next;
    }
}
#endif

/* Transfer big number to string and free */
char *bn_tostring(bignum_t **bnum)
{
    char *str;
    bnlist_t *ptr;
    int i;

    if (*bnum == NULL)
        return NULL;

    str = (char *) kcalloc((*bnum)->cnt_d + 1, sizeof(char), GFP_KERNEL);

    if (str == NULL)
        return NULL;

    ptr = (*bnum)->msd;

    for (i = 0; i < (*bnum)->cnt_d; i++) {
        str[i] = ptr->value + 0x30;
        ptr = ptr->next;
    }

    return str;
}

// Copy big number from source to destination
int bn_copy(bignum_t **dst, bignum_t *src)
{
    // Local variable declaration
    bignum_t *bn_dst = NULL;
    bnlist_t *ptr = NULL;
    int retn;

    bn_dst = bn_create();

    if (bn_dst == NULL)
        return -2;  // Failed allocation

    ptr = src->lsd;
    bn_dst->lsd->value = ptr->value;

    while (ptr->prev != NULL) {
        ptr = ptr->prev;
        retn = bn_msd_carry(&bn_dst, ptr->value);

        if (retn != 0)
            goto bncpy_FAA;
    }

    if (*dst != NULL)
        bn_free(dst);

    *dst = bn_dst;

    return 0;

bncpy_FAA:  // Failed after allocation

    // Free all allocated space
    bn_free(&bn_dst);
    return -2;
}

/* Cast long long int to big number */
int bn_cast_from_ll(bignum_t **dst, long long input)
{
    bignum_t *bn_dst;

    if (input < 0)
        input = 0;

    bn_dst = bn_create();

    if (bn_dst == NULL)
        return -2;

    while (1) {
        bn_dst->msd->value = (input % 10);
        input /= 10;

        if (input != 0) {
            if (bn_msd_carry(&bn_dst, 0) != 0) {
                bn_free(&bn_dst);
                return -2;
            }
        } else
            break;
    }

    if (*dst != NULL)
        bn_free(dst);

    *dst = bn_dst;

    return 0;
}
