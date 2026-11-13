# -*- coding: utf-8 -*-
"""C4・触手の攻撃（No.139）、ガード・パリィ（No.140）、横走り・空中の技（No.141）のクリップを
MekaGirl のリグに作り、FBX に書き出す。

使い方:
  mayapy tools/maya/build_more_clips.py <mekagirl_motion.mb> build
  mayapy tools/maya/build_more_clips.py <mekagirl_motion.mb> export <assets/models>
  （Maya の画面から: import build_more_clips as m; m.build(); m.export('<assets/models>')）

姿勢の書き方は build_weapon_clips.py と同じ（+Z が前、-X がキャラの右、+Y が上）。
右手（右へ動く）として書き、左手（左へ動く）は左右反転して作る。
クリップの名前・フレーム・区切りの約束は docs/plan/07_アニメの追加/139〜141 の「クリップの約束」。
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build_weapon_clips as w  # noqa: E402
import build_reaction_clips as r  # noqa: E402  （姿勢の作り方 rpose と足の置き方を使う）
import maya.cmds as cmds  # noqa: E402
import maya.mel as mel  # noqa: E402

w.STANCES.update({
    # 横走り（右へ）：着いている足は左へすべり、浮いた足は右へ運ぶ
    'sr0': ((-14, 0, 0), (14, 0, 0)),
    'sr4': ((-8, 0, 0), (8, 9, 0)),
    'sr8': ((-2, 0, 0), (2, 0, 0)),
    'sr12': ((-8, 9, 0), (8, 0, 0)),
    'tuck': ((-3, 30, 10), (3, 28, 6)),  # 空中で膝を抱える
    'tuck_half': ((-2, 16, 6), (2, 20, -4)),
    'straight': ((0, 0, 0), (0, 0, 0)),  # 脚を伸ばす（腰を上げて使う）
    'trail': ((-1, 12, -30), (1, 20, -40)),  # 脚を後ろへ流す
})

DOWN = r.DOWN_R
DOWN_L = r.DOWN_L
HOLD = (-0.1, 0.4, 0.91)  # ハンマーの持ち方
GUARD = (0.08, 0.55, 0.83)  # 顔の前に立てた腕
GUARD_HAND = (0.1, 0.98, 0.15)


def P(torso=(0, 0, 0), hips=(0, 0, 0), ty=0.0, tz=0.0, stance=None, arm=DOWN, hand=None, off=DOWN_L, head=(0, 0, 0), root=None):
    """右手の攻撃の姿勢（hand は手先の向き。root は体ごと回す度数）。"""
    p = r.rpose(torso=torso, hips=hips, ty=ty, tz=tz, stance=stance, arm_r=arm, arm_l=off, head=head)
    p['weapon'] = hand
    if root is not None:
        p['joints']['root'] = root
    return p


# name, ファイル名（%s に r / l）, 右の開始, 左の開始, [(フレーム, 姿勢, 'lin')]
CLIPS = [
    # --- No.139 C4・触手 ---
    # C4 の単押し：引いて 0-3・前へ放る 3-6・残す 6-8
    ('c4_throw', 'mekagirl_atk_c4_throw_%s', 3401, 3501, [
        (0, P()),
        (3, P(torso=(4, -16, 2), arm=(-0.32, -0.74, -0.59), hand=(-0.3, -0.6, -0.74))),
        (6, P(torso=(6, 14, -2), arm=(-0.14, 0.12, 0.98), hand=(-0.1, 0.4, 0.91)), 'lin'),
        (8, P(torso=(4, 10, -2), arm=(-0.16, -0.12, 0.98), hand=(-0.12, 0.1, 0.99))),
    ]),
    # C4 の長押し：頭の上へ構えて反る（ため）0-24・投げ下ろす 24-32
    ('c4_charge', 'mekagirl_atk_c4_charge_%s', 3601, 3701, [
        (0, P()),
        (10, P(torso=(-18, -10, 0), arm=(-0.1, 0.95, -0.3), hand=(-0.05, 0.6, -0.8), off=(0.5, 0.2, 0.84), head=(-8, 0, 0))),
        (24, P(torso=(-24, -14, 0), arm=(-0.08, 0.9, -0.43), hand=(-0.04, 0.5, -0.86), off=(0.55, 0.25, 0.8), head=(-10, 0, 0))),
        (27, P(torso=(12, 6, 0), arm=(-0.1, 0.42, 0.9), hand=(-0.06, 0.7, 0.71), off=(0.4, -0.5, 0.77)), 'lin'),
        (29, P(torso=(30, 12, -2), arm=(-0.1, -0.5, 0.86), hand=(-0.06, -0.3, 0.95), off=(0.3, -0.85, -0.42)), 'lin'),
        (32, P(torso=(24, 8, -2), arm=(-0.16, -0.62, 0.77), hand=(-0.1, -0.5, 0.86), off=(0.3, -0.88, -0.36))),
    ]),
    # 触手を伸ばす：肩を前へ突き出す 0-6（最後の姿勢のまま）
    ('tentacle_reach', 'mekagirl_atk_tentacle_reach_%s', 3801, 3901, [
        (0, P()),
        (6, P(torso=(8, 20, -2), arm=(-0.08, 0.06, 1.0), off=(0.35, -0.86, -0.36))),
    ]),
    # 触手で投げる：つかんで横へ引きつけ振りかぶる 0-8・反対の前へ振り抜く 8-16
    ('tentacle_throw', 'mekagirl_atk_tentacle_throw_%s', 4001, 4101, [
        (0, P(torso=(8, 20, -2), arm=(-0.08, 0.06, 1.0), off=(0.35, -0.86, -0.36))),
        (8, P(torso=(-4, -32, 4), hips=(0, -10, 0), arm=(-0.92, 0.12, -0.36), off=(0.5, -0.7, 0.5))),
        (12, P(torso=(6, 4, 0), arm=(-0.32, 0.12, 0.94), off=(0.45, -0.8, 0.2)), 'lin'),
        (16, P(torso=(10, 36, -4), hips=(0, 12, 0), arm=(0.62, 0.0, 0.78), off=(0.3, -0.7, -0.64)), 'lin'),
    ]),
    # 触手の長押し：腕の側へひねって溜め 0-6・その場で1回転 6-16・戻る 16-21（全身）
    ('tentacle_spin', 'mekagirl_atk_tentacle_spin_%s', 4201, 4301, [
        (0, P(root=(0, 0, 0))),
        (6, P(torso=(6, -40, 4), hips=(0, -16, 0), ty=-6, stance='low', arm=(-0.94, 0.05, -0.34), off=(0.4, -0.8, 0.45), root=(0, 0, 0))),
        (8, P(torso=(6, -10, 4), hips=(0, -6, 0), ty=-7, stance='low', arm=(-0.98, 0.08, 0.18), off=(0.4, -0.85, 0.35), root=(0, 72, 0)), 'lin'),
        (10, P(torso=(6, 0, 4), ty=-7, stance='low', arm=(-0.98, 0.08, 0.18), off=(0.4, -0.85, 0.35), root=(0, 144, 0)), 'lin'),
        (12, P(torso=(6, 4, 4), ty=-7, stance='low', arm=(-0.98, 0.08, 0.18), off=(0.4, -0.85, 0.35), root=(0, 216, 0)), 'lin'),
        (14, P(torso=(6, 8, 4), ty=-7, stance='low', arm=(-0.98, 0.08, 0.18), off=(0.4, -0.85, 0.35), root=(0, 288, 0)), 'lin'),
        (16, P(torso=(6, 12, 2), ty=-6, stance='low', arm=(-0.9, 0.0, 0.43), off=(0.4, -0.85, 0.35), root=(0, 360, 0)), 'lin'),
        (21, P(torso=(2, 0, 0), ty=-2, stance='low', root=(0, 360, 0))),
    ]),
    # --- No.140 ガード・パリィ ---
    # 構える 0-4（最後の姿勢のまま構え続ける）
    ('guard_raise', 'mekagirl_guard_raise_%s', 4401, 4501, [
        (0, P()),
        (4, P(torso=(10, 8, 0), arm=GUARD, hand=GUARD_HAND, head=(4, 0, 0))),
    ]),
    # 受ける：押される 0-2・戻る 2-8
    ('guard_hit', 'mekagirl_guard_hit_%s', 4601, 4701, [
        (0, P(torso=(10, 8, 0), arm=GUARD, hand=GUARD_HAND, head=(4, 0, 0))),
        (2, P(torso=(-10, 4, 2), arm=(0.04, 0.68, 0.73), hand=(0.08, 0.96, -0.26), head=(-10, 0, 0)), 'lin'),
        (8, P(torso=(10, 8, 0), arm=GUARD, hand=GUARD_HAND, head=(4, 0, 0))),
    ]),
    # パリィ：外へ払う 0-3・胸を張る 3-7・解く 7-12
    ('guard_parry', 'mekagirl_guard_parry_%s', 4801, 4901, [
        (0, P(torso=(10, 8, 0), arm=GUARD, hand=GUARD_HAND, head=(4, 0, 0))),
        (3, P(torso=(-6, -16, 2), arm=(-0.86, 0.32, 0.4), hand=(-0.7, 0.7, -0.1), off=(0.6, -0.5, -0.62), head=(-6, 0, 0)), 'lin'),
        (7, P(torso=(-10, -18, 2), arm=(-0.88, 0.26, 0.4), hand=(-0.72, 0.66, -0.2), off=(0.62, -0.5, -0.6), head=(-8, 0, 0))),
        (12, P(torso=(2, 0, 0), arm=(-0.4, -0.8, 0.45), off=(0.35, -0.86, 0.36))),
    ]),
    # --- No.141 横走り・空中の技 ---
    # 横走り（右へ。左は反転）：ループ 0-16
    ('strafe', 'mekagirl_strafe_%s', 5101, 5001, [
        (0, P(torso=(4, 0, 6), ty=-4, stance='sr0', arm=(-0.42, -0.88, 0.2), off=(0.3, -0.9, 0.3))),
        (4, P(torso=(4, 0, 4), ty=-1, stance='sr4', arm=(-0.36, -0.9, 0.24), off=(0.38, -0.88, 0.26))),
        (8, P(torso=(4, 0, 6), ty=-3, stance='sr8', arm=(-0.3, -0.9, 0.3), off=(0.42, -0.88, 0.2))),
        (12, P(torso=(4, 0, 4), ty=-1, stance='sr12', arm=(-0.36, -0.9, 0.24), off=(0.38, -0.88, 0.26))),
        (16, P(torso=(4, 0, 6), ty=-4, stance='sr0', arm=(-0.42, -0.88, 0.2), off=(0.3, -0.9, 0.3))),
    ]),
    # ハンマーの空中の跳ね：膝を抱えて振りかぶる 0-7・足元へ叩く 7-10・伸び上がる 10-20（全身）
    ('hammer_hop', 'mekagirl_atk_hammer_hop_%s', 5201, 5301, [
        (0, P(arm=DOWN, hand=HOLD)),
        (7, P(torso=(-20, 6, 0), stance='tuck', arm=(-0.14, 0.96, -0.24), hand=(0.02, 0.42, -0.91), off=(0.5, 0.3, 0.8))),
        (8, P(torso=(10, 2, 0), stance='tuck', arm=(-0.1, 0.42, 0.9), hand=(-0.04, 0.8, 0.6), off=(0.5, -0.2, 0.84)), 'lin'),
        (10, P(torso=(42, 0, 0), ty=4, stance='straight', arm=(-0.08, -0.86, 0.5), hand=(-0.02, -0.98, 0.2), off=(0.45, -0.6, 0.66)), 'lin'),
        (14, P(torso=(30, 0, 0), ty=4, stance='straight', arm=(-0.1, -0.84, 0.53), hand=(-0.03, -0.97, 0.24), off=(0.45, -0.7, 0.55))),
        (20, P(torso=(-6, 0, 0), ty=2, stance='straight', arm=DOWN, hand=HOLD, head=(-6, 0, 0))),
    ]),
    # ナックルの空中突進：拳を引いてため 0-10・斜め上へ突き出して飛ぶ 10-18・戻る 18-26（全身）
    ('knuckle_airrush', 'mekagirl_atk_knuckle_airrush_%s', 5401, 5501, [
        (0, P()),
        (6, P(torso=(10, 26, -4), stance='tuck_half', arm=(-0.5, -0.12, -0.86), off=(0.6, 0.1, 0.79))),
        (10, P(torso=(14, 32, -6), stance='tuck_half', arm=(-0.54, -0.08, -0.84), off=(0.62, 0.12, 0.77))),
        (12, P(torso=(-22, -18, 2), hips=(-8, -6, 0), stance='trail', arm=(-0.04, 0.64, 0.77), off=(0.36, -0.5, -0.79), head=(10, 0, 0)), 'lin'),
        (18, P(torso=(-24, -22, 2), hips=(-10, -8, 0), stance='trail', arm=(0.0, 0.66, 0.75), off=(0.38, -0.5, -0.78), head=(12, 0, 0))),
        (22, P(torso=(8, -6, 0), stance='tuck_half', arm=(-0.1, -0.3, 0.95))),
        (26, P(torso=(4, 0, 0))),
    ]),
]

MOVED = set(r.MOVED) | {'root'}


def build(path=None):
    """クリップのキーを打って保存する（path を省くと、開いているシーンに作る）。"""
    if path:
        cmds.file(path, open=True, force=True, executeScriptNodes=False, prompt=False)
    joints = cmds.ls(type='joint')
    for _, _, right, left, keys in CLIPS:
        for start in (right, left):
            cmds.cutKey(joints, time=(start, start + 99), clear=True)
    idle = {j: tuple(cmds.getAttr(j + '.rotate', time=1)[0]) for j in joints}
    pelvis_t = tuple(cmds.getAttr('pelvis.translate', time=1)[0])
    cmds.currentTime(1)
    w.RIG['t'] = {j: tuple(cmds.getAttr(j + '.translate', time=1)[0]) for j in joints}
    w.RIG['ankle'] = {j: tuple(cmds.xform(j, q=True, ws=True, t=True)) for j in ('foot_r', 'foot_l')}
    for name, _, right, left, keys in CLIPS:
        frames = w.sample_clip(keys, idle, pelvis_t)
        w.write_clip(frames, right, False, idle, MOVED)
        w.write_clip(frames, left, True, idle, MOVED)
        print('built', name, right, left, keys[-1][0])
    cmds.file(save=True, type='mayaBinary', force=True)
    print('saved', cmds.file(q=True, sceneName=True))


def export(out_dir, path=None):
    """クリップを1つずつ FBX に書き出す（書き出したあと、元のシーンを開き直す）。"""
    path = path or cmds.file(q=True, sceneName=True)
    cmds.loadPlugin('fbxmaya', quiet=True)
    for name, pattern, right, left, keys in CLIPS:
        length = keys[-1][0] - keys[0][0]
        for side, start in (('r', right), ('l', left)):
            cmds.file(path, open=True, force=True, executeScriptNodes=False, prompt=False)
            joints = cmds.ls(type='joint')
            cmds.cutKey(joints, time=(-100000, start - 1), clear=True)
            cmds.cutKey(joints, time=(start + length + 1, 100000), clear=True)
            cmds.keyframe(joints, edit=True, relative=True, timeChange=-start)
            cmds.currentTime(0)
            cmds.playbackOptions(min=0, max=length, animationStartTime=0, animationEndTime=length)
            cmds.select('root', hierarchy=True, replace=True)
            for command in ('FBXResetExport', 'FBXExportInAscii -v false', 'FBXExportFileVersion -v FBX201800',
                            'FBXExportUpAxis y', 'FBXExportBakeComplexAnimation -v true',
                            'FBXExportBakeComplexStart -v 0', 'FBXExportBakeComplexEnd -v %d' % length,
                            'FBXExportBakeComplexStep -v 1', 'FBXExportBakeResampleAnimation -v false',
                            'FBXExportSkins -v false', 'FBXExportShapes -v false', 'FBXExportConstraints -v false',
                            'FBXExportCameras -v false', 'FBXExportLights -v false', 'FBXExportInputConnections -v false'):
                mel.eval(command)
            out = '%s/%s.fbx' % (out_dir.replace('\\', '/'), pattern % side)
            mel.eval('FBXExport -f "%s" -s' % out)
            print('exported', out)
    cmds.file(path, open=True, force=True, executeScriptNodes=False, prompt=False)


if __name__ == '__main__':
    import maya.standalone
    maya.standalone.initialize(name='python')
    try:
        if sys.argv[2] == 'build':
            build(sys.argv[1])
        else:
            export(sys.argv[3], sys.argv[1])
    finally:
        maya.standalone.uninitialize()
