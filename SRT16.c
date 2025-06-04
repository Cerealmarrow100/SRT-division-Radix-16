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
    int64_t R = partial_remainder&(-2); //Top 7 bits
    if (R < 0) R++;
    int64_t D = divisor&(-4); //Top 6 bits

    if ((2*R) >= (15*D))
        return 8;
    else if ((2*R) >= (13*D))
        return 7;
    else if ((2*R) >= (11*D))
        return 6;
    else if ((2*R) >= (9*D))
        return 5;
    else if ((2*R) >= (7*D))
        return 4;
    else if ((2*R) >= (5*D))
        return 3;
    else if ((2*R) >= (3*D))
        return 2;
    else if ((2*R) >= D)
        return 1;
    else if ((2*R) >= (-D))
        return 0;
    else if ((2*R) >= (-3*D))
        return -1;
    else if ((2*R) >= (-5*D))
        return -2;
    else if ((2*R) >= (-7*D))
        return -3;
    else if ((2*R) >= (-9*D))
        return -4;
    else if ((2*R) >= (-11*D))
        return -5;
    else if ((2*R) >= (-13*D))
        return -6;
    else if ((2*R) >= (-15*D))
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




DivisionResult uSRT16(uint64_t dividend, uint32_t divisor) {
    if (divisor == 0) {
        return (DivisionResult){(uint32_t)(-1),0};
    }

    DivisionResult result = {0,0};
    int8_t quotient_digits[9];

    int scale = MSBpos32(divisor);
    uint32_t normalised_divisor = divisor << scale;
    __int128_t partial_remainder = dividend;
    partial_remainder <<= scale;
    
    int saved_remainder = 0;
    for (int i = 0; i < 9; i++) {
        int8_t quotient_digit = SelectQuotientDigit16(partial_remainder>>48, normalised_divisor>>16);
        quotient_digits[i] = quotient_digit;
        partial_remainder -= ((__int128_t)quotient_digit)*(((__uint128_t)normalised_divisor)<<32);
        partial_remainder <<= 4;
    }
    
    uint64_t final_quotient = 0;
    for (int i = 0; i < 9; i++) {
        final_quotient += (int64_t)quotient_digits[i] << ((8-i) << 2);
    }


    int64_t final_remainder = partial_remainder >> (32 + 4 + scale);

    if (final_remainder < 0) {
        final_remainder += divisor;
        final_quotient--;
    }


    result.quotient = final_quotient;
    result.remainder = final_remainder;
   
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




    printf("Enter unsigned divisor: ");
    scanf("%lu",&divisor_input);
    printf("Enter unsigned dividend: ");
    scanf("%llu",&dividend_input);




    DivisionResult result = uSRT16(dividend_input, divisor_input);
    printf("%llu / %lu = %lu r %llu\n", dividend_input, divisor_input, result.quotient, result.remainder);
    printf("%llu / %lu = %lu r %llu\n", dividend_input, divisor_input, dividend_input/divisor_input, dividend_input%divisor_input);




    return 0;
}
