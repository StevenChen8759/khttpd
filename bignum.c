#include <linux/slab.h>

#include "bignum.h"

bignum_t *bn_create(void)
{
    /* Local variable declaration */
    bignum_t *bn_new;

    /* Allocate big number`s memory space */
    bn_new = (bignum_t *) kcalloc(1, sizeof(bignum_t), GFP_KERNEL);

    /* Check if allocated space is null */
    if (bn_new == NULL)
        return bn_new;

    /* Allocate space for first number (as list) */
    bn_new->lsd = kcalloc(1, sizeof(bnlist_t), GFP_KERNEL);

    /* Allocate node fail, free allocated space and return NULL */
    if (bn_new->lsd == NULL) {
        kfree(bn_new);
        return NULL;
    }

    /* Assign cnt_d value */
    bn_new->cnt_d = 1;

    /* Pointer operation for big number */
    bn_new->msd = bn_new->lsd;

    /* Assign initial value for allocated number(Not necessary for calloc()) */

    /* Result return */
    return bn_new;
}

bignum_t *bn_fibonacci(long long n)
{
    /* Declare dynamic array for big number */
    bignum_t **bn_fib;
    bignum_t *ptr_retn;
    long long i;
    // unsigned long long temp;

    /* Return NULL while input is not non-negative value */
    if (n == 0)
        return NULL;

    /* Allocate n bignum_t* pointer spaces for fibonacci number storage */
    bn_fib = kcalloc(n + 1, sizeof(bignum_t *),
                     GFP_KERNEL);  // Allocate pointer spaces...

    /* Return NULL while failed allocation */
    if (bn_fib == NULL)
        return NULL;

    /* Allocate bignum_t spaces for initial value F[0] and F[1] */
    bn_fib[0] = bn_create();  // Allocate F[0]
    bn_fib[1] = bn_create();  // Allocate F[1]

    /* Return NULL while failed allocation*/
    if (bn_fib[0] == NULL || bn_fib[1] == NULL) {
        if (bn_fib[0] != NULL)
            bn_free(bn_fib[0]);
        if (bn_fib[1] != NULL)
            bn_free(bn_fib[1]);
        kfree(bn_fib);
        return NULL;
    }

    /* Fibonacci number initial value assignment */
    ((bn_fib[0])->lsd)->value = 0;  // F[0] = 0
    ((bn_fib[1])->lsd)->value = 1;  // F[1] = 1

    // printf("Reading from /dev/fibonacci at offset 0, returned the sequence
    // ");
    // bn_print(bn_fib[0]);
    // printf(".\n");
    // printf("Reading from /dev/fibonacci at offset 1, returned the sequence
    // ");
    // bn_print(bn_fib[1]);
    // printf(".\n");

    /* Calculate Fibonacci Number (Start from 2)  */
    for (i = 2; i <= n; i++) {
        bn_fib[i] = bn_add_with_malloc(bn_fib[i - 1], bn_fib[i - 2]);

        if (i == n)
            ptr_retn = bn_fib[i];  // Return nth sequence
    }

    for (i = n - 1;; i--) {
        bn_free(bn_fib[i]);
        if (i == 0)
            break;
    }

    kfree(bn_fib);

    return ptr_retn;
}

bignum_t *bn_add_with_malloc(bignum_t *src_1, bignum_t *src_2)
{
    /* Variable declaration */
    bignum_t *des;
    bnlist_t *ptr_des, *ptr_src1, *ptr_src2;
    int add_max, i;
    char carry;

    if (src_1 == NULL || src_2 == NULL)
        return NULL;

    des = bn_create();

    if (des == NULL)
        return NULL;

    /* Assign max run of digit added */
    if (src_1->cnt_d >= src_2->cnt_d) {
        bn_set_digit(des, src_1->cnt_d);
        add_max = src_1->cnt_d;
    } else {
        add_max = src_2->cnt_d;
    }

    /* Allocate enough digits for addition result */
    bn_set_digit(des, add_max);

    /* Assign initial pointer */
    ptr_des = des->lsd;
    ptr_src1 = src_1->lsd;
    ptr_src2 = src_2->lsd;

    /* Run addition */
    carry = 0;
    for (i = 0; i < add_max; i++) {
        /* Add number by different cases */
        // printf("Initial des: %d, carry: %d, ", ptr_des->value, carry);

        if (ptr_src1 != NULL) {
            ptr_des->value += ptr_src1->value;
        }

        if (ptr_src2 != NULL) {
            ptr_des->value += ptr_src2->value;
        }

        ptr_des->value += carry;

        /* Do digit carry */
        if (ptr_des->value >= 10) {
            ptr_des->value -= 10;
            carry = 1;
        } else {
            carry = 0;
        }

        /* Move pointer */
        ptr_des = ptr_des->prev;
        if (ptr_src1 != NULL)
            ptr_src1 = ptr_src1->prev;
        if (ptr_src2 != NULL)
            ptr_src2 = ptr_src2->prev;
    }

    /* Final MSD carry */
    if (carry != 0) {
        /* Allocate carried MSD */
        ptr_des = kcalloc(1, sizeof(bnlist_t), GFP_KERNEL);

        /* Pointer and value assignment*/
        ptr_des->next = des->msd;
        ptr_des->value = 1;
        des->msd->prev = ptr_des;
        des->msd = ptr_des;
        des->cnt_d += 1;
    }

    return des;
}

void bn_set_digit(bignum_t *bnum, int digit)
{
    /* TODO: Consider allocation fail case! */

    bnlist_t *ptr_bnl;

    if (bnum == NULL)
        return;

    while (bnum->cnt_d < digit) {
        ptr_bnl = kcalloc(1, sizeof(bnlist_t), GFP_KERNEL);

        /* Edit pointer */
        ptr_bnl->next = bnum->msd;
        bnum->msd->prev = ptr_bnl;

        /* Move MSD pointer */
        bnum->msd = bnum->msd->prev;
        bnum->cnt_d++;
    }
}

char *bn_tostring_and_free(bignum_t **bnum)
{
    int i;
    char *str;
    bnlist_t *ptr;

    if (*bnum == NULL)
        return NULL;

    str = (char *) kcalloc((*bnum)->cnt_d + 1, sizeof(char), GFP_KERNEL);
    ptr = (*bnum)->msd;

    for (i = 0; i < (*bnum)->cnt_d; i++) {
        str[i] = ptr->value + 0x30;
        ptr = ptr->next;
    }

    bn_free(*bnum);
    *bnum = NULL;

    return str;
}

void bn_free(bignum_t *bnum)
{
    /* Local variable declaration */
    bnlist_t *ptr;

    /* Return while bnum is NULL */
    if (bnum == NULL)
        return;

    /* Pointer assignment*/
    ptr = bnum->msd;

    /* free the list */
    while (ptr != NULL) {
        bnum->msd = bnum->msd->next;
        kfree(ptr);
        ptr = bnum->msd;
    }

    /* free the number structure */
    kfree(bnum);
}


/*
int main(int argc, char *argv[])
{
    bn_free(bn_fibonacci(100));

    return 0;
}
*/
