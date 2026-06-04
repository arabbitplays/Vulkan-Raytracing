import numpy as np
import gurobipy as gp
from gurobipy import GRB

def poison_matrix(n):
    A = (
        2 * np.eye(n)
        - np.eye(n, k=1)
        - np.eye(n, k=-1)
    )
    return A[1:-1]

def regularization_matrix(n):
    S = poison_matrix(n)
    return np.matmul(S.T, S)

def solve_qp(G, f, k):
    Q = regularization_matrix(k)

    m = gp.Model()
    m.setParam("OutputFlag", 0) # suppress solver logging
    m.setParam("InfUnbdInfo", 1) # track status flag

    c = m.addMVar(k, name="c")

    m.addConstr(G @ c == f)
    m.setObjective(0.5 * c @ Q @ c, GRB.MINIMIZE)

    m.optimize()

    if not m.Status == GRB.OPTIMAL:
        print("NO OPTIMAL SOLUTION FOUND! STATUS CODE " + str(m.Status))
        return None

    return c.X
