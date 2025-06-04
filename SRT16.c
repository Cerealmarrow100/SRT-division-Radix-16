#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RADIX 16 //This algorithm divides 4 bits (1 nibble or hex digit) at a time

void print_array(int8_t* array, int length) { //Utility debug function for printing a signed byte array
    printf("[");
    for (int i = 0; i<length; i++) {
        printf("%d, ", array[i]);
    }
    printf("\b]\n");
}

void print_array2(int64_t* array, int length) { //Utility debug function for printing a signed int64 array
    printf("[");
    for (int i = 0; i<length; i++) {
        printf("%lld, ", array[i]);
    }
    printf("\b]\n");
}

void print128(__uint128_t x) { //Utility debug function for printing a 128-bit number as two 64-bit hexadecimal integers 
    uint64_t h = x>>64;
    uint64_t l = x&((uint64_t)-1);
    printf("%"PRIx64 " %"PRIx64, h,l);
}



typedef struct DivisionResult { //Struct for holding division results
    uint64_t quotient;
    uint32_t remainder;
} DivisionResult;

/*
Select Quotient Digit 16
This function takes in the truncated partial remainder and truncated divisor (both normalised)
and guesses the best quotient digit from the two in radix-16.
The bounds are donw to round the resulting quotient to the nearest integer. So if A/B = 0.5, round to 1
The redundant digit set (-8,...,0,...8) will correct in the worst case scenario that the following bits are 0
The top 7 partial remainder bits and top 6 normalised divisor bits are examined for this to perform a quick
comparison rather than a full comparison and while keeping the accuracy such that we never fall out of the
correct range for convergence and recovery
*/
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

int MSBpos32(uint32_t x) { //Utility function for counting the position of the first non-zero bit because I didn't knoe the clz intrinsic
    int i = 0;
    while ((x & 0x80000000) == 0) {
        i++;
        x <<= 1;
    }
    return i;
}

int MSBpos64(int64_t x) { //Same as before but 64-bit
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
    //SRT preconsitions and declarations
    if (divisor == 0) {
        return (DivisionResult){(uint32_t)(-1),0};
    }

    DivisionResult result = {0,0};
    int8_t quotient_digits[9];

    /*
    This is the normalisation step of the division so the leading bit of the
    divisor is a 1 so the algorithm is correct and all quotient digits lie in range
    -8 to 8
    r*scale/d*scale = R/D = r/d so no difference except remainder
    */
    int scale = MSBpos32(divisor);
    uint32_t normalised_divisor = divisor << scale;
    __int128_t partial_remainder = dividend;
    partial_remainder <<= scale;
    
    /*
    Main division loop
    Digit recurrence: R_i+1 = 16(R_i - q_i*D)
    */
    for (int i = 0; i < 9; i++) {
        int8_t quotient_digit = SelectQuotientDigit16(partial_remainder>>48, normalised_divisor>>16);
        quotient_digits[i] = quotient_digit;
        //SIgn extend to 128-bits to not lose bits from shift truncation
        partial_remainder -= ((__int128_t)quotient_digit)*(((__uint128_t)normalised_divisor)<<32);
        partial_remainder <<= 4;
    }

    //Convert quotient into non-redundant form
    uint64_t final_quotient = 0;
    for (int i = 0; i < 9; i++) {
        final_quotient += (int64_t)quotient_digits[i] << ((8-i) << 2);
    }

    //Constant factor of 32 as the partial remainder had 32 extra bits originally + 4 for the final unneeded shift + scale to undo normalisation factor
    //Remember that n = q*d + r so when n is scaled, r is also scaled so this must also be undone 
    int64_t final_remainder = partial_remainder >> (32 + 4 + scale);

    /*
    Final full restoration if the final remainder is negative.
    Remainder is always in range -d < r < d so add d is r < 0
    This means 1 less unit can fit within the dividend so decrement quotient
    */
    if (final_remainder < 0) {
        final_remainder += divisor;
        final_quotient--;
    }

    //Return data in result struct
    result.quotient = final_quotient;
    result.remainder = final_remainder;
   
    return result;
}

//Signed integer division function
void divide(int64_t dividend, int32_t divisor) {
    uint8_t sign_dividend = ((uint64_t)dividend) >> 63; //MSB is the sign bit in 2's complement
    uint8_t sign_divisor = ((uint64_t)divisor) >> 63; //Look up gng ^
    uint8_t sign_quotient = (sign_dividend^sign_divisor) & 1; //XOR sign bits for quotient sign cuz truth table gng
    DivisionResult unsigned_result; //self explanatory
    if (RADIX == 16) //Originally for multiple radices but I cba as 16 is the most complex practically used radix
        //Divide the absolute values of the division inputs.
        // If the sign is 0 (+ve) then x-0 = x (+ve). If the sign is 1 (-ve) then x-2x = -x (+ve)
        unsigned_result = uSRT16(dividend - sign_dividend*(dividend << 1), divisor - sign_divisor*(divisor << 1));
    unsigned_result.quotient -= sign_quotient*(unsigned_result.quotient << 1); //Same as before but reverses and signs the quotient
    unsigned_result.remainder -= sign_dividend*(unsigned_result.remainder << 1); //Look up gng ^ and the remainder takes the sign of the dividend
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

    /*
    SRT test: unsigned divide with our algorithm, then use actual division and modulo and compare to test correctness
    TODO: more testing so the divide function can be used instead.
    TODO: for some reason the inputs only work in that order. Reversing the printf scanf order makes the dividend input zero?
    */
    DivisionResult result = uSRT16(dividend_input, divisor_input);
    printf("%llu / %lu = %lu r %llu\n", dividend_input, divisor_input, result.quotient, result.remainder);
    printf("%llu / %lu = %lu r %llu\n", dividend_input, divisor_input, dividend_input/divisor_input, dividend_input%divisor_input);

    return 0;
}