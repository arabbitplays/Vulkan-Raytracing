import numpy as np
from numpy.polynomial.legendre import Legendre

def legendre_to_monomial_matrix(N):
    A = np.zeros((N+1, N+1))

    for n in range(N+1):
        Pn = Legendre.basis(n).convert(kind=np.polynomial.Polynomial)

        # Pn is now in monomial basis
        for k, c in enumerate(Pn.coef):
            A[n, k] = c

    return A

def legendre_moments_to_monomial(m_leg):
    N = len(m_leg) - 1
    A = legendre_to_monomial_matrix(N)

    # solve A m_x = m_leg
    return np.linalg.solve(A, m_leg)

def hankel_matrices(m_mono, n, calc_w = True, calc_v = True):
    U = np.zeros((n, n))
    V = np.zeros((n, n))
    W = np.zeros((n, n))

    for x in range(n):
        for y in range(n):
            i = x + 1
            j = y + 1
            U[x, y] = m_mono[i + j - 2]
            if calc_v:
                V[x, y] = m_mono[i + j - 1]
            if calc_w:
                W[x, y] = m_mono[i + j]

    return (U, V, W)

def is_positiv_semidefinite(M, tol=1e-10):
    eigvals = np.linalg.eigvalsh(M)  # for symmetric matrices
    return np.all(eigvals >= -tol)

def check_existance(m_leg):
    N = m_leg.shape[0] - 1
    m_mono = legendre_moments_to_monomial(m_leg)

    if N % 2 == 0:
        k = int(N / 2)
        U, _, _ = hankel_matrices(m_mono, k + 1, False, False)
        if not is_positiv_semidefinite(U):
            return False

        U, _, W = hankel_matrices(m_mono, k)
        if not is_positiv_semidefinite(U - W):
            return False
        return True
    else:
        k = int((N - 1) / 2)
        U, V, _ = hankel_matrices(m_mono, k + 1, False)
        return is_positiv_semidefinite(U - V) and is_positiv_semidefinite(U + V)
