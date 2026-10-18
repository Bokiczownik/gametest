"""Downloaded models (Poly Pizza .glb): rescale and export as FBX for Unreal.

Run inside Blender (e.g. via blender-mcp):
    exec(compile(open(P).read(), P, "exec"), {"__file__": P, "__name__": "__main__"})

Only touches the "DOWNLOADED" collection. Imports every entry of ASSETS from SRC_DIR, bakes the
glTF node transforms plus the entry's scale into the mesh, puts the origin at the bottom centre of
the asset (z = 0 on the ground) and exports one FBX per output mesh to EXPORT_DIR (textures
embedded). Multi-part models (chests) export each part separately, sharing the asset's origin, so
the parts line up when placed at the same actor location. Saves a copy of the session to BLEND_PATH.
Materials are merged by (name, base colour, texture) and renamed M_<Kit>_<Name> (suffix _2.. when a
kit uses one name for different colours), so Unreal creates one material per distinct look.

Scales: the Quaternius village kit has 0.269 m door leaves, KIT_SCALE 8.5 makes them ~2.29 m
(player capsule is 1.92 m). The CreativeTrio cottage door leaf is 0.096 m, scale 24 -> ~2.3 m.
"""

import os

import bpy
from mathutils import Matrix, Vector

COLLECTION_NAME = "DOWNLOADED"

_HERE = os.path.dirname(os.path.abspath(globals().get("__file__", os.path.join(os.getcwd(), "x"))))
PROJECT_ROOT = os.path.normpath(os.path.join(_HERE, "..", ".."))
SRC_DIR = os.path.join(PROJECT_ROOT, "SourceArt", "Downloaded")
BLEND_PATH = os.path.join(SRC_DIR, "downloaded_assets.blend")
EXPORT_DIR = os.path.join(SRC_DIR, "Export")

KIT_SCALE = 8.5

# (file id from the Poly Pizza name, output name, scale, material kit, {glTF node name: output suffix} or None for one joined mesh)
ASSETS = [
    ("O72u4Drp8k", "SM_Chest", 1.0, "Chest", {"Chest_Base": "_Base", "Chest_Top": "_Top"}),
    ("haqf9qoiOG", "SM_ChestGold", 1.0, "Chest", {"Chest_Base": "_Base", "Chest_Top": "_Top"}),
    ("YDGLLT0emC", "SM_Cottage", 24.0, "Cottage", None),
    ("INctJkmEAl", "SM_Farm_L2", KIT_SCALE, "Village", None),
    ("91wMLb9kKo", "SM_Farm_L3", KIT_SCALE, "Village", None),
    ("oJJIRwv6Bo", "SM_House_1_L1", KIT_SCALE, "Village", None),
    ("nihGGju7DW", "SM_House_2_L1", KIT_SCALE, "Village", None),
    ("k6tP5nFUd2", "SM_House_2_L2", KIT_SCALE, "Village", None),
    ("gEjAC1UvVU", "SM_House_2_L3", KIT_SCALE, "Village", None),
    ("YlADpCJU8U", "SM_House_3_L1", KIT_SCALE, "Village", None),
    ("vCoDG5hFyI", "SM_House_3_L3", KIT_SCALE, "Village", None),
    ("I0IzdnH2iN", "SM_Hut", KIT_SCALE, "Village", None),
    ("fkTsDdpQAA", "SM_StorageShed", KIT_SCALE, "Village", None),
    ("oYtDty0fR6", "SM_PineTrees", KIT_SCALE, "Village", None),
    ("ZuRHRsKWoz", "SM_PathStraight", KIT_SCALE, "Village", None),
    ("8wDmuvrSya", "SM_RockPathSquareWide", 1.0, "RockPath", None),
]


def downloaded_collection():
    col = bpy.data.collections.get(COLLECTION_NAME)
    if col is None:
        col = bpy.data.collections.new(COLLECTION_NAME)
        bpy.context.scene.collection.children.link(col)
    for obj in list(col.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    # Orphans from an earlier run would make the M_* names collide (Blender adds .001).
    for mesh in [m for m in bpy.data.meshes if m.users == 0]:
        bpy.data.meshes.remove(mesh)
    for mat in [m for m in bpy.data.materials if m.users == 0]:
        bpy.data.materials.remove(mat)
    bpy.context.view_layer.active_layer_collection = bpy.context.view_layer.layer_collection.children[col.name]
    return col


def find_source(file_id):
    for name in os.listdir(SRC_DIR):
        if name.endswith(".glb") and file_id in name:
            return os.path.join(SRC_DIR, name)
    raise FileNotFoundError(f"No .glb with id {file_id} in {SRC_DIR}")


def import_asset(path):
    """Imports a glb and returns its mesh objects with transforms baked into the mesh data, unparented."""
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    meshes = [o for o in new if o.type == "MESH"]
    for obj in meshes:
        world = obj.matrix_world.copy()
        obj.parent = None
        obj.data.transform(world)
        obj.matrix_world = Matrix.Identity(4)
    for obj in new:
        if obj.type != "MESH":
            bpy.data.objects.remove(obj, do_unlink=True)
    return meshes


def material_key(mat):
    bsdf = next((n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED"), None) if mat.node_tree else None
    if bsdf is None:
        return (None, None)
    base = bsdf.inputs["Base Color"]
    image = next((l.from_node.image.name for l in base.links if l.from_node.type == "TEX_IMAGE" and l.from_node.image), None)
    return (tuple(round(c, 3) for c in base.default_value[:3]), image)


def merge_materials(meshes, kit, canon):
    """Points every slot at one material per (kit, name, look), named M_<Kit>_<Name>[_n]."""
    for obj in meshes:
        for slot in obj.material_slots:
            mat = slot.material
            if mat is None or any(mat is m for m in canon.values()):
                continue
            base_name = mat.name.split(".")[0]
            key = (kit, base_name, material_key(mat))
            if key not in canon:
                taken = {m.name for m in canon.values()}
                name = f"M_{kit}_{base_name}"
                n = 2
                while name in taken:
                    name = f"M_{kit}_{base_name}_{n}"
                    n += 1
                mat.name = name
                canon[key] = mat
            slot.material = canon[key]


def bounds(objs):
    pts = [v.co for o in objs for v in o.data.vertices]
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    return lo, hi


def export_fbx(obj, path):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(
        filepath=path, use_selection=True, object_types={"MESH"},
        use_mesh_modifiers=True, mesh_smooth_type="FACE",
        apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
        use_custom_props=False, bake_anim=False, path_mode="COPY", embed_textures=True)


def process(file_id, out_name, scale, kit, parts, canon):
    meshes = import_asset(find_source(file_id))
    merge_materials(meshes, kit, canon)
    lo, hi = bounds(meshes)
    pivot = Vector(((lo.x + hi.x) * 0.5, (lo.y + hi.y) * 0.5, lo.z))
    to_origin = Matrix.Scale(scale, 4) @ Matrix.Translation(-pivot)
    for obj in meshes:
        obj.data.transform(to_origin)

    if parts is None:
        if len(meshes) > 1:
            bpy.ops.object.select_all(action="DESELECT")
            for obj in meshes:
                obj.select_set(True)
            bpy.context.view_layer.objects.active = meshes[0]
            bpy.ops.object.join()
        outputs = [(meshes[0], out_name)]
    else:
        by_node = {o.name.split(".")[0]: o for o in meshes}
        outputs = [(by_node[node], out_name + suffix) for node, suffix in parts.items()]

    result = []
    for obj, name in outputs:
        obj.name = name
        obj.data.name = name
        export_fbx(obj, os.path.join(EXPORT_DIR, name + ".fbx"))
        lo, hi = bounds([obj])
        result.append((name, tuple(round(v, 2) for v in hi - lo), len(obj.data.polygons)))
    return result


def main():
    os.makedirs(EXPORT_DIR, exist_ok=True)
    downloaded_collection()
    x = 0.0
    canon = {}
    for file_id, out_name, scale, kit, parts in ASSETS:
        width = 0.0
        for name, size, faces in process(file_id, out_name, scale, kit, parts, canon):
            bpy.data.objects[name].location.x = x  # spread out for viewing only; exports are at the origin
            width = max(width, size[0])
            print(f"{name}: size {size} m, {faces} faces")
        x += width + 2.0
    print("Materials:", sorted(m.name for m in canon.values()))
    bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH, copy=True)
    print("Saved", BLEND_PATH)


if __name__ == "__main__":
    main()
