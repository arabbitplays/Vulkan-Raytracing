from pathlib import Path

import numpy as np
from numpy.polynomial.legendre import Legendre, leggauss
from existance_check import check_existance
from gurobi_solver import solve_qp
from plotting import plotPhaseAndReconstruction, henyey_greenstein
import matplotlib.pyplot as plt

DEFAULT_COEFFS_GLSL = (
    Path(__file__).resolve().parent.parent.parent
    / "shaders" / "similarity" / "altered_phase_coefficients.glsl"
)

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

MIN_ORDER = 2

def generate_coefficients_glsl(output_path=DEFAULT_COEFFS_GLSL, k=360, N_max=10):
    """Generate altered_phase_coefficients.glsl with tables for a grid of
    (g, alpha) values.

    g sweeps 0.1..0.9 in steps of 0.1. For each g, alpha sweeps 1-g..1.0 in
    steps of 0.1. Each (g, alpha) pair must admit an order-N solution with
    N >= MIN_ORDER (=2); if N=2 existence fails or the QP is infeasible, the
    alpha is bumped by +0.1 and retried until a valid table is found or the
    physical bound alpha=1.0 is exceeded. The stored ALPHAS[i] therefore
    reflects the alpha actually solved with — the same value threaded through
    volume.similarity_alpha in the shader.
    """
    output_path = Path(output_path)
    g_values = [round(0.1 * i, 6) for i in range(1, 10)]

    entries = []
    for g in g_values:
        n_alphas = int(round(g / 0.1)) + 1   # base alphas from 1-g..1.0 inclusive
        base_alphas = [round(1.0 - g + 0.1 * j, 6) for j in range(n_alphas)]

        for base_alpha in base_alphas:
            alpha = base_alpha
            while alpha <= 1.0 + 1e-9:
                f_full = altered_phase_moments(g, N_max, alpha)
                if not check_existance(f_full[0:MIN_ORDER + 1]):
                    print(f"  bump   g={g:.2f}  alpha={alpha:.2f} -> {alpha + 0.1:.2f}  (N={MIN_ORDER} existence fails)")
                    alpha = round(alpha + 0.1, 6)
                    continue

                n = MIN_ORDER
                while n < N_max and check_existance(f_full[0:n + 2]):
                    n += 1
                f = f_full[0:n + 1]

                G = moments_matrix(n, k)
                c = solve_qp(G, f, k)

                if c is None:
                    print(f"  bump   g={g:.2f}  alpha={alpha:.2f} -> {alpha + 0.1:.2f}  N={n}  (no optimal QP solution)")
                    alpha = round(alpha + 0.1, 6)
                    continue

                if any(abs(g - eg) < 1e-9 and abs(alpha - ea) < 1e-9 for eg, ea, _, _ in entries):
                    print(f"  dup    g={g:.2f}  alpha={alpha:.2f}  N={n}  (already added, skipping)")
                    break

                print(f"  added  g={g:.2f}  base_alpha={base_alpha:.2f}  alpha={alpha:.2f}  N={n}")
                entries.append((float(g), float(alpha), int(n), np.asarray(c, dtype=float)))
                break
            else:
                print(f"  SKIP   g={g:.2f}  base_alpha={base_alpha:.2f}  (no N>={MIN_ORDER} solution up to alpha=1.0)")

    if not entries:
        raise RuntimeError("No coefficient tables generated; aborting write.")

    print(f"Writing {len(entries)} tables to {output_path}")

    lines = [
        "#ifndef ALTERED_PHASE_COEFFICIENTS",
        "#define ALTERED_PHASE_COEFFICIENTS",
        "",
        f"const int NUM_KEYS = {len(entries)};",
        f"const int NUM_COEFFS = {k};",
        "",
        "const float G_KEYS[NUM_KEYS] = float[](",
        ",\n".join(f"    {g:.6g}" for g, _, _, _ in entries),
        ");",
        "",
        "const float ALPHAS[NUM_KEYS] = float[](",
        ",\n".join(f"    {alpha:.6g}" for _, alpha, _, _ in entries),
        ");",
        "",
    ]

    for i, (g, alpha, n, c) in enumerate(entries):
        vals = ", ".join(f"{x:.6g}" for x in np.ravel(c))
        lines.append(f"// g = {g:.2f}, alpha = {alpha:.2f}, N = {n}")
        lines.append(f"const float COEFFS_{i}[NUM_COEFFS] = float[]({vals});")
        lines.append("")

    lines.append("float fetchPhaseCoefficient(int tableIdx, int i) {")
    lines.append("    switch (tableIdx) {")
    for i in range(len(entries)):
        lines.append(f"        case {i}: return COEFFS_{i}[i];")
    lines.append("        default: return 0.0;")
    lines.append("    }")
    lines.append("}")
    lines.append("")
    lines.append("#endif")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(lines) + "\n")

def solve_and_show_specific(g, alpha, N_max = 10, k = 360):
    f = altered_phase_moments(g, N_max, alpha)
    #f = legendre_moments(henyey_greenstein, g, N)

    n = 1
    while check_existance(f[0:n + 2]) and n < N_max:
        n = n + 1

    f = f[0:n+1]

    print("Found solution for N = " + str(n))

    G = moments_matrix(n, k)

    c = solve_qp(G, f, k)
    plotPhaseAndReconstruction(c, g)

    print(to_glsl_array(c))

if __name__ == "__main__":
    plt.rcParams.update({'font.size': 20})   # default is 10
    generate_coefficients_glsl()
