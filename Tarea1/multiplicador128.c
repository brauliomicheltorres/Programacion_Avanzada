/*
 * =====================================================================================
 * Asignatura : Programacion Avanzada
 * Archivo    : multiplicador128.c
 * Descripcion: Multiplicador de dos numeros enteros de 128 bits utilizando:
 *              1. Uniones en C (para acceder a diferentes granularidades de bits).
 *              2. Biblioteca de Intel Intrinsics (_mm_mul_epu32 de SSE2/AVX).
 *
 * Explicacion teorica:
 * --------------------
 * Un numero entero sin signo de 128 bits se puede descomponer en dos partes de 64 bits:
 *      A = A1 * 2^64 + A0
 *      B = B1 * 2^64 + B0
 *
 * El producto completo A * B puede tener hasta 256 bits y se expande como:
 *      A * B = (A0 * B0) + (A0 * B1 + A1 * B0) * 2^64 + (A1 * B1) * 2^128
 *
 * Cada multiplicacion de dos terminos de 64 bits (Xi * Yj) produce un resultado
 * de hasta 128 bits. Para calcular Xi * Yj mediante Intel Intrinsics, dividimos
 * cada termino de 64 bits en dos mitades de 32 bits:
 *      X = X1 * 2^32 + X0
 *      Y = Y1 * 2^32 + Y0
 *
 * Usamos la instruccion intrinseca `_mm_mul_epu32`:
 * Esta instruccion toma dos enteros sin signo de 32 bits y calcula su producto
 * exacto de 64 bits en paralelo en un registro __m128i.
 *
 * Compilacion:
 *      gcc -Wall -Wextra -O2 -msse2 multiplicador128.c -o multiplicador128
 * Ejecucion:
 *      ./multiplicador128
 * =====================================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <emmintrin.h> // SSE2 intrinsics (_mm_mul_epu32, etc.)
#include <immintrin.h> // AVX / AVX2 intrinsics

/* -------------------------------------------------------------------------
 * 
 * ------------------------------------------------------------------------- */
union MSGunion {
    unsigned char MSG[256];
    unsigned int MSGi[64];
    unsigned long long MSGll[32];
    __m128i MSG128[16];
    __m256i MSG256[8];
};

/* -------------------------------------------------------------------------
 * 2. Uniones tipadas para 128 bits y 256 bits
 *    Permiten acceder simultaneamente a:
 *    - El registro intrinsic (__m128i o __m256i)
 *    - Palabras de 64 bits (unsigned long long)
 *    - Palabras de 32 bits (unsigned int)
 *    - Bytes individuales (unsigned char)
 * ------------------------------------------------------------------------- */
typedef union {
    __m128i v;                   /* Registro SSE/AVX de 128 bits           */
    unsigned long long q[2];     /* 2 palabras de 64 bits (q[0]: bajo, q[1]: alto) */
    unsigned int d[4];           /* 4 palabras de 32 bits                  */
    unsigned char b[16];         /* 16 bytes individuales                  */
} uint128_u;

typedef union {
    __m256i v;                   /* Registro AVX de 256 bits               */
    __m128i v128[2];             /* 2 registros de 128 bits (bajo y alto)  */
    unsigned long long q[4];     /* 4 palabras de 64 bits                  */
    unsigned int d[8];           /* 8 palabras de 32 bits                  */
    unsigned char b[32];         /* 32 bytes individuales                  */
} uint256_u;

/* -------------------------------------------------------------------------
 
 * ------------------------------------------------------------------------- */
void print_m128i(__m128i *v) {
    unsigned long long *k = (unsigned long long *)v;
    /* Imprime parte alta (k[1]) seguida de parte baja (k[0]) en hexadecimal */
    printf("%016llx%016llx", k[1], k[0]);
}

void print_m256i(__m256i *v) {
    unsigned long long *k = (unsigned long long *)v;
    /* Imprime de mayor a menor significancia: k[3], k[2], k[1], k[0] */
    printf("%016llx%016llx%016llx%016llx", k[3], k[2], k[1], k[0]);
}

/* -------------------------------------------------------------------------
 * 4. Multiplicacion de dos enteros de 64 bits -> 128 bits con Intel Intrinsics
 *    Calcula X * Y usando `_mm_mul_epu32`.
 *
 *    Sean X = (X1 << 32) + X0  y  Y = (Y1 << 32) + Y0.
 *    X * Y = X0*Y0 + (X0*Y1 + X1*Y0)*2^32 + X1*Y1*2^64.
 * ------------------------------------------------------------------------- */
static inline uint128_u mul64_intrinsics(unsigned long long X, unsigned long long Y) {
    uint128_u vX, vY, vY_swap;
    uint128_u P_diag, P_cross;
    uint128_u resultado;

    /* Extraemos las mitades de 32 bits utilizando campos de la union */
    unsigned int x0 = (unsigned int)(X & 0xFFFFFFFFULL);
    unsigned int x1 = (unsigned int)(X >> 32);
    unsigned int y0 = (unsigned int)(Y & 0xFFFFFFFFULL);
    unsigned int y1 = (unsigned int)(Y >> 32);

    /*
     * Configuramos los registros para _mm_mul_epu32:
     * _mm_mul_epu32 multiplica:
     *   elemento d[0] * elemento d[0] -> resultado q[0] (64 bits)
     *   elemento d[2] * elemento d[2] -> resultado q[1] (64 bits)
     */
    vX.d[0] = x0;  vX.d[1] = 0;  vX.d[2] = x1;  vX.d[3] = 0;
    vY.d[0] = y0;  vY.d[1] = 0;  vY.d[2] = y1;  vY.d[3] = 0;
    vY_swap.d[0] = y1;  vY_swap.d[1] = 0;  vY_swap.d[2] = y0;  vY_swap.d[3] = 0;

    /* Multiplicacion de terminos directos (diagonales):
     * P_diag.q[0] = x0 * y0
     * P_diag.q[1] = x1 * y1
     */
    P_diag.v = _mm_mul_epu32(vX.v, vY.v);

    /* Multiplicacion de terminos cruzados:
     * P_cross.q[0] = x0 * y1
     * P_cross.q[1] = x1 * y0
     */
    P_cross.v = _mm_mul_epu32(vX.v, vY_swap.v);

    /* Suma de los terminos cruzados y deteccion de acarreo */
    unsigned long long c0 = P_cross.q[0];
    unsigned long long c1 = P_cross.q[1];
    unsigned long long cross = c0 + c1;
    unsigned long long cross_carry = (cross < c0) ? 1ULL : 0ULL;

    /* El termino cruzado esta alineado con 2^32 */
    unsigned long long cross_low = cross << 32;
    unsigned long long cross_high = (cross >> 32) | (cross_carry << 32);

    /* Suma al producto bajo (64 bits) y propagacion de acarreo */
    unsigned long long low64 = P_diag.q[0] + cross_low;
    unsigned long long carry = (low64 < cross_low) ? 1ULL : 0ULL;
    unsigned long long high64 = P_diag.q[1] + cross_high + carry;

    /* Empaquetar resultado en la union de 128 bits */
    resultado.q[0] = low64;
    resultado.q[1] = high64;
    return resultado;
}

/* -------------------------------------------------------------------------
 * 5. Multiplicador de 128 bits * 128 bits -> 256 bits (Resultado Completo)
 *
 *    Sean:
 *      A = A.q[1]*2^64 + A.q[0]
 *      B = B.q[1]*2^64 + B.q[0]
 *
 *    Productos parciales de 64x64 bits a 128 bits:
 *      P00 = A0 * B0 = (H00 << 64) + L00
 *      P01 = A0 * B1 = (H01 << 64) + L01
 *      P10 = A1 * B0 = (H10 << 64) + L10
 *      P11 = A1 * B1 = (H11 << 64) + L11
 *
 *    Suma por columnas en palabras de 64 bits (R0, R1, R2, R3):
 *      R0 = L00
 *      R1 = H00 + L01 + L10 + acarreo(0)
 *      R2 = H01 + H10 + L11 + acarreo(1)
 *      R3 = H11 + acarreo(2)
 * ------------------------------------------------------------------------- */
uint256_u mul128_full(uint128_u A, uint128_u B) {
    uint256_u res;

    /* Paso 1: Cuatro multiplicaciones de 64x64 usando intrinsics */
    uint128_u P00 = mul64_intrinsics(A.q[0], B.q[0]);
    uint128_u P01 = mul64_intrinsics(A.q[0], B.q[1]);
    uint128_u P10 = mul64_intrinsics(A.q[1], B.q[0]);
    uint128_u P11 = mul64_intrinsics(A.q[1], B.q[1]);

    /* Columna 0 (bits 0 a 63) */
    res.q[0] = P00.q[0];

    /* Columna 1 (bits 64 a 127): H00 + L01 + L10 */
    unsigned long long carry1 = 0;
    unsigned long long s1 = P00.q[1] + P01.q[0];
    if (s1 < P00.q[1]) carry1++;

    unsigned long long s2 = s1 + P10.q[0];
    if (s2 < s1) carry1++;
    res.q[1] = s2;

    /* Columna 2 (bits 128 a 191): H01 + H10 + L11 + carry1 */
    unsigned long long carry2 = 0;
    unsigned long long t1 = P01.q[1] + P10.q[1];
    if (t1 < P01.q[1]) carry2++;

    unsigned long long t2 = t1 + P11.q[0];
    if (t2 < t1) carry2++;

    unsigned long long t3 = t2 + carry1;
    if (t3 < t2) carry2++;
    res.q[2] = t3;

    /* Columna 3 (bits 192 a 255): H11 + carry2 */
    res.q[3] = P11.q[1] + carry2;

    return res;
}

/* -------------------------------------------------------------------------
 * 6. Multiplicador de 128 bits * 128 bits -> 128 bits (Truncado a 128 bits)
 *    Equivalente a multiplicar enteros modulo 2^128 (sin guardar desbordamiento).
 * ------------------------------------------------------------------------- */
uint128_u mul128_low(uint128_u A, uint128_u B) {
    uint128_u res;
    /* P00 proporciona los 64 bits inferiores y contribuye a los 64 superiores */
    uint128_u P00 = mul64_intrinsics(A.q[0], B.q[0]);

    /* Solo necesitamos la parte baja de los productos cruzados */
    unsigned long long cross1 = A.q[0] * B.q[1];
    unsigned long long cross2 = A.q[1] * B.q[0];

    res.q[0] = P00.q[0];
    res.q[1] = P00.q[1] + cross1 + cross2;
    return res;
}

/* -------------------------------------------------------------------------
 * 7. Demostracion integrando con la union MSGunion del profesor
 * ------------------------------------------------------------------------- */
void demo_con_MSGunion(uint128_u A, uint128_u B) {
    union MSGunion buffer;

    printf("\n>>> Demostracion integrando directamente con union MSGunion del profesor <<<\n");

    /* Almacenamos el operando A en MSG128[0] y el operando B en MSG128[1] */
    buffer.MSG128[0] = A.v;
    buffer.MSG128[1] = B.v;

    /* Calculamos el producto completo de 256 bits */
    uint256_u producto = mul128_full(A, B);

    /* Guardamos el resultado en MSG256[1] */
    buffer.MSG256[1] = producto.v;

    printf("Operando A (almacenado en MSG128[0]) : ");
    print_m128i(&buffer.MSG128[0]);
    printf("\n");

    printf("Operando B (almacenado en MSG128[1]) : ");
    print_m128i(&buffer.MSG128[1]);
    printf("\n");

    printf("Producto 256 bits (en MSG256[1])    : ");
    print_m256i(&buffer.MSG256[1]);
    printf("\n");
}

/* -------------------------------------------------------------------------
 * 8. Funcion auxiliar para inicializar uint128_u desde dos enteros de 64 bits
 * ------------------------------------------------------------------------- */
static inline uint128_u make_uint128(unsigned long long high64, unsigned long long low64) {
    uint128_u u;
    u.q[0] = low64;
    u.q[1] = high64;
    return u;
}

/* -------------------------------------------------------------------------
 * 9. Casos de prueba y Funcion principal
 * ------------------------------------------------------------------------- */
void ejecutar_prueba(const char *titulo, uint128_u A, uint128_u B) {
    printf("================================================================================\n");
    printf("PRUEBA: %s\n", titulo);
    printf("================================================================================\n");

    printf("A                = 0x");
    print_m128i(&A.v);
    printf("\n");

    printf("B                = 0x");
    print_m128i(&B.v);
    printf("\n");

    /* Multiplicacion completa a 256 bits */
    uint256_u P256 = mul128_full(A, B);
    printf("A * B (256 bits) = 0x");
    print_m256i(&P256.v);
    printf("\n");

    /* Multiplicacion truncada a 128 bits */
    uint128_u P128 = mul128_low(A, B);
    printf("A * B (128 bits) = 0x");
    print_m128i(&P128.v);
    printf("\n\n");
}

int main(void) {
    printf("====================================================================\n");
    printf("   MULTIPLICADOR DE ENTEROS DE 128 BITS (UNIONES + INTEL INTRINSICS)\n");
    printf("====================================================================\n\n");

    /* Caso 1: Multiplicacion simple (3 * 5 = 15 = 0xF) */
    {
        uint128_u A = make_uint128(0x0ULL, 0x3ULL);
        uint128_u B = make_uint128(0x0ULL, 0x5ULL);
        ejecutar_prueba("1. Multiplicacion de numeros pequenos (3 * 5 = 15)", A, B);
    }

    /* Caso 2: Limite de 64 bits (2^64 * 2 = 2^65) */
    {
        uint128_u A = make_uint128(0x1ULL, 0x0ULL); // 2^64
        uint128_u B = make_uint128(0x2ULL, 0x0ULL); // 2 * 2^64
        ejecutar_prueba("2. Multiplicacion cruzando limites de 64 bits (2^64 * 2*2^64)", A, B);
    }

    /* Caso 3: Dos numeros de 64 bits maximos (0xFFFFFFFFFFFFFFFF * 0xFFFFFFFFFFFFFFFF) */
    {
        uint128_u A = make_uint128(0x0ULL, 0xFFFFFFFFFFFFFFFFULL);
        uint128_u B = make_uint128(0x0ULL, 0xFFFFFFFFFFFFFFFFULL);
        ejecutar_prueba("3. Multiplicacion de dos uint64 maximos (2^64 - 1)^2", A, B);
    }

    /* Caso 4: Valores arbitrarios grandes de 128 bits */
    {
        uint128_u A = make_uint128(0x0000000123456789ULL, 0xABCDEF0123456789ULL);
        uint128_u B = make_uint128(0x000000023456789AULL, 0xBCDEF0123456789AULL);
        ejecutar_prueba("4. Multiplicacion de dos valores grandes de 128 bits", A, B);
    }

    /* Caso 5: Maximo valor de 128 bits (2^128 - 1)^2 */
    {
        uint128_u A = make_uint128(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
        uint128_u B = make_uint128(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
        ejecutar_prueba("5. Caso limite maximo (2^128 - 1)^2", A, B);
    }

    /* Demostracion adicional con union MSGunion */
    {
        uint128_u A = make_uint128(0x1234567890ABCDEFULL, 0x1122334455667788ULL);
        uint128_u B = make_uint128(0xFEDCBA0987654321ULL, 0x9988776655443322ULL);
        demo_con_MSGunion(A, B);
    }

    return 0;
}
