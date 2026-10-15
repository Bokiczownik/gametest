"""Ground grass generator: stylized low-poly grass blades and clumps for UE instancing.

Run inside Blender (e.g. via blender-mcp):
    exec(compile(open(P).read(), P, "exec"), {"__file__": P, "__name__": "__main__"})

Only touches the "GRASS" collection. Saves a copy of the session to BLEND_PATH and
exports every variant to EXPORT_DIR as FBX (no materials, object placed at origin).

Vertex data (read by the UE grass material, Vertex Color + UV0):
    R = height along blade (0 base -> 1 tip), G = random per blade, B = random per clump, A = 1
    UV0: U = across blade (0..1), V = height (same as R)
    Color attribute is FLOAT_COLOR and exported with colors_type='LINEAR', so values reach UE
    without sRGB conversion. Custom normals point straight up (0, 0, 1).
"""

import math
import os
import random

import bpy
from mathutils import Matrix, Vector

# ---------------------------------------------------------------- parameters
SEED = 1337
COLLECTION_NAME = "GRASS"

BLADE_HEIGHT = 0.35                  # base blade height (m)
BLADE_HEIGHT_RANGE = (0.25, 0.45)    # random height inside clumps (m)
BLADE_WIDTH = 0.04                   # width at base (m)
BLADE_SEGMENTS = 3                   # LOD0 segments (last one is a single triangle)
LOD1_SEGMENTS = 1                    # LOD1 segments (1 = single triangle)
TAPER_EXPONENT = 0.8                 # width = BLADE_WIDTH * (1 - t) ** TAPER_EXPONENT

# Forward bend (+Y) of the tip as a fraction of blade height.
SINGLE_BLADE_BENDS = {"A": 0.10, "B": 0.20, "C": 0.32}
CLUMP_BEND_RANGE = (0.08, 0.30)

CLUMP_NAMES = ["A", "B", "C", "D"]
CLUMP_BLADE_COUNT = (7, 12)
CLUMP_RADIUS = 0.12
CLUMP_MAX_TILT_DEG = 15.0
CLUMP_YAW_JITTER_DEG = 45.0          # random deviation from facing away from the clump centre
LOD1_KEEP_EVERY = 2                  # LOD1 keeps every n-th blade (half)

ROW_SPACING = 0.5                    # objects laid out along X
ROW_ORIGIN = Vector((0.0, 3.0, 0.0))  # keeps the row clear of other scene content

PREVIEW_MATERIAL = "M_Grass_Preview"
PREVIEW_BASE_HEX = "2E5A1C"
PREVIEW_TIP_HEX = "8BC34A"

_HERE = os.path.dirname(os.path.abspath(globals().get("__file__", os.path.join(os.getcwd(), "x"))))
PROJECT_ROOT = os.path.normpath(os.path.join(_HERE, "..", ".."))
ART_DIR = os.path.join(PROJECT_ROOT, "SourceArt", "Ground", "Grass")
BLEND_PATH = os.path.join(ART_DIR, "grass.blend")
EXPORT_DIR = os.path.join(ART_DIR, "Export")

DO_SAVE_BLEND = True
DO_EXPORT = True


# ---------------------------------------------------------------- geometry
def blade_geometry(height, bend, segments, matrix, g_value, b_value):
    """Returns (verts, faces, colors, uvs) for one blade transformed by matrix (base at origin)."""
    verts, colors, uvs = [], [], []
    half = BLADE_WIDTH * 0.5
    for i in range(segments):
        t = i / segments
        w = half * (1.0 - t) ** TAPER_EXPONENT
        y = bend * height * t * t
        z = height * t
        for u, x in ((0.0, -w), (1.0, w)):
            verts.append(matrix @ Vector((x, y, z)))
            colors.append((t, g_value, b_value, 1.0))
            uvs.append((u, t))
    verts.append(matrix @ Vector((0.0, bend * height, height)))
    colors.append((1.0, g_value, b_value, 1.0))
    uvs.append((0.5, 1.0))

    faces = []
    for i in range(segments - 1):
        a = i * 2
        faces.append((a, a + 1, a + 3, a + 2))
    last = (segments - 1) * 2
    faces.append((last, last + 1, len(verts) - 1))
    return verts, faces, colors, uvs


def build_mesh(name, blades):
    """blades: list of (verts, faces, colors, uvs). Creates a single mesh with all data layers."""
    all_v, all_f, all_c, all_uv = [], [], [], []
    for verts, faces, colors, uvs in blades:
        off = len(all_v)
        all_v.extend(verts)
        all_f.extend(tuple(i + off for i in f) for f in faces)
        all_c.extend(colors)
        all_uv.extend(uvs)

    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([tuple(v) for v in all_v], [], all_f)
    mesh.update()

    for attr in list(mesh.color_attributes):
        mesh.color_attributes.remove(attr)
    col = mesh.color_attributes.new(name="Col", type="FLOAT_COLOR", domain="POINT")
    for i, c in enumerate(all_c):
        col.data[i].color = c
    mesh.color_attributes.active_color = col
    mesh.color_attributes.render_color_index = list(mesh.color_attributes).index(col)

    uv_layer = mesh.uv_layers.new(name="UVMap")
    for loop in mesh.loops:
        uv_layer.data[loop.index].uv = all_uv[loop.vertex_index]

    mesh.shade_smooth()
    mesh.normals_split_custom_set_from_vertices([(0.0, 0.0, 1.0)] * len(mesh.vertices))
    return mesh


def single_blade(letter):
    data = blade_geometry(BLADE_HEIGHT, SINGLE_BLADE_BENDS[letter], BLADE_SEGMENTS,
                          Matrix.Identity(4), 0.5, 0.5)
    return build_mesh("SM_GrassBlade_" + letter, [data])


def clump_layout(rng):
    """Random blade placements for one clump."""
    b_value = rng.random()
    blades = []
    for _ in range(rng.randint(*CLUMP_BLADE_COUNT)):
        r = CLUMP_RADIUS * math.sqrt(rng.random())
        a = rng.uniform(0.0, 2.0 * math.pi)
        # Blade bends/leans along local +Y; face it roughly away from the clump centre.
        yaw = a - math.pi * 0.5 + math.radians(rng.uniform(-CLUMP_YAW_JITTER_DEG, CLUMP_YAW_JITTER_DEG))
        # Negative rotation around the width axis (X) leans towards +Y and keeps the base on z = 0.
        tilt = -math.radians(rng.uniform(0.0, CLUMP_MAX_TILT_DEG))
        matrix = (Matrix.Translation((r * math.cos(a), r * math.sin(a), 0.0))
                  @ Matrix.Rotation(yaw, 4, "Z")
                  @ Matrix.Rotation(tilt, 4, "X"))
        blades.append({
            "height": rng.uniform(*BLADE_HEIGHT_RANGE),
            "bend": rng.uniform(*CLUMP_BEND_RANGE),
            "matrix": matrix,
            "g": rng.random(),
        })
    return blades, b_value


def clump_meshes(letter, rng):
    blades, b_value = clump_layout(rng)
    lod0 = [blade_geometry(b["height"], b["bend"], BLADE_SEGMENTS, b["matrix"], b["g"], b_value)
            for b in blades]
    lod1 = [blade_geometry(b["height"], b["bend"], LOD1_SEGMENTS, b["matrix"], b["g"], b_value)
            for b in blades[::LOD1_KEEP_EVERY]]
    name = "SM_GrassClump_" + letter
    return build_mesh(name, lod0), build_mesh(name + "_LOD1", lod1)


# ---------------------------------------------------------------- scene
def hex_to_linear(hex_str):
    def chan(v):
        v /= 255.0
        return v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4
    return tuple(chan(int(hex_str[i:i + 2], 16)) for i in (0, 2, 4)) + (1.0,)


def sock(sockets, identifier):
    """Socket by identifier (names are localized)."""
    return next(s for s in sockets if s.identifier == identifier)


def preview_material():
    mat = bpy.data.materials.get(PREVIEW_MATERIAL) or bpy.data.materials.new(PREVIEW_MATERIAL)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    for n in list(nodes):
        if n.type not in ("BSDF_PRINCIPLED", "OUTPUT_MATERIAL"):
            nodes.remove(n)
    bsdf = next((n for n in nodes if n.type == "BSDF_PRINCIPLED"), None) \
        or nodes.new("ShaderNodeBsdfPrincipled")
    out = next((n for n in nodes if n.type == "OUTPUT_MATERIAL"), None) \
        or nodes.new("ShaderNodeOutputMaterial")
    vcol = nodes.new("ShaderNodeVertexColor")
    vcol.layer_name = "Col"
    sep = nodes.new("ShaderNodeSeparateColor")
    mix = nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    sock(mix.inputs, "A_Color").default_value = hex_to_linear(PREVIEW_BASE_HEX)
    sock(mix.inputs, "B_Color").default_value = hex_to_linear(PREVIEW_TIP_HEX)
    links.new(sock(vcol.outputs, "Color"), sep.inputs[0])
    links.new(sock(sep.outputs, "Red"), sock(mix.inputs, "Factor_Float"))
    links.new(sock(mix.outputs, "Result_Color"), sock(bsdf.inputs, "Base Color"))
    links.new(bsdf.outputs[0], out.inputs[0])
    vcol.location, sep.location, mix.location = (-700, 0), (-500, 0), (-300, 0)
    return mat


def grass_collection():
    coll = bpy.data.collections.get(COLLECTION_NAME)
    if coll is None:
        coll = bpy.data.collections.new(COLLECTION_NAME)
        bpy.context.scene.collection.children.link(coll)
    for obj in list(coll.objects):
        mesh = obj.data
        bpy.data.objects.remove(obj)
        if mesh and mesh.users == 0:
            bpy.data.meshes.remove(mesh)
    return coll


def export_fbx(obj, path):
    """Exports obj alone, at the origin, without materials. Restores selection afterwards."""
    view_layer = bpy.context.view_layer
    prev_sel = [o for o in view_layer.objects if o.select_get()]
    prev_active = view_layer.objects.active

    tmp = bpy.data.objects.new(obj.name, obj.data.copy())
    tmp.data.materials.clear()
    obj.users_collection[0].objects.link(tmp)
    obj.name, tmp.name = obj.name + "__src", obj.name  # export under the real name
    try:
        for o in prev_sel:
            o.select_set(False)
        tmp.select_set(True)
        view_layer.objects.active = tmp
        bpy.ops.export_scene.fbx(
            filepath=path, use_selection=True, object_types={"MESH"},
            use_mesh_modifiers=True, mesh_smooth_type="FACE",
            colors_type="LINEAR", prioritize_active_color=True,
            apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
            use_custom_props=False, bake_anim=False, path_mode="AUTO", embed_textures=False)
    finally:
        mesh = tmp.data
        real_name = tmp.name
        bpy.data.objects.remove(tmp)
        bpy.data.meshes.remove(mesh)
        obj.name = real_name
        for o in prev_sel:
            o.select_set(True)
        view_layer.objects.active = prev_active


def main():
    rng = random.Random(SEED)
    coll = grass_collection()
    mat = preview_material()

    meshes = [single_blade(letter) for letter in SINGLE_BLADE_BENDS]
    lod1s = []
    for letter in CLUMP_NAMES:
        lod0, lod1 = clump_meshes(letter, rng)
        meshes.append(lod0)
        lod1s.append(lod1)
    meshes.extend(lod1s)

    objects = []
    for i, mesh in enumerate(meshes):
        mesh.materials.append(mat)
        obj = bpy.data.objects.new(mesh.name, mesh)
        obj.location = ROW_ORIGIN + Vector((i * ROW_SPACING, 0.0, 0.0))
        coll.objects.link(obj)
        objects.append(obj)

    if DO_EXPORT:
        os.makedirs(EXPORT_DIR, exist_ok=True)
        for obj in objects:
            export_fbx(obj, os.path.join(EXPORT_DIR, obj.name + ".fbx"))
    if DO_SAVE_BLEND:
        os.makedirs(ART_DIR, exist_ok=True)
        bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH, copy=True)

    for obj in objects:
        tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        print(f"{obj.name}: verts={len(obj.data.vertices)} tris={tris}")


if __name__ == "__main__":
    main()
