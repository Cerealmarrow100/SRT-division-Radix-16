#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>












#define RADIX 16


void print_array(int8_t* array, int length) {
    printf("[");
    for (int i = 0; i<length; i++) {
        printf("%d, ", array[i]);
    }
    printf("\b]\n");
}




void print_array2(int64_t* array, int length) {
    printf("[");
    for (int i = 0; i<length; i++) {
        printf("%lld, ", array[i]);
    }
    printf("\b]\n");
}




void print128(__uint128_t x) {
    uint64_t h = x>>64;
    uint64_t l = x&((uint64_t)-1);
    printf("%"PRIx64 " %"PRIx64, h,l);
}




typedef struct DivisionResult {
    uint64_t quotient;
    uint32_t remainder;
} DivisionResult;




int8_t SelectQuotientDigit16(int64_t partial_remainder, uint32_t divisor) {
    int64_t R = partial_remainder;
    if (R < 0) R++;
    int64_t D = divisor;


    if ((2*R) >= (15*D))
        return 8;
    else if ((2*R) > (13*D))
        return 7;
    else if ((2*R) > (11*D))
        return 6;
    else if ((2*R) > (9*D))
        return 5;
    else if ((2*R) > (7*D))
        return 4;
    else if ((2*R) > (5*D))
        return 3;
    else if ((2*R) > (3*D))
        return 2;
    else if ((2*R) > D)
        return 1;
    else if ((2*R) > (-D))
        return 0;
    else if ((2*R) > (-3*D))
        return -1;
    else if ((2*R) > (-5*D))
        return -2;
    else if ((2*R) > (-7*D))
        return -3;
    else if ((2*R) > (-9*D))
        return -4;
    else if ((2*R) > (-11*D))
        return -5;
    else if ((2*R) > (-13*D))
        return -6;
    else if ((2*R) > (-15*D))
        return -7;
    else
        return -8;
}




int MSBpos32(uint32_t x) {
    int i = 0;
    while ((x & 0x80000000) == 0) {
        i++;
        x <<= 1;
    }
    return i;
}




int MSBpos64(int64_t x) {
    int i = 0;
    int is_negative = x < 0;
    if (is_negative) x = -x;
    while ((x & 0x8000000000000000) == 0) {
        i++;
        x <<= 1;
    }
    return i;
}




int32_t remainder_tweak_sign(int32_t unfinished_remainder, int32_t divisor) {
    return (unfinished_remainder >= divisor)? -1 : 1;
}




DivisionResult uSRT16(uint64_t dividend, uint32_t divisor) {
    if (divisor == 0) {
        return (DivisionResult){(uint32_t)(-1),0};
    }


    DivisionResult result = {0,0};
    int8_t quotient_digits[16];




    int scale = MSBpos32(divisor);
    uint32_t normalised_divisor = divisor << scale;
    __int128_t partial_remainder = dividend;
    partial_remainder <<= (scale)&3;
    int64_t remainder;
    int remainder_cycle;
    int saved_remainder = 0;
    for (int i = 0; i < 16; i++) {
        int8_t quotient_digit = SelectQuotientDigit16((partial_remainder>>48), normalised_divisor>>16);


        quotient_digits[i] = quotient_digit;
        partial_remainder -= ((__int128_t)quotient_digit)*(((__uint128_t)normalised_divisor)<<32);
        if ((((15-i)<<2) < (32-scale)) && !saved_remainder) {
            remainder = partial_remainder>>((i)<<2);
            saved_remainder = 1;
            remainder_cycle = i;
        }
        partial_remainder <<= 4;
    }

    for (int i = 0; i <= remainder_cycle; i++) {
        result.quotient += (int64_t)quotient_digits[i] << ((15-i) << 2);
    }
    result.quotient >>= (15-remainder_cycle)<<2;
    int64_t final_remainder = remainder >> ((4-(((32-scale)&3)))>>((((32-scale)&3) == 0)<<3));


    int8_t potential_digits[17] = {
        //-15,-14,-13,-12,-11,-10,-9,
        -8,-7,-6,-5,-4,-3,-2,-1,
        0,1,2,3,4,5,6,7,8
        //,9,10,11,12,13,14,15
        };
    int8_t final_quotient_digit = (remainder > 0)? 8: -8;




    int64_t d = divisor;
    for (int i = 0; i < 17; i++) {
        int x = potential_digits[i];
        int64_t test_remainder = (int64_t)final_remainder - ((int64_t)potential_digits[i]*(uint64_t)divisor);
        int cond1 = test_remainder >= 0;
        int cond2 = test_remainder < (int64_t)divisor;
        if (cond1 && cond2) {
            final_quotient_digit = potential_digits[i];
            break;
        }
    }
    result.quotient += final_quotient_digit;
    result.remainder = final_remainder - final_quotient_digit*divisor;
   
    return result;
}




void divide(int64_t dividend, int32_t divisor) {
    uint8_t sign_dividend = ((uint64_t)dividend) >> 63;
    uint8_t sign_divisor = ((uint64_t)divisor) >> 63;
    uint8_t sign_quotient = ((sign_dividend&(~sign_divisor))|((~sign_dividend)&sign_divisor)) & 1;
    DivisionResult unsigned_result;
    if (RADIX == 16)
        unsigned_result = uSRT16(dividend - sign_dividend*(dividend << 1), divisor - sign_divisor*(divisor << 1));
    unsigned_result.quotient -= sign_quotient*(unsigned_result.quotient << 1);
    unsigned_result.remainder -= sign_dividend*(unsigned_result.remainder << 1);
    printf("Quotient: %" PRIi32 "\n", unsigned_result.quotient);
    printf("Remainder: %" PRIi32 "\n", unsigned_result.remainder);
}




int main(void) {
    srand(time(NULL));
    uint64_t dividend_input;
    uint32_t divisor_input;


    int valid = 1;
    for (uint64_t i = 1; (i < UINT64_MAX) && (i != 0); i+=(rand()&0xFFFFFFFF)) { //This runs through every fucking possible number.
            dividend_input = i;
            divisor_input = rand()|1;


            DivisionResult result = uSRT16(dividend_input, divisor_input);
            if ((result.quotient - (dividend_input/divisor_input) != 0) || (result.remainder - (dividend_input%divisor_input) != 0)) {
                valid = 0;
                printf("%llu / %lu = %lu r %llu\n", dividend_input, divisor_input, result.quotient, result.remainder);
                printf("%llu / %lu = %lu r %llu\n", dividend_input, divisor_input, dividend_input/divisor_input, dividend_input%divisor_input);
                break;
            }
        printf("%"PRIu64" Valid\n",i);
    }
    if (valid) printf("Good Job!\n");




    return 0;
}


