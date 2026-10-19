"""Spear weapon mesh (low-poly, stylised) for the player's first weapon.

Run inside Blender (e.g. via blender-mcp):
    exec(compile(open(P).read(), P, "exec"), {"__file__": P, "__name__": "__main__"})
or headless:
    blender -b --factory-startup --python Tools/Blender/spear_generator.py

Only touches the "SPEAR" collection. Saves a copy of the session to BLEND_PATH and exports SM_Spear to
EXPORT_DIR as FBX. The origin is the grip point (where the right hand holds the spear); the shaft runs
along +X, the blade tip is at +X (TIP_X), the butt at -X (BUTT_X). The blade is flat in the XY plane.
Material slots: M_Spear_Wood, M_Spear_Metal, M_Spear_Wrap.
"""

import math
import os

import bpy
import bmesh
from mathutils import Matrix, Vector

COLLECTION_NAME = "SPEAR"
OBJECT_NAME = "SM_Spear"

_HERE = os.path.dirname(os.path.abspath(globals().get("__file__", os.path.join(os.getcwd(), "x"))))
PROJECT_ROOT = os.path.normpath(os.path.join(_HERE, "..", ".."))
ART_DIR = os.path.join(PROJECT_ROOT, "SourceArt", "Weapons", "Spear")
BLEND_PATH = os.path.join(ART_DIR, "spear.blend")
EXPORT_DIR = os.path.join(ART_DIR, "Export")

# Dimensions (m). Total length 2.23 m; grip 0.75 m from the butt.
BUTT_X = -0.75
SHAFT_END_X = 1.05          # shaft ends inside the blade socket
SHAFT_R = 0.017
SOCKET_X0, SOCKET_X1, SOCKET_R0, SOCKET_R1 = 0.98, 1.10, 0.024, 0.019
BLADE_X0, BLADE_X1 = 1.10, 1.48   # blade base and tip (TIP_X)
BLADE_HALF_W, BLADE_WIDEST_T = 0.065, 0.35   # widest point at 35 % of the blade length
BLADE_HALF_T = 0.011
TIP_X = BLADE_X1
BUTT_CAP_LEN, BUTT_CAP_R = 0.06, 0.021
WRAPS = [(-0.12, 0.24), (0.55, 0.62)]   # grip wrap and a second wrap near the head (x0, x1)
WRAP_R = 0.0205
SEGMENTS = 10

MATERIALS = {
    "M_Spear_Wood": (0.32, 0.19, 0.09, 1.0),
    "M_Spear_Metal": (0.62, 0.64, 0.66, 1.0),
    "M_Spear_Wrap": (0.45, 0.08, 0.06, 1.0),
}


def get_collection():
    col = bpy.data.collections.get(COLLECTION_NAME)
    if col is None:
        col = bpy.data.collections.new(COLLECTION_NAME)
        bpy.context.scene.collection.children.link(col)
    for obj in list(col.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    return col


def get_material(name):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Base Color"].default_value = MATERIALS[name]
    bsdf.inputs["Metallic"].default_value = 1.0 if "Metal" in name else 0.0
    bsdf.inputs["Roughness"].default_value = 0.35 if "Metal" in name else 0.8
    mat.diffuse_color = MATERIALS[name]
    return mat


def add_tube(bm, x0, x1, r0, r1, mat_index, cap=True):
    """Cylinder/cone along +X from x0 to x1 (radii r0 -> r1)."""
    length = x1 - x0
    geom = bmesh.ops.create_cone(bm, cap_ends=cap, cap_tris=False, segments=SEGMENTS,
                                 radius1=r0, radius2=r1, depth=length)
    verts = geom["verts"]
    # create_cone builds along Z centred at 0; rotate to +X and move into place
    rot = Matrix.Rotation(math.radians(90.0), 4, "Y")
    bmesh.ops.transform(bm, matrix=Matrix.Translation(Vector((x0 + length * 0.5, 0, 0))) @ rot, verts=verts)
    faces = {f for v in verts for f in v.link_faces}
    for f in faces:
        f.material_index = mat_index
        f.smooth = True
    return faces


def add_blade(bm, mat_index):
    """Leaf-shaped blade: diamond cross-section, flat in XY."""
    x0, x1 = BLADE_X0, BLADE_X1
    xw = x0 + (x1 - x0) * BLADE_WIDEST_T
    base_w = SOCKET_R1 * 0.9
    tip = bm.verts.new((x1, 0, 0))
    ring = []
    for x, hw, ht in ((x0, base_w, SOCKET_R1 * 0.9), (xw, BLADE_HALF_W, BLADE_HALF_T)):
        ring.append([bm.verts.new((x, hw, 0)), bm.verts.new((x, 0, ht)),
                     bm.verts.new((x, -hw, 0)), bm.verts.new((x, 0, -ht))])
    faces = []
    a, b = ring
    for i in range(4):
        j = (i + 1) % 4
        faces.append(bm.faces.new((a[i], a[j], b[j], b[i])))
        faces.append(bm.faces.new((b[i], b[j], tip)))
    faces.append(bm.faces.new(list(reversed(a))))
    for f in faces:
        f.material_index = mat_index
        f.smooth = False
    return faces


def build(col):
    mesh = bpy.data.meshes.new(OBJECT_NAME)
    for name in MATERIALS:
        mesh.materials.append(get_material(name))
    wood, metal, wrap = 0, 1, 2

    bm = bmesh.new()
    add_tube(bm, BUTT_X + BUTT_CAP_LEN, SHAFT_END_X, SHAFT_R, SHAFT_R, wood)
    add_tube(bm, BUTT_X, BUTT_X + BUTT_CAP_LEN, BUTT_CAP_R * 0.8, BUTT_CAP_R, metal)
    add_tube(bm, SOCKET_X0, SOCKET_X1, SOCKET_R0, SOCKET_R1, metal)
    for x0, x1 in WRAPS:
        add_tube(bm, x0, x1, WRAP_R, WRAP_R, wrap)
    add_blade(bm, metal)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)

    # Box-projected UVs (1 UV unit = 1 m); flat colour materials do not need more.
    uv = bm.loops.layers.uv.new("UVMap")
    for f in bm.faces:
        n = f.normal
        axis = max(range(3), key=lambda k: abs(n[k]))
        u_ax, v_ax = [(1, 2), (0, 2), (0, 1)][axis]
        for loop in f.loops:
            co = loop.vert.co
            loop[uv].uv = (co[u_ax], co[v_ax])

    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(OBJECT_NAME, mesh)
    col.objects.link(obj)
    return obj


def export(obj):
    os.makedirs(EXPORT_DIR, exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(
        filepath=os.path.join(EXPORT_DIR, obj.name + ".fbx"), use_selection=True,
        object_types={"MESH"}, use_mesh_modifiers=True, mesh_smooth_type="FACE",
        apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
        use_custom_props=False, bake_anim=False, path_mode="AUTO", embed_textures=False)


def main():
    col = get_collection()
    obj = build(col)
    export(obj)
    os.makedirs(ART_DIR, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH, copy=True)
    dims = obj.dimensions
    print("SPEAR tris=%d dims=(%.3f, %.3f, %.3f) bounds_x=(%.3f, %.3f)" % (
        sum(len(p.vertices) - 2 for p in obj.data.polygons), dims.x, dims.y, dims.z,
        min(v.co.x for v in obj.data.vertices), max(v.co.x for v in obj.data.vertices)))


if __name__ == "__main__":
    main()
