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

    moments = np.zeros(N + 1)

    for n in range(N + 1):
        Pn = Legendre.basis(n)(x)
        moments[n] = np.sum(w * f(x, g) * Pn)

    return moments

def monomial_moments(f, g, N):
    x, w = leggauss(200)

    moments = np.zeros(N + 1)

    for n in range(N + 1):
        moments[n] = np.sum(w * f(x, g) * x**n)

    return moments

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

def phase_moments(g, N):
    return g ** np.arange(N + 1)

def altered_phase_moments(g, N, alpha):
    return alter_moment(phase_moments(g, N), alpha)

def altered_approx_phase_moments(g, N, alpha):
    return alter_moment(legendre_moments(henyey_greenstein, g, N), alpha)

def alter_moment(moment, alpha):
    return 1 - (1 - moment) / alpha

k = 360
N = 4
g = 0.7
alpha = 0.7

G = moments_matrix(N, k)
f = altered_phase_moments(g, N, alpha)
#f = altered_approx_phase_moments(g, N, alpha)
#f = phase_moments(g, N)
#f = legendre_moments(henyey_greenstein, g, N)

if not check_existance(f):
    print("Reconstruction does not exist")
else:
    c = solve_qp(G, f, k)
    plotPhaseAndReconstruction(c, g)