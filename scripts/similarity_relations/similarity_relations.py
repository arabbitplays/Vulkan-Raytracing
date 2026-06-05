import numpy as np
from numpy.polynomial.legendre import Legendre, leggauss
from existance_check import check_existance
from gurobi_solver import solve_qp
from plotting import plotPhaseAndReconstruction, henyey_greenstein

def legendre(n, x):
    Pn = Legendre.basis(n)
    return Pn(x)

def int_legendre(n, a, b):
    Pn = Legendre.basis(n)
    F = Pn.integ()
    return F(b) - F(a)

def legendre_moments(f, g, N):
    # Gauss-Legendre quadrature points/weights
    x, w = leggauss(200)
    fx = f(x, g)

    moments = np.zeros(N + 1)

    for n in range(N + 1):
        Pn = Legendre.basis(n)(x)
        moments[n] = 2 * np.pi * np.sum(w * fx * Pn)

    return moments / moments[0] # enforce f_0 = 1

def basis_moment(i, n, k):
    a = -1 + (2 * i - 2) / k
    b = -1 + 2 * i / k
    return 2 * np.pi * int_legendre(n, a, b)

def moments_matrix(N, k):
    G = np.zeros((N + 1, k))
    for i in range(N + 1):
        for j in range(k):
            G[i, j] = basis_moment(j + 1, i, k)
    return G

def altered_phase_moments(g, N, alpha):
    return alter_moments(legendre_moments(henyey_greenstein, g, N), alpha)

def alter_moments(moments, alpha):
    altered = 1 - (1 - np.asarray(moments, dtype=float)) / alpha;
    altered[0] = 1.0
    return altered

def to_glsl_array(a, name="A"):
    vals = ", ".join(f"{x:.6g}" for x in np.ravel(a))
    return f"float {name}[{len(np.ravel(a))}] = float[]({vals});"

k = 360
N = 1
g = 0.4
alpha = 0.7

f = altered_phase_moments(g, N, alpha)
#f = legendre_moments(henyey_greenstein, g, N)

n = 1
while check_existance(f[0:n + 2]) and n < N:
    n = n + 1

f = f[0:n+1]

print("Found solution for N = " + str(n))

G = moments_matrix(n, k)

print(G.shape)
print(f.shape)

c = solve_qp(G, f, k)
plotPhaseAndReconstruction(c, g)

print(to_glsl_array(c))