#include <stdio.h>
#include <stdlib.h>

// 1. Estructura única para un Nodo Doblemente Enlazado
typedef struct Nodo {
    int dato;
    struct Nodo* ant;
    struct Nodo* sig;
} Nodo;

// Estructura para la Pila (LIFO)
typedef struct {
    Nodo* tope;
} Pila;

// Estructura para la Cola (FIFO)
typedef struct {
    Nodo* frente;
    Nodo* final;
} Cola;

// --- FUNCIONES AUXILIARES ---
Nodo* crearNodo(int dato) {
    Nodo* nuevo = (Nodo*)malloc(sizeof(Nodo));
    nuevo->dato = dato;
    nuevo->ant = NULL;
    nuevo->sig = NULL;
    return nuevo;
}

// --- OPERACIONES DE LA PILA (LIFO) ---
void push(Pila* p, int dato) {
    Nodo* nuevo = crearNodo(dato);
    if (p->tope != NULL) {
        p->tope->sig = nuevo;
        nuevo->ant = p->tope;
    }
    p->tope = nuevo;
    printf("Pila (Push): %d\n", dato);
}

int pop(Pila* p) {
    if (p->tope == NULL) {
        printf("Error: Pila vacía.\n");
        return -1;
    }
    Nodo* temp = p->tope;
    int valor = temp->dato;
    
    p->tope = p->tope->ant;
    if (p->tope != NULL) {
        p->tope->sig = NULL;
    }
    
    free(temp);
    return valor;
}

// --- OPERACIONES DE LA COLA (FIFO) ---
void enqueue(Cola* c, int dato) {
    Nodo* nuevo = crearNodo(dato);
    if (c->final == NULL) {
        c->frente = nuevo;
        c->final = nuevo;
    } else {
        c->final->sig = nuevo;
        nuevo->ant = c->final;
        c->final = nuevo;
    }
    printf("Cola (Enqueue): %d\n", dato);
}

int dequeue(Cola* c) {
    if (c->frente == NULL) {
        printf("Error: Cola vacía.\n");
        return -1;
    }
    Nodo* temp = c->frente;
    int valor = temp->dato;
    
    c->frente = c->frente->sig;
    if (c->frente != NULL) {
        c->frente->ant = NULL;
    } else {
        c->final = NULL; // La cola quedó vacía
    }
    
    free(temp);
    return valor;
}

// --- PRUEBA DEL SISTEMA ---
int main() {
    // Inicializar Pila
    Pila miPila = { .tope = NULL };
    // Inicializar Cola
    Cola miCola = { .frente = NULL, .final = NULL };

    printf("=== Probando PILA (LIFO) ===\n");
    push(&miPila, 10);
    push(&miPila, 20);
    push(&miPila, 30);
    printf("Pop obtenido: %d\n", pop(&miPila)); // Debe ser 30
    printf("Pop obtenido: %d\n\n", pop(&miPila)); // Debe ser 20

    printf("=== Probando COLA (FIFO) ===\n");
    enqueue(&miCola, 100);
    enqueue(&miCola, 200);
    enqueue(&miCola, 300);
    printf("Dequeue obtenido: %d\n", dequeue(&miCola)); // Debe ser 100
    printf("Dequeue obtenido: %d\n", dequeue(&miCola)); // Debe ser 200

    return 0;
}

