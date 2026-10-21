"""Builds the shield skeleton's block / shield bash clips and the knockdown clip in skeleton_anims.blend.

Run inside Blender via blender-mcp with the .blend open (then save it and run skeleton_export.py):
    exec(compile(open(P).read(), P, "exec"), {"__file__": P, "__name__": "__main__"})

All three are full-body clips: ABP_Skeleton drops its shield arm layer while AEnemyCharacter reports a full-body
animation (block, shield bash, knockdown), so the left arm comes from these clips.
- Shield_Block (0-24, loops): Sword_IdleReady with the Shield_Guard arm, shield squared in front, torso leaning in.
- Shield_Bash (0-22): block pose -> windup (torso turned, shield pulled back, 5) -> thrust, impact frame 9 (0.375 s)
  -> combat pose (Sword_IdleReady frame 0 + Shield_Guard arm) at 22.
- KnockDown (0-40): Death 0-18, lies still until 22, then Death backwards to its first frame (stands up) at 40.
  Only the bones Death keys are keyed; skeleton_export.py puts the rest in the glTF default pose.
Offsets are rotations in world space (character faces -Y, its left side is +X) about the bone's head, or (bone, MOVE,
world offset in m), applied in order. Moving Hips shifts the upper body only (legs hang from Body, feet from Root).
"""
import math

import bpy
from mathutils import Matrix, Vector

ARM = "CharacterArmature"
READY = "Sword_IdleReady"
GUARD = "Shield_Guard"
DEATH_TAG = "|Death|"
MOVE = "move"

BLOCK = [("Abdomen", (1, 0, 0), 12), ("UpperArm.L", (0, 0, 1), -25)]
WINDUP = [("Hips", MOVE, (0, 0.04, 0)), ("Abdomen", (1, 0, 0), -4), ("Abdomen", (0, 0, 1), 25),
          ("UpperArm.L", (0, 0, 1), -10), ("UpperArm.L", (1, 0, 0), 15)]
STRIKE = [("Hips", MOVE, (0, -0.12, -0.02)), ("Abdomen", (1, 0, 0), 20), ("Abdomen", (0, 0, 1), -20),
          ("UpperArm.L", (0, 0, 1), -25), ("UpperArm.L", (1, 0, 0), -35)]
# (frame, offsets on the combat pose)
BASH_KEYS = [(0, BLOCK), (5, WINDUP), (9, STRIKE), (13, STRIKE), (22, [])]
KNOCKDOWN_FALL_END = 18
KNOCKDOWN_LENGTH = 40


def keyed_bones(act):
    return {fc.data_path.split('"')[1] for fc in act.layers[0].strips[0].channelbags[0].fcurves}


def sample(arm, act, frame):
    arm.animation_data.action = act
    bpy.context.scene.frame_set(frame)
    return {pb.name: (pb.location.copy(), pb.rotation_quaternion.copy(), pb.scale.copy()) for pb in arm.pose.bones}


def set_pose(arm, pose):
    for name, (loc, rot, scl) in pose.items():
        pb = arm.pose.bones[name]
        pb.location, pb.rotation_quaternion, pb.scale = loc, rot, scl
    bpy.context.view_layer.update()


def offset_pose(arm, base, offsets):
    """Base pose with world-space rotations applied (the armature has no action while this runs)."""
    set_pose(arm, base)
    to_arm = arm.matrix_world.to_3x3().normalized().inverted()
    for bone, axis, amount in offsets:
        pb = arm.pose.bones[bone]
        if axis == MOVE:
            pb.matrix = Matrix.Translation(arm.matrix_world.inverted().to_3x3() @ Vector(amount)) @ pb.matrix
        else:
            head = pb.matrix.translation.copy()
            rotation = Matrix.Rotation(math.radians(amount), 4, (to_arm @ Vector(axis)).normalized())
            pb.matrix = Matrix.Translation(head) @ rotation @ Matrix.Translation(-head) @ pb.matrix
        bpy.context.view_layer.update()
    return {pb.name: (pb.location.copy(), pb.rotation_quaternion.copy(), pb.scale.copy()) for pb in arm.pose.bones}


def write_action(arm, name, keys, bones=None):
    """keys: frame -> pose; keys every bone (or only `bones`)."""
    old = bpy.data.actions.get(name)
    if old:
        bpy.data.actions.remove(old)
    act = bpy.data.actions.new(name)
    act.use_fake_user = True
    arm.animation_data.action = act
    for frame, pose in sorted(keys.items()):
        set_pose(arm, pose)
        for bone in (bones or pose.keys()):
            pb = arm.pose.bones[bone]
            for prop in ("location", "rotation_quaternion", "scale"):
                pb.keyframe_insert(prop, frame=frame, group=bone)
    arm.animation_data.action = None
    return act


def combat_pose(arm, ready_frame, guard_pose, guard_bones):
    pose = sample(arm, bpy.data.actions[READY], ready_frame)
    pose.update({bone: guard_pose[bone] for bone in guard_bones})
    return pose


def main():
    arm = bpy.data.objects[ARM]
    scene = bpy.context.scene
    saved_action, saved_frame = arm.animation_data.action, scene.frame_current

    guard_bones = keyed_bones(bpy.data.actions[GUARD])
    guard_pose = sample(arm, bpy.data.actions[GUARD], 0)
    ready_start, ready_end = (int(round(v)) for v in bpy.data.actions[READY].frame_range)

    bases = {f: combat_pose(arm, f, guard_pose, guard_bones) for f in range(ready_start, ready_end + 1)}
    arm.animation_data.action = None
    block = {f: offset_pose(arm, base, BLOCK) for f, base in bases.items()}
    bash = {f: offset_pose(arm, bases[ready_start], offsets) for f, offsets in BASH_KEYS}
    write_action(arm, "Shield_Block", block)
    write_action(arm, "Shield_Bash", bash)

    death = next(a for a in bpy.data.actions if DEATH_TAG in a.name)
    death_bones = keyed_bones(death)
    knockdown = {}
    for f in range(0, KNOCKDOWN_FALL_END + 1):
        pose = sample(arm, death, f)
        knockdown[f] = pose
        knockdown[KNOCKDOWN_LENGTH - f] = pose
    arm.animation_data.action = None
    write_action(arm, "KnockDown", knockdown, sorted(death_bones))

    arm.animation_data.action = saved_action
    scene.frame_set(saved_frame)
    print("built Shield_Block, Shield_Bash, KnockDown")


if __name__ == "__main__":
    main()
