import os
import sys
import time
import heapq
from collections import Counter

# =====================================================================

# =====================================================================
class HuffmanNodePy:
    def __init__(self, char, freq):
        self.char = char
        self.freq = freq
        self.left = None
        self.right = None

    def __lt__(self, other):
        return self.freq < other.freq

def build_tree_python(frequencies):
    heap = [HuffmanNodePy(char, freq) for char, freq in frequencies.items()]
    heapq.heapify(heap)

    while len(heap) > 1:
        left = heapq.heappop(heap)
        right = heapq.heappop(heap)
        merged = HuffmanNodePy(None, left.freq + right.freq)
        merged.left = left
        merged.right = right
        heapq.heappush(heap, merged)

    return heap[0] if heap else None

def generate_codes_python(node, current_code="", codes=None):
    if codes is None:
        codes = {}
    if node is None:
        return codes

    if node.char is not None:
        codes[node.char] = current_code

    generate_codes_python(node.left, current_code + "0", codes)
    generate_codes_python(node.right, current_code + "1", codes)
    return codes

def encode_python(text):
    freq = Counter(text)
    root = build_tree_python(freq)
    codes = generate_codes_python(root)
    encoded = "".join(codes[ch] for ch in text)
    return encoded, codes


# =====================================================================
# 
# =====================================================================
cython_code = """# cython: language_level=3
import heapq

cdef class HuffmanNodeCy:
    cdef public object char
    cdef public int freq
    cdef public HuffmanNodeCy left
    cdef public HuffmanNodeCy right

    def __init__(self, object char, int freq):
        self.char = char
        self.freq = freq
        self.left = None
        self.right = None

    def __lt__(self, HuffmanNodeCy other):
        return self.freq < other.freq

cdef HuffmanNodeCy _build_tree_cy(dict frequencies):
    cdef list heap = [HuffmanNodeCy(char, freq) for char, freq in frequencies.items()]
    heapq.heapify(heap)

    cdef HuffmanNodeCy left, right, merged
    while len(heap) > 1:
        left = heapq.heappop(heap)
        right = heapq.heappop(heap)
        merged = HuffmanNodeCy(None, left.freq + right.freq)
        merged.left = left
        merged.right = right
        heapq.heappush(heap, merged)

    return heap[0] if heap else None

cdef void _generate_codes_cy(HuffmanNodeCy node, str current_code, dict codes):
    if node is None:
        return
    if node.char is not None:
        codes[node.char] = current_code
        return
    _generate_codes_cy(node.left, current_code + "0", codes)
    _generate_codes_cy(node.right, current_code + "1", codes)

def encode_cython(str text):
    # Frecuencias usando tipos optimizados
    cdef dict freq = {}
    cdef str ch
    for ch in text:
        freq[ch] = freq.get(ch, 0) + 1

    cdef HuffmanNodeCy root = _build_tree_cy(freq)
    cdef dict codes = {}
    _generate_codes_cy(root, "", codes)

    cdef list result = [codes[ch] for ch in text]
    return "".join(result), codes
"""

# Carga o compilación automática de Cython
def get_cython_encoder():
    try:
        import pyximport
        pyximport.install()
        # Si pyximport está disponible, escribe temporalmente el módulo
        with open("_huffman_cy.pyx", "w") as f:
            f.write(cython_code)
        import _huffman_cy
        return _huffman_cy.encode_cython
    except Exception:
        # Fallback usando Cython inline / compilador
        from Cython.Build.Inline import cython_inline
        # Ejecuta la definición compilándola al vuelo
        context = {}
        exec(cython_inline(cython_code), context)
        return context["encode_cython"]


# =====================================================================
# 3. BENCHMARK COMPARATIVO
# =====================================================================
if __name__ == "__main__":
    # Generamos un texto de prueba repetitivo (~500,000 caracteres)
    sample_text = ("algoritmo de compresion huffman en c y cython comparando rendimiento " * 7500)
    print(f"Longitud del texto de prueba: {len(sample_text):,} caracteres\n")

    # Medición de Python Puro
    t0 = time.perf_counter()
    res_py, codes_py = encode_python(sample_text)
    t_python = (time.perf_counter() - t0) * 1000.0  # en milisegundos

    print(f"-> Tiempo Python: {t_python:.2f} ms")

    # Medición de Cython
    try:
        encode_cython = get_cython_encoder()
        t0 = time.perf_counter()
        res_cy, codes_cy = encode_cython(sample_text)
        t_cython = (time.perf_counter() - t0) * 1000.0  # en milisegundos

        print(f"-> Tiempo Cython: {t_cython:.2f} ms")
        print(f"\nResultado: Cython fue {t_python / t_cython:.2f}x más rápido que Python.")
        print(f"¿Ambos resultados coinciden en longitud?: {len(res_py) == len(res_cy)}")
    except ImportError:
        print("\nPara ejecutar la parte de Cython, instala Cython en tu entorno:")
        print("   pip install cython setuptools")
