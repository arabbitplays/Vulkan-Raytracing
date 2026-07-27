import numpy as np
import matplotlib.pyplot as plt

def henyey_greenstein(mu, g):
    return (1 / (4 * np.pi)) * (1 - g**2) / (1 + g**2 - 2*g*mu)**1.5

def evaluate_boxcar(x, c):
    k = len(c)

    # map x from [-1,1] -> [0,k)
    idx = np.minimum((x + 1) * k / 2, k - 1).astype(int)

    # clamp edge case (x == 1)
    idx = np.clip(idx, 0, k - 1)

    return c[idx]

def plotPhaseAndReconstruction(c, g):
    mu = np.linspace(-1, 1, 500)

    P = henyey_greenstein(mu, g)
    P_hat = evaluate_boxcar(mu, c)

    plt.plot(mu, P, label="Henyey Greenstein")
    plt.plot(mu, P_hat, label="Altered phase function")
    plt.xlabel(r'$\mu = \cos(\theta)$')
    plt.ylabel('Phase function $f_p(μ)$')
    plt.title(f'Original vs. altered phase function')
    plt.subplots_adjust(left=0.12, right=0.99)
    plt.grid(True)
    plt.legend()
    plt.show()

