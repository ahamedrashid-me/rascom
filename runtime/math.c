// RasCode Math & Bit Operations Library
// Lean systems-focused math without heavy floating point

#include <math.h>

// Integer square root (fast integer-only version)
// Usage: @isqrt[x] -> sqrt(x) as integer
long sc_isqrt(long x) {
    if (x < 0) return -1;
    if (x == 0) return 0;
    if (x == 1) return 1;
    
    long guess = x;
    long next = (guess + x / guess) / 2;
    
    while (next < guess) {
        guess = next;
        next = (guess + x / guess) / 2;
    }
    
    return guess;
}

// Power function (integer exponentiation)
// Usage: @pow[base, exp] -> base^exp
long sc_pow(long base, long exp) {
    if (exp < 0) return 0;
    if (exp == 0) return 1;
    
    long result = 1;
    long b = base;
    
    while (exp > 0) {
        if (exp & 1) result *= b;
        b *= b;
        exp >>= 1;
    }
    
    return result;
}

// Absolute value
// Usage: @abs[x] -> |x|
long sc_abs(long x) {
    return x < 0 ? -x : x;
}

// Minimum of two numbers
// Usage: @min[a, b] -> min(a, b)
long sc_min(long a, long b) {
    return a < b ? a : b;
}

// Maximum of two numbers
// Usage: @max[a, b] -> max(a, b)
long sc_max(long a, long b) {
    return a > b ? a : b;
}

// Count leading zeros (bit manipulation)
// Usage: @clz[x] -> number of leading zero bits
long sc_clz(long x) {
    if (x == 0) return 64;
    
    long count = 0;
    long bit = 1LL << 63;
    
    while (!(x & bit)) {
        count++;
        bit >>= 1;
    }
    
    return count;
}

// Count trailing zeros
// Usage: @ctz[x] -> number of trailing zero bits
long sc_ctz(long x) {
    if (x == 0) return 64;
    
    long count = 0;
    while (!(x & 1)) {
        count++;
        x >>= 1;
    }
    
    return count;
}

// Population count (count set bits)
// Usage: @popcount[x] -> number of 1 bits
long sc_popcount(long x) {
    long count = 0;
    while (x) {
        count += x & 1;
        x >>= 1;
    }
    return count;
}

// Greatest common divisor (Euclidean algorithm)
// Usage: @gcd[a, b] -> gcd(a, b)
long sc_gcd(long a, long b) {
    a = sc_abs(a);
    b = sc_abs(b);
    
    while (b) {
        long temp = b;
        b = a % b;
        a = temp;
    }
    
    return a;
}

// Least common multiple
// Usage: @lcm[a, b] -> lcm(a, b)
long sc_lcm(long a, long b) {
    if (a == 0 || b == 0) return 0;
    return sc_abs(a * b) / sc_gcd(a, b);
}

// Check if number is prime
// Usage: @isprime[n] -> 1 if prime, 0 if not
long sc_isprime(long n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    
    long i = 3;
    long limit = sc_isqrt(n);
    
    while (i <= limit) {
        if (n % i == 0) return 0;
        i += 2;
    }
    
    return 1;
}

// Modular exponentiation (for cryptography)
// Usage: @modpow[base, exp, mod] -> (base^exp) % mod
long sc_modpow(long base, long exp, long mod) {
    if (mod == 1) return 0;
    
    long result = 1;
    base = base % mod;
    
    while (exp > 0) {
        if (exp & 1) result = (result * base) % mod;
        exp >>= 1;
        base = (base * base) % mod;
    }
    
    return result;
}

#include <math.h>

/**
 * @sqrt[x] - Floating-point square root
 * x: input value (as deci/float in RAX)
 * Returns: square root as deci/float
 */
long sc_sqrt(long x) {
    // Interpret RAX bits as double
    double val = *(double *)&x;
    double result = sqrt(val);
    // Return result as bits in RAX
    return *(long *)&result;
}

/**
 * @floor[x] - Floor function
 * x: input value (as deci/float)
 * Returns: floor value
 */
long sc_floor(long x) {
    double val = *(double *)&x;
    double result = floor(val);
    return *(long *)&result;
}

/**
 * @ceil[x] - Ceiling function
 * x: input value (as deci/float)
 * Returns: ceiling value
 */
long sc_ceil(long x) {
    double val = *(double *)&x;
    double result = ceil(val);
    return *(long *)&result;
}
