#include <time.h>
#include <memory>
#include <iostream>
extern "C"
{
#include <immintrin.h>
}

using namespace std;

typedef unsigned long long bench_t;

static bench_t before;
static bench_t after;


static inline bench_t cycles(void) {
	unsigned int hi, lo;
	__asm__ __volatile__ ("rdtsc\n\t":"=a" (lo), "=d"(hi));
	return ((bench_t) lo) | (((bench_t) hi) << 32);
}

// Versión secuencial en precisión simple (float)

float horner(float X, float *coef, long size){
	float ACC = 0.0f;
	int i;
	for(i = 0; i < size; i++){
		ACC = (ACC + coef[i]) * X;
	}
	return ACC;	
}

// Versión vectorizada con Intrinsics AVX en precisión simple (float)
float horner_intrinsic(float X, float *coef, long size){
	float *R, P;
	int i;
	__m256 *ymm0, X256, Y;	
	
	// Un registro AVX (__m256) almacena 8 floats de 32 bits
	ymm0 = (__m256*)coef; 
	
	// Precalculamos potencias de X:
	// Como avanzamos de a 8 elementos, multiplicamos acumuladores por X^8
	float X2 = X * X;
	float X4 = X2 * X2;
	float X8 = X4 * X4;
	
	X256 = _mm256_set1_ps(X8);       // _ps para "Packed Single" (float)
	Y = _mm256_set1_ps(0.0f);
    
	// Procesamos de a bloques de 8 floats
	for(i = 0; i < size/8 - 1; i++){
		Y = _mm256_add_ps(Y, ymm0[i]);
		Y = _mm256_mul_ps(Y, X256);
	}
	
	Y = _mm256_add_ps(Y, ymm0[i]);
	
	R = (float *)(&Y);
	
	// Reducción final: cada posición R[k] necesita multiplicarse por X^(8 - k)
	// R[7] corresponde a coeficientes con desplazamiento 7 -> necesita X^1
	// R[6] -> X^2, ..., R[0] -> X^8
	float X3 = X2 * X;
	float X5 = X4 * X;
	float X6 = X4 * X2;
	float X7 = X6 * X;

	P  = R[7] * X;
	P += R[6] * X2;
	P += R[5] * X3;
	P += R[4] * X4;
	P += R[3] * X5;
	P += R[2] * X6;
	P += R[1] * X7;
	P += R[0] * X8;

	return P;
}

int main(){
	// Nota: Si X > 1.0 (ej. 1.1) y size = 10000, 1.1^10000 desborda a infinito 
	// Usamos un valor cercano a 1.0 para verificar valores numéricos finitos,
	// o 1.1f si se quiere reproducir exactamente el apunte del profesor.
	float X = 1.0001f; 
	float R;
	int i, num_trails = 100000;
	
	clock_t t1, t2;
	
	srand(time(NULL));	

	float *coeficientes;
	int j;
	// 32 bytes de alineación requeridos por AVX (8 floats * 4 bytes = 32 bytes)
	coeficientes = (float *)_mm_malloc(10000 * sizeof(float), 32);

	for(j = 0; j < 1; j++){
		for(i = 0; i < 10000; i++){
			coeficientes[i] = (float)(rand() % 1000) / 1000.0f;
			if(i < 10)
				cout << coeficientes[i] << endl;
		}
		R = horner(X, coeficientes, 10000);
		cout << "Horner secuencial: " << R << endl;
		R = horner_intrinsic(X, coeficientes, 10000);
		cout << "Horner intrinsics: " << R << endl;		
	}
	
	t1 = clock();
	for(j = 0; j < num_trails; j++)	
		R = horner(X, coeficientes, 10000);
	t2 = clock();
  	float diff = (((float)t2 - (float)t1) / CLOCKS_PER_SEC);
  	cout << "Time taken (secuencial): " << diff << endl;

	t1 = clock();
	for(j = 0; j < num_trails; j++)	
		R = horner_intrinsic(X, coeficientes, 10000);
	t2 = clock();
  	diff = (((float)t2 - (float)t1) / CLOCKS_PER_SEC);
  	cout << "Time taken (intrinsics): " << diff << endl;
		
	_mm_free(coeficientes);

	return 0;
}
