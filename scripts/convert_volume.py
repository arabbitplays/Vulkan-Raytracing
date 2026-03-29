import openvdb
import numpy as np
import sys

input_path = sys.argv[1]
output_path = sys.argv[2]

grid = openvdb.read(input_path)["density"]

bbox = grid.evalActiveVoxelBoundingBox()
min_v = np.array(bbox.min())
max_v = np.array(bbox.max())
extent = max_v - min_v

Nx, Ny, Nz = 128, 128, 128

data = np.zeros((Nz, Ny, Nx, 2), dtype=np.float32)

scale = 10.0
albedo = 0.9

for z in range(Nz):
    for y in range(Ny):
        for x in range(Nx):
            u = (x + 0.5) / Nx
            v = (y + 0.5) / Ny
            w = (z + 0.5) / Nz

            ijk = min_v + np.array([u, v, w]) * extent

            density = grid.sample(ijk)

            sigma_t = max(density, 0.0) * scale
            sigma_s = sigma_t * albedo
            sigma_a = sigma_t - sigma_s

            data[z, y, x, 0] = sigma_s
            data[z, y, x, 1] = sigma_a

data.tofile(output_path)