import numpy as np
import matplotlib.pyplot as plt

def henyey_greenstein(mu, g):
    return (1 - g**2) / (1 + g**2 - 2*g*mu)**1.5

def evaluate_boxcar(x, c):
    k = len(c)

    # map x from [-1,1] -> [0,k)
    idx = np.minimum((x + 1) * k / 2, k - 1).astype(int)

    # clamp edge case (x == 1)
    idx = np.clip(idx, 0, k - 1)

    return 4 * np.pi * c[idx]

def plotPhaseAndReconstruction(c, g):
    mu = np.linspace(-1, 1, 500)

    P = henyey_greenstein(mu, g)
    P_hat = evaluate_boxcar(mu, c)

    plt.plot(mu, P, label="Henyey Greenstein")
    plt.plot(mu, P_hat, label="Reconstruction")
    plt.xlabel(r'$\mu = \cos(\theta)$')
    plt.ylabel('Phase function P(μ)')
    plt.title(f'Henyey-Greenstein vs reconstruction (g={g})')
    plt.grid(True)
    plt.legend()
    plt.show()

