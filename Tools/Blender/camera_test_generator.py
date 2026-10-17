"""Camera test scene: blockout meshes that occlude the player (bridge, debris, tower, ruin, ground).

Run inside Blender (e.g. via blender-mcp):
    exec(compile(open(P).read(), P, "exec"), {"__file__": P, "__name__": "__main__"})

Only touches the "CAMERATEST" collection. Saves a copy of the session to BLEND_PATH and exports
each object to EXPORT_DIR as FBX. Every object has its origin at ground level (z = 0) and is
built from boxes; UVs are box-projected, 1 UV unit = 1 m (for grid materials).
Heights are chosen for testing camera occlusion and simple parkour (steps 0.9 m).
"""

import math
import os

import bpy
import bmesh
from mathutils import Euler, Matrix, Vector

COLLECTION_NAME = "CAMERATEST"

_HERE = os.path.dirname(os.path.abspath(globals().get("__file__", os.path.join(os.getcwd(), "x"))))
PROJECT_ROOT = os.path.normpath(os.path.join(_HERE, "..", ".."))
ART_DIR = os.path.join(PROJECT_ROOT, "SourceArt", "CameraTest")
BLEND_PATH = os.path.join(ART_DIR, "camera_test.blend")
EXPORT_DIR = os.path.join(ART_DIR, "Export")

STEP = 0.9   # parkour step height (m)


def box(center, size, rot=(0.0, 0.0, 0.0)):
    """(center xyz m, size xyz m, rotation xyz degrees)"""
    return (center, size, rot)


BRIDGE_DECK_TOP, BRIDGE_DECK_T, BRIDGE_LEN, BRIDGE_W, BRIDGE_RAMP_RUN = 6.0, 0.6, 24.0, 5.0, 14.0


def bridge_deck():
    """Deck with railings. Bridge parts share one origin: place all three at the same location."""
    top, t, length, width = BRIDGE_DECK_TOP, BRIDGE_DECK_T, BRIDGE_LEN, BRIDGE_W
    return [
        box((0, 0, top - t * 0.5), (length, width, t)),
        box((0, width * 0.5 - 0.15, top + 0.5), (length, 0.3, 1.0)),          # railings
        box((0, -width * 0.5 + 0.15, top + 0.5), (length, 0.3, 1.0)),
    ]


def bridge_ramps():
    top, t, half, run = BRIDGE_DECK_TOP, BRIDGE_DECK_T, BRIDGE_LEN * 0.5, BRIDGE_RAMP_RUN
    a = math.atan2(top, run)
    ramp_len = math.hypot(top, run)
    parts = []
    for s in (1, -1):
        cx = s * (half + run * 0.5) - s * t * 0.5 * math.sin(a)
        cz = top * 0.5 - t * 0.5 * math.cos(a)
        parts.append(box((cx, 0, cz), (ramp_len, BRIDGE_W, t), (0, s * math.degrees(a), 0)))
    return parts


BRIDGE_PILLAR_OFFSETS = [(s * 7.0, y) for s in (1, -1) for y in (1.8, -1.8)]   # place one pillar actor at each (m)


def bridge_pillar():
    """One pillar, origin at its base centre; placed 4x at BRIDGE_PILLAR_OFFSETS relative to the bridge."""
    h = BRIDGE_DECK_TOP - BRIDGE_DECK_T
    return [box((0, 0, h * 0.5), (1.0, 1.0, h))]


DEBRIS_PIECES = [
    box((-3.1, 0.0, 0.45), (3.5, 2.5, 0.35), (0, -14, 0)),               # ramp onto first block
    box((0.0, 0.0, STEP * 0.5), (2.5, 2.5, STEP)),
    box((2.8, 0.6, STEP), (2.2, 2.5, STEP * 2), (0, 0, 12)),
    box((5.4, -0.6, STEP * 1.5), (2.2, 2.2, STEP * 3), (0, 0, -8)),
    box((8.0, 0.2, STEP * 2), (2.2, 2.6, STEP * 4), (0, 0, 5)),
    box((11.0, 0.5, STEP * 2.5), (3.2, 3.2, STEP * 5)),
    box((5.0, 3.2, 0.9), (4.5, 1.8, 0.35), (25, 0, 20)),                 # leaning slab (clutter)
    box((9.5, -2.8, 0.5), (1.2, 1.2, 5.0), (90, 0, 30)),                 # fallen column
]


def debris_piece(i):
    """Collapsed blocks rising in STEP increments; one mesh per piece, all sharing the debris origin."""
    return lambda: [DEBRIS_PIECES[i]]


def tower():
    """Solid core with a spiral of ledges every STEP up to the top platform."""
    core, height = 4.0, 20 * STEP
    parts = [box((0, 0, height * 0.5), (core, core, height))]
    for k in range(1, 21):
        ang = math.radians(45.0 * (k - 1))
        r = 3.2
        top = k * STEP
        parts.append(box((r * math.cos(ang), r * math.sin(ang), top - 0.2), (2.0, 2.0, 0.4),
                         (0, 0, math.degrees(ang))))
    parts.append(box((0, 0, height + 0.2), (5.0, 5.0, 0.4)))                  # top platform
    return parts


RUIN_W, RUIN_D, RUIN_H, RUIN_T = 10.0, 8.0, 4.0, 0.4


def ruin_walls():
    """Walled room with a 2 m door. Ruin parts share one origin: place all three at the same location."""
    w, d, h, t = RUIN_W, RUIN_D, RUIN_H, RUIN_T
    return [
        box((0, d * 0.5 - t * 0.5, h * 0.5), (w, t, h)),                        # back wall
        box((-w * 0.5 + t * 0.5, 0, h * 0.5), (t, d, h)),                       # left wall
        box((w * 0.5 - t * 0.5, 0, h * 0.5), (t, d, h)),                        # right wall
        box((-3.0, -d * 0.5 + t * 0.5, h * 0.5), (4.0, t, h)),                  # front wall, door 2 m
        box((3.0, -d * 0.5 + t * 0.5, h * 0.5), (4.0, t, h)),
    ]


def ruin_roof():
    """Half-collapsed roof over the room."""
    return [
        box((-1.5, 0, RUIN_H + 0.2), (7.0, RUIN_D, 0.4)),                       # roof (x -5..2)
        box((3.6, 0.5, 2.2), (3.5, 5.0, 0.35), (0, 35, 0)),                     # collapsed roof part
    ]


def ruin_tunnel():
    """Short covered tunnel at +x of the room."""
    t = RUIN_T
    tx, tl, tw, th = 13.0, 8.0, 3.0, 3.0
    return [
        box((tx, tw * 0.5 + t * 0.5, th * 0.5), (tl, t, th)),
        box((tx, -tw * 0.5 - t * 0.5, th * 0.5), (tl, t, th)),
        box((tx, 0, th + 0.2), (tl, tw + 2 * t, 0.4)),
    ]


def ground():
    return [box((0, 0, -0.25), (60.0, 60.0, 0.5))]


# Separate meshes per structural part, so the camera occlusion cuts only the part that hides the player.
# name -> (builder, group); parts of one group share an origin.
OBJECTS = {
    "SM_CamTest_Ground": (ground, None),
    "SM_CamTest_BridgeDeck": (bridge_deck, 0),
    "SM_CamTest_BridgeRamps": (bridge_ramps, 0),
    "SM_CamTest_BridgePillar": (bridge_pillar, 0),
    **{"SM_CamTest_Debris_%02d" % (i + 1): (debris_piece(i), 1) for i in range(len(DEBRIS_PIECES))},
    "SM_CamTest_Tower": (tower, 2),
    "SM_CamTest_RuinWalls": (ruin_walls, 3),
    "SM_CamTest_RuinRoof": (ruin_roof, 3),
    "SM_CamTest_RuinTunnel": (ruin_tunnel, 3),
}


def get_collection():
    col = bpy.data.collections.get(COLLECTION_NAME)
    if col is None:
        col = bpy.data.collections.new(COLLECTION_NAME)
        bpy.context.scene.collection.children.link(col)
    for obj in list(col.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    return col


def build(name, parts, col):
    bm = bmesh.new()
    for center, size, rot in parts:
        m = (Matrix.Translation(Vector(center))
             @ Euler([math.radians(r) for r in rot], "XYZ").to_matrix().to_4x4()
             @ Matrix.Diagonal(Vector(size)).to_4x4())
        bmesh.ops.create_cube(bm, size=1.0, matrix=m)
    bm.normal_update()
    uv = bm.loops.layers.uv.new("UVMap")
    for f in bm.faces:
        n = f.normal
        ax = max(range(3), key=lambda i: abs(n[i]))
        u, v = [(1, 2), (0, 2), (0, 1)][ax]
        for loop in f.loops:
            loop[uv].uv = (loop.vert.co[u], loop.vert.co[v])
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    col.objects.link(obj)
    return obj


def export(obj):
    os.makedirs(EXPORT_DIR, exist_ok=True)
    for o in bpy.context.selected_objects:
        o.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(
        filepath=os.path.join(EXPORT_DIR, obj.name + ".fbx"), use_selection=True,
        object_types={"MESH"}, use_mesh_modifiers=True, mesh_smooth_type="FACE",
        apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
        use_custom_props=False, bake_anim=False, path_mode="AUTO", embed_textures=False)


def main():
    col = get_collection()
    for name, (fn, group) in OBJECTS.items():
        obj = build(name, fn(), col)
        export(obj)                                    # exported at the origin (pivot at z = 0)
        if group is not None:
            obj.location.y = -22.5 + 15.0 * group      # spread out in the .blend for viewing only
        tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        lo = [min(v.co[k] for v in obj.data.vertices) for k in range(3)]
        hi = [max(v.co[k] for v in obj.data.vertices) for k in range(3)]
        print(f"{name}: tris={tris} min=({lo[0]:.2f},{lo[1]:.2f},{lo[2]:.2f}) max=({hi[0]:.2f},{hi[1]:.2f},{hi[2]:.2f})")
    os.makedirs(ART_DIR, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH, copy=True)


if __name__ == "__main__":
    main()
