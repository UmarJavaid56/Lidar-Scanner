import open3d as o3d
import numpy as np

vertical_layers = 3
rotation_samples = 64

pcd = o3d.io.read_point_cloud("./pointcloud.xyz", format='xyz')

lines = []

# create loops
for layer in range(vertical_layers):

    offset = rotation_samples * layer

    for i in range(rotation_samples):
        if i < rotation_samples - 1:
            lines.append([i + offset, i+1 + offset])
        else:
            lines.append([i + offset, offset])


# connect the loops
for z in range(vertical_layers-1):
    startIndex = z*rotation_samples
    nextStartIndex = (z+1)*rotation_samples

    for i in range(rotation_samples):
        lines.append([startIndex+i, nextStartIndex+i])


pts = np.asarray(pcd.points)
#5 print(pts)


line_set = o3d.geometry.LineSet(
    points=o3d.utility.Vector3dVector(pts),
    lines=o3d.utility.Vector2iVector(lines),
)

o3d.visualization.draw_geometries([line_set], width=1280, height=720)
