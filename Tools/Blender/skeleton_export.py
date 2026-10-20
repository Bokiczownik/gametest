"""Export the Quaternius skeleton character (SourceArt/Characters/Skeleton/skeleton_anims.blend) to FBX for UE.

Run inside Blender via blender-mcp with the .blend open:
    exec(compile(open(P).read(), P, "exec"), {"__file__": P, "__name__": "__main__"})

Why the extra steps:
- The armature object carries the glTF import transform (scale 100, rotated 90 deg). Exported as is, UE gets a
  root bone scaled 10000 and bone translations of ~0.001, which animation compression flattens (feet stay on the
  ground and stretch). The export copy has rotation/scale applied and pose locations multiplied by that scale.
- Quaternius actions only key some bones; the rest must take the glTF default pose (read from the .glb with its
  animations stripped), otherwise they bake whatever pose was left in the scene.
- The weapon is bone-parented in the .blend; UE turns that into a bone with a broken bind pose, so the export
  joins a copy of it into the body mesh, fully weighted to its parent bone.
"""
import json
import os
import struct

import bpy

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
GLB = os.path.join(ROOT, "SourceArt", "Downloaded", "Skeleton by Quaternius - yq5ATpujSt.glb")
OUT = os.path.join(ROOT, "SourceArt", "Characters", "Skeleton", "Export", "SK_Skeleton.fbx")
SRC_ARM = "CharacterArmature"
SRC_MESH = "Skeleton"
WEAPON = "Weapon_Dagger"
TMP_SCENE = "SKELETON_EXPORT_TMP"


def action_bones(act):
    return act.layers[0].strips[0].channelbags[0].fcurves if act.layers else []


def read_default_pose(arm):
    """Pose basis of every bone in the glTF default pose (no animation applied)."""
    data = open(GLB, "rb").read()
    json_len = struct.unpack("<I", data[12:16])[0]
    gltf = json.loads(data[20:20 + json_len])
    rest = data[20 + json_len:]
    gltf.pop("animations", None)
    js = json.dumps(gltf).encode()
    js += b" " * ((4 - len(js) % 4) % 4)
    glb = struct.pack("<III", 0x46546C67, 2, 20 + len(js) + len(rest)) + struct.pack("<II", len(js), 0x4E4F534A) + js + rest
    tmp_path = os.path.join(bpy.app.tempdir, "skeleton_noanim.glb")
    open(tmp_path, "wb").write(glb)

    scene = bpy.data.scenes.new("SKELETON_GLB_TMP")
    window_scene = bpy.context.window.scene
    bpy.context.window.scene = scene
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=tmp_path)
    new = [o for o in bpy.data.objects if o not in before]
    glb_arm = next(o for o in new if o.type == "ARMATURE")
    pose = {pb.name: (pb.location.copy(), pb.rotation_quaternion.copy(), pb.scale.copy()) for pb in glb_arm.pose.bones}
    bpy.context.window.scene = window_scene
    for o in new:
        bpy.data.objects.remove(o)
    bpy.data.scenes.remove(scene)
    os.remove(tmp_path)
    missing = [b.name for b in arm.pose.bones if b.name not in pose]
    if missing:
        raise RuntimeError(f"bones missing from glTF default pose: {missing}")
    return pose


def export_actions(arm):
    """Original Quaternius clips (the armature's NLA tracks from the glTF import) plus our Sword_* actions."""
    acts = []
    for track in arm.animation_data.nla_tracks:
        for strip in track.strips:
            if strip.action:
                acts.append((strip.action.name.split("|")[3], strip.action))
    acts += [(act.name, act) for act in bpy.data.actions if act.name.startswith("Sword_")]
    return acts


def bake(arm, act, default_pose, scene):
    arm.animation_data.action = act
    start, end = (int(round(v)) for v in act.frame_range)
    frames = {}
    for f in range(start, end + 1):
        for pb in arm.pose.bones:
            loc, rot, scl = default_pose[pb.name]
            pb.location, pb.rotation_quaternion, pb.scale = loc, rot, scl
        scene.frame_set(f)
        frames[f] = {pb.name: (pb.location.copy(), pb.rotation_quaternion.copy(), pb.scale.copy()) for pb in arm.pose.bones}
    return frames


def main():
    scene = bpy.context.window.scene
    src_arm = bpy.data.objects[SRC_ARM]
    src_mesh = bpy.data.objects[SRC_MESH]
    weapon = bpy.data.objects[WEAPON]
    saved_action = src_arm.animation_data.action
    saved_frame = scene.frame_current
    unit = src_arm.matrix_world.to_scale()[0]

    default_pose = read_default_pose(src_arm)
    baked = [(name, bake(src_arm, act, default_pose, scene)) for name, act in export_actions(src_arm)]

    # Weapon in world space at rest, so it can be joined into the mesh.
    src_arm.data.pose_position = "REST"
    bpy.context.view_layer.update()
    weapon_world = weapon.matrix_world.copy()
    weapon_bone = weapon.parent_bone
    src_arm.data.pose_position = "POSE"
    src_arm.animation_data.action = saved_action
    scene.frame_set(saved_frame)

    tmp = bpy.data.scenes.new(TMP_SCENE)
    bpy.context.window.scene = tmp
    arm = src_arm.copy()
    arm.data = src_arm.data.copy()
    arm.animation_data_clear()
    arm.name = "SkeletonRig"
    arm.parent = None
    tmp.collection.objects.link(arm)
    arm.matrix_world = src_arm.matrix_world.copy()

    mesh = src_mesh.copy()
    mesh.data = src_mesh.data.copy()
    mesh.parent = None
    tmp.collection.objects.link(mesh)
    mesh.matrix_world = src_mesh.matrix_world.copy()
    blade = weapon.copy()
    blade.data = weapon.data.copy()
    blade.parent = None
    tmp.collection.objects.link(blade)
    blade.matrix_world = weapon_world
    blade.data.transform(mesh.matrix_world.inverted() @ blade.matrix_world)
    blade.matrix_world = mesh.matrix_world.copy()
    blade.vertex_groups.new(name=weapon_bone).add(list(range(len(blade.data.vertices))), 1.0, "REPLACE")
    for mod in mesh.modifiers:
        if mod.type == "ARMATURE":
            mod.object = arm

    bpy.ops.object.select_all(action="DESELECT")
    mesh.select_set(True)
    blade.select_set(True)
    bpy.context.view_layer.objects.active = mesh
    bpy.ops.object.join()
    mesh.name = SRC_MESH

    bpy.ops.object.select_all(action="DESELECT")
    arm.select_set(True)
    mesh.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    mesh.parent = arm

    arm.animation_data_create()
    new_actions = []
    for name, frames in baked:
        act = bpy.data.actions.new(name)
        new_actions.append(act)
        arm.animation_data.action = act
        for f, pose in frames.items():
            for pb in arm.pose.bones:
                loc, rot, scl = pose[pb.name]
                pb.location = loc * unit
                pb.rotation_quaternion = rot
                pb.scale = scl
                for prop in ("location", "rotation_quaternion", "scale"):
                    pb.keyframe_insert(prop, frame=f, group=pb.name)
        arm.animation_data.action = None
        track = arm.animation_data.nla_tracks.new()
        track.name = name
        track.strips.new(name, min(frames), act)

    bpy.ops.export_scene.fbx(
        filepath=OUT, use_selection=True, object_types={"ARMATURE", "MESH"}, add_leaf_bones=False,
        bake_anim=True, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=True,
        bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0.0, apply_unit_scale=True,
        mesh_smooth_type="FACE")

    bpy.context.window.scene = scene
    for act in new_actions:
        bpy.data.actions.remove(act)
    arm_data, mesh_data = arm.data, mesh.data
    bpy.data.objects.remove(mesh)
    bpy.data.objects.remove(arm)
    bpy.data.meshes.remove(mesh_data)
    bpy.data.armatures.remove(arm_data)
    bpy.data.scenes.remove(tmp)
    print(f"exported {len(baked)} actions to {OUT}")


if __name__ == "__main__":
    main()
