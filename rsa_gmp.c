/*
 * rsa_gmp.c
 * Ejemplo del algoritmo RSA usando la biblioteca GMP (GNU Multiple Precision)
 *
 * Algoritmo RSA:
 *   1. Generacion de claves:
 *      - Elegir dos primos grandes p y q
 *      - Calcular n = p * q  (modulo RSA)
 *      - Calcular fi(n) = (p-1)(q-1)  (funcion de Euler)
 *      - Elegir e tal que 1 < e < fi(n) y mcd(e, fi(n)) = 1
 *      - Calcular d = e^(-1) mod fi(n)  (inverso modular)
 *      - Clave publica:  (n, e)
 *      - Clave privada:  (n, d)
 *   2. Cifrado:   c = m^e mod n
 *   3. Descifrado: m = c^d mod n
 *
 * Compilacion:
 *   gcc -o rsa_gmp rsa_gmp.c -lgmp
 *
 * Requisitos:
 *   sudo apt install libgmp-dev   (Debian/Ubuntu)
 *   sudo dnf install gmp-devel    (Fedora/RHEL)
 *   brew install gmp              (macOS)
 *   En Windows: instalar GMP mediante MSYS2/MinGW o WSL
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gmp.h>

/* ---------------------------------------------------------
   Estructura para almacenar las claves RSA
   --------------------------------------------------------- */
typedef struct {
    mpz_t n;   /* modulo publico              */
    mpz_t e;   /* exponente publico           */
    mpz_t d;   /* exponente privado (secreto) */
    mpz_t p;   /* primo p (secreto)           */
    mpz_t q;   /* primo q (secreto)           */
} RSAKey;

void rsa_init(RSAKey *key)
{
    mpz_inits(key->n, key->e, key->d, key->p, key->q, NULL);
}

void rsa_clear(RSAKey *key)
{
    mpz_clears(key->n, key->e, key->d, key->p, key->q, NULL);
}

/* ---------------------------------------------------------
   Genera un primo aleatorio de `bits` bits
   usando el test de Miller-Rabin de GMP.
   --------------------------------------------------------- */
void generar_primo(mpz_t primo, int bits, gmp_randstate_t estado)
{
    do {
        mpz_urandomb(primo, estado, bits);
        mpz_setbit(primo, bits - 1);  /* bit mas significativo = 1 */
        mpz_setbit(primo, 0);         /* forzar numero impar       */
    } while (mpz_probab_prime_p(primo, 50) == 0);
    /*
     * mpz_probab_prime_p retorna:
     *   0 -> definitivamente compuesto
     *   1 -> probable primo (50 rondas de Miller-Rabin)
     *   2 -> primo exacto (solo para numeros pequenos)
     */
}

/* ---------------------------------------------------------
   Generacion de claves RSA
   bits_primo: tamano en bits de cada primo p y q
               (el modulo n tendra aprox. el doble de bits)
   --------------------------------------------------------- */
void rsa_generar_claves(RSAKey *key, int bits_primo)
{
    mpz_t phi, mcd, uno, p_menos1, q_menos1;
    mpz_inits(phi, mcd, uno, p_menos1, q_menos1, NULL);
    mpz_set_ui(uno, 1);

    /* Inicializar generador Mersenne Twister con semilla temporal */
    gmp_randstate_t estado;
    gmp_randinit_mt(estado);
    gmp_randseed_ui(estado, (unsigned long)time(NULL));

    printf("=== Generacion de claves RSA (%d bits por primo) ===\n\n",
           bits_primo);

    /* Paso 1: generar p y q primos distintos */
    generar_primo(key->p, bits_primo, estado);
    do {
        generar_primo(key->q, bits_primo, estado);
    } while (mpz_cmp(key->p, key->q) == 0);   /* garantizar p != q */

    /* Paso 2: n = p * q */
    mpz_mul(key->n, key->p, key->q);

    /* Paso 3: fi(n) = (p-1)(q-1) */
    mpz_sub_ui(p_menos1, key->p, 1);
    mpz_sub_ui(q_menos1, key->q, 1);
    mpz_mul(phi, p_menos1, q_menos1);

    /*
     * Paso 4: elegir e
     * e = 65537 (0x10001) es el exponente publico mas comun:
     *   - Es primo
     *   - Solo 2 bits en 1 (peso de Hamming minimo = exponenciacion rapida)
     *   - Ampliamente estandarizado (PKCS#1, RFC 3447)
     */
    mpz_set_ui(key->e, 65537);
    mpz_gcd(mcd, key->e, phi);

    /* Si mcd(e, fi(n)) != 1, buscamos otro e (raro con e=65537) */
    if (mpz_cmp(mcd, uno) != 0) {
        mpz_set_ui(key->e, 3);
        mpz_gcd(mcd, key->e, phi);
        while (mpz_cmp(mcd, uno) != 0) {
            mpz_add_ui(key->e, key->e, 2);
            mpz_gcd(mcd, key->e, phi);
        }
    }

    /* Paso 5: d = e^(-1) mod fi(n)  (inverso modular via algoritmo extendido de Euclides) */
    if (!mpz_invert(key->d, key->e, phi)) {
        fprintf(stderr, "Error: no existe inverso modular de e.\n");
        exit(EXIT_FAILURE);
    }

    /* Mostrar las claves generadas */
    gmp_printf("  p      = %Zd\n\n", key->p);
    gmp_printf("  q      = %Zd\n\n", key->q);
    gmp_printf("  n      = %Zd\n\n", key->n);
    gmp_printf("  fi(n)  = %Zd\n\n", phi);
    gmp_printf("  e      = %Zd\n\n", key->e);
    gmp_printf("  d      = %Zd\n\n", key->d);
    printf("  Clave publica  : (n, e)\n");
    printf("  Clave privada  : (n, d)\n\n");

    mpz_clears(phi, mcd, uno, p_menos1, q_menos1, NULL);
    gmp_randclear(estado);
}

/* ---------------------------------------------------------
   Cifrado RSA: c = m^e mod n
   --------------------------------------------------------- */
void rsa_cifrar(mpz_t cifrado, const mpz_t mensaje,
                const mpz_t e, const mpz_t n)
{
    if (mpz_cmp_ui(mensaje, 0) < 0 || mpz_cmp(mensaje, n) >= 0) {
        fprintf(stderr, "Error: el mensaje debe estar en [0, n-1].\n");
        exit(EXIT_FAILURE);
    }
    mpz_powm(cifrado, mensaje, e, n);   /* c = m^e mod n */
}

/* ---------------------------------------------------------
   Descifrado RSA: m = c^d mod n
   --------------------------------------------------------- */
void rsa_descifrar(mpz_t descifrado, const mpz_t cifrado,
                   const mpz_t d, const mpz_t n)
{
    mpz_powm(descifrado, cifrado, d, n);   /* m = c^d mod n */
}

/* ---------------------------------------------------------
   Convierte texto a entero grande (representacion big-endian)
   --------------------------------------------------------- */
void texto_a_entero(mpz_t resultado, const char *texto)
{
    mpz_import(resultado,
               strlen(texto),   /* numero de bytes         */
               1,               /* order: 1 = big-endian   */
               1,               /* size: 1 byte/elemento   */
               0,               /* endian nativo           */
               0,               /* nails: 0 bits ignorados */
               texto);
}

/* ---------------------------------------------------------
   Convierte un entero grande de vuelta a texto.
   Retorna buffer asignado con malloc (el llamador libera).
   --------------------------------------------------------- */
char *entero_a_texto(const mpz_t numero)
{
    size_t num_bytes = (mpz_sizeinbase(numero, 2) + 7) / 8;
    char *buf = (char *)calloc(num_bytes + 1, 1);
    if (!buf) { perror("calloc"); exit(EXIT_FAILURE); }
    size_t count;
    mpz_export(buf, &count, 1, 1, 0, 0, numero);
    buf[count] = '\0';
    return buf;
}

/* =========================================================
   MAIN: demostracion completa del algoritmo RSA
   ========================================================= */
int main(void)
{
    /* --- 1. Generacion de claves -------------------------*/
    RSAKey key;
    rsa_init(&key);

    /*
     * Primos de 512 bits -> modulo de ~1024 bits.
     * Para mayor seguridad use 1024 bits -> modulo de ~2048 bits.
     * NOTA: no usar RSA-1024 en produccion; RSA-2048 es el minimo recomendado.
     */
    rsa_generar_claves(&key, 512);

    /* --- 2. Cifrar y descifrar un mensaje de texto -------*/
    const char *texto_original = "Hola RSA con GMP!";
    printf("=== Cifrado de mensaje de texto ===\n\n");
    printf("  Mensaje original : \"%s\"\n\n", texto_original);

    mpz_t m, c, m_rec;
    mpz_inits(m, c, m_rec, NULL);

    texto_a_entero(m, texto_original);
    gmp_printf("  m (entero)       : %Zd\n\n", m);

    /* Verificar que m < n */
    if (mpz_cmp(m, key.n) >= 0) {
        fprintf(stderr,
                "Error: el mensaje es mayor o igual que el modulo n.\n"
                "Use un modulo mas grande o un mensaje mas corto.\n");
        mpz_clears(m, c, m_rec, NULL);
        rsa_clear(&key);
        return EXIT_FAILURE;
    }

    rsa_cifrar(c, m, key.e, key.n);
    gmp_printf("  c (cifrado)      : %Zd\n\n", c);

    rsa_descifrar(m_rec, c, key.d, key.n);
    gmp_printf("  m' (descifrado)  : %Zd\n\n", m_rec);

    char *texto_recuperado = entero_a_texto(m_rec);
    printf("  Texto recuperado : \"%s\"\n\n", texto_recuperado);

    if (mpz_cmp(m, m_rec) == 0)
        printf("  OK: cifrado/descifrado consistente.\n");
    else
        printf("  ERROR: m != m'.\n");

    /* --- 3. Cifrar un numero arbitrario ------------------*/
    printf("\n=== Cifrado de numero arbitrario (m = 42) ===\n\n");
    mpz_t num, enc, dec;
    mpz_inits(num, enc, dec, NULL);

    mpz_set_ui(num, 42);
    gmp_printf("  m  = %Zd\n", num);

    rsa_cifrar(enc, num, key.e, key.n);
    gmp_printf("  c  = %Zd\n", enc);

    rsa_descifrar(dec, enc, key.d, key.n);
    gmp_printf("  m' = %Zd\n", dec);

    printf("  %s\n",
           mpz_cmp(num, dec) == 0 ? "OK: descifrado correcto" : "ERROR");

    /* --- Limpieza ----------------------------------------*/
    free(texto_recuperado);
    mpz_clears(m, c, m_rec, num, enc, dec, NULL);
    rsa_clear(&key);

    return EXIT_SUCCESS;
}
