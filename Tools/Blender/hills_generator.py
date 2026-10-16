"""Test hills: a subdivided plane raised into gentle hills (ground for grass-on-slopes tests).

Run inside Blender (e.g. via blender-mcp):
    exec(compile(open(P).read(), P, "exec"), {"__file__": P, "__name__": "__main__"})

Only touches the "HILLS" collection. Saves a copy of the session to BLEND_PATH and exports
the mesh to EXPORT_DIR as FBX. Origin at the centre of the plane, z = 0 on the edges, so the
mesh sits flush with a flat floor at the same height. Slopes stay below ~30 deg (walkable).
"""

import math
import os

import bpy
import bmesh

# ---------------------------------------------------------------- parameters
COLLECTION_NAME = "HILLS"
OBJECT_NAME = "SM_TestHills"

SIZE_X = 32.0          # m
SIZE_Y = 32.0          # m
CELL = 0.5             # grid spacing (m)
EDGE_FALLOFF = 4.0     # distance from the border over which height fades to 0 (m)

# Gaussian bumps: (x, y, height, sigma) in metres, relative to the plane centre.
BUMPS = [
    (-6.0, -3.0, 3.0, 4.0),
    (4.0, 5.0, 2.2, 3.2),
    (8.0, -6.0, 1.6, 2.6),
    (-2.0, 7.0, 1.2, 2.4),
    (0.0, -7.0, -0.6, 3.0),   # shallow dip
]

_HERE = os.path.dirname(os.path.abspath(globals().get("__file__", os.path.join(os.getcwd(), "x"))))
PROJECT_ROOT = os.path.normpath(os.path.join(_HERE, "..", ".."))
ART_DIR = os.path.join(PROJECT_ROOT, "SourceArt", "Ground", "Hills")
BLEND_PATH = os.path.join(ART_DIR, "hills_test.blend")
EXPORT_DIR = os.path.join(ART_DIR, "Export")


def smoothstep(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3.0 - 2.0 * t)


def height(x, y):
    h = sum(a * math.exp(-((x - bx) ** 2 + (y - by) ** 2) / (2.0 * s * s)) for bx, by, a, s in BUMPS)
    edge = min(SIZE_X * 0.5 - abs(x), SIZE_Y * 0.5 - abs(y))
    return h * smoothstep(edge / EDGE_FALLOFF)


def get_collection():
    col = bpy.data.collections.get(COLLECTION_NAME)
    if col is None:
        col = bpy.data.collections.new(COLLECTION_NAME)
        bpy.context.scene.collection.children.link(col)
    for obj in list(col.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    return col


def build(col):
    nx, ny = int(round(SIZE_X / CELL)), int(round(SIZE_Y / CELL))
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    verts = []
    for j in range(ny + 1):
        row = []
        for i in range(nx + 1):
            x = -SIZE_X * 0.5 + i * CELL
            y = -SIZE_Y * 0.5 + j * CELL
            row.append(bm.verts.new((x, y, height(x, y))))
        verts.append(row)
    for j in range(ny):
        for i in range(nx):
            f = bm.faces.new((verts[j][i], verts[j][i + 1], verts[j + 1][i + 1], verts[j + 1][i]))
            f.smooth = True
            for loop in f.loops:
                loop[uv].uv = (loop.vert.co.x, loop.vert.co.y)   # 1 UV unit = 1 m
    mesh = bpy.data.meshes.new(OBJECT_NAME)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(OBJECT_NAME, mesh)
    col.objects.link(obj)
    return obj


def export(obj):
    os.makedirs(EXPORT_DIR, exist_ok=True)
    for o in bpy.context.selected_objects:
        o.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(
        filepath=os.path.join(EXPORT_DIR, OBJECT_NAME + ".fbx"), use_selection=True,
        object_types={"MESH"}, use_mesh_modifiers=True, mesh_smooth_type="FACE",
        apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
        use_custom_props=False, bake_anim=False, path_mode="AUTO", embed_textures=False)


def main():
    obj = build(get_collection())
    export(obj)
    os.makedirs(ART_DIR, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH, copy=True)
    zs = [v.co.z for v in obj.data.vertices]
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    print(f"{obj.name}: verts={len(zs)} tris={tris} z={min(zs):.2f}..{max(zs):.2f}")


if __name__ == "__main__":
    main()
