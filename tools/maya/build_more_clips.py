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
    # 横走り（右へ）：着いている足は左へすべり、浮いた足を高く右へ運ぶ
    'sr0': ((-26, 0, 0), (24, 0, 0)),
    'sr4': ((-14, 0, 0), (12, 16, 0)),
    'sr8': ((-2, 0, 0), (2, 0, 0)),
    'sr12': ((-14, 16, 0), (12, 0, 0)),
    'tuck': ((-3, 42, 16), (3, 40, 10)),  # 空中で膝を胸まで抱える
    'tuck_half': ((-2, 22, 10), (2, 28, -6)),
    'straight': ((0, 0, 0), (0, 0, 0)),  # 脚を伸ばす（腰を上げて使う）
    'trail': ((-1, 18, -40), (1, 28, -52)),  # 脚を後ろへ流す
    'wide': ((-14, 0, 6), (14, 0, -6)),  # 足を大きく開いて踏ん張る
})

DOWN = r.DOWN_R
DOWN_L = r.DOWN_L
HOLD = (-0.1, 0.4, 0.91)  # ハンマーの持ち方
GUARD = (0.1, 0.55, 0.83)  # 顔の前に立てた腕
GUARD_HAND = (0.12, 0.98, 0.15)


def P(torso=(0, 0, 0), hips=(0, 0, 0), ty=0.0, tz=0.0, stance=None, arm=DOWN, hand=None, off=DOWN_L, head=(0, 0, 0), root=None):
    """右手の攻撃の姿勢（hand は手先の向き。root は体ごと回す度数）。"""
    p = r.rpose(torso=torso, hips=hips, ty=ty, tz=tz, stance=stance, arm_r=arm, arm_l=off, head=head)
    p['weapon'] = hand
    if root is not None:
        p['joints']['root'] = root
    return p


GUARD_POSE = dict(torso=(18, 14, 0), arm=GUARD, hand=GUARD_HAND, off=(0.4, -0.5, 0.77), head=(10, 0, 0))
SPIN_ARM = (-0.99, 0.1, 0.1)  # 回転中に横へ伸ばした腕
REACH = dict(torso=(16, 34, -6), hips=(0, 10, 0), arm=(-0.04, 0.12, 0.99), off=(0.45, -0.6, -0.66), head=(-6, -14, 0))

# name, ファイル名（%s に r / l）, 右の開始, 左の開始, [(フレーム, 姿勢, 'lin')]
# 大きく・速く見せるため、振る瞬間は 'lin' で一気に動かし、振り切ってから少し戻す。
CLIPS = [
    # --- No.139 C4・触手 ---
    # C4 の単押し：大きく引いて 0-3・一気に放る 3-5・振り切る 5-8
    ('c4_throw', 'mekagirl_atk_c4_throw_%s', 3401, 3501, [
        (0, P()),
        (3, P(torso=(6, -34, 6), hips=(0, -12, 0), arm=(-0.46, -0.52, -0.72), hand=(-0.4, -0.4, -0.82), off=(0.6, 0.0, 0.8), head=(0, 12, 0))),
        (5, P(torso=(16, 24, -4), hips=(0, 8, 0), arm=(-0.12, 0.35, 0.93), hand=(-0.1, 0.7, 0.7), off=(0.5, -0.6, -0.62)), 'lin'),
        (8, P(torso=(20, 32, -6), hips=(0, 10, 0), arm=(0.08, -0.08, 0.99), hand=(0.05, 0.2, 0.98), off=(0.45, -0.7, -0.55))),
    ]),
    # C4 の長押し：頭の上へ構えて大きく反る（ため）0-24・全身で投げ下ろす 24-32
    ('c4_charge', 'mekagirl_atk_c4_charge_%s', 3601, 3701, [
        (0, P()),
        (8, P(torso=(-30, -18, 0), arm=(-0.06, 0.9, -0.43), hand=(0.0, 0.4, -0.92), off=(0.6, 0.5, 0.62), head=(-12, 0, 0))),
        (24, P(torso=(-42, -26, -4), arm=(-0.04, 0.72, -0.69), hand=(0.0, 0.15, -0.99), off=(0.64, 0.56, 0.53), head=(-16, 0, 0))),
        (26, P(torso=(10, 8, 0), arm=(-0.08, 0.8, 0.6), hand=(-0.04, 0.95, 0.3), off=(0.5, -0.2, 0.84)), 'lin'),
        (28, P(torso=(50, 20, -4), arm=(-0.08, -0.62, 0.78), hand=(-0.05, -0.5, 0.86), off=(0.4, -0.7, -0.6), head=(10, 0, 0)), 'lin'),
        (32, P(torso=(42, 16, -4), arm=(-0.12, -0.76, 0.64), hand=(-0.08, -0.66, 0.75), off=(0.4, -0.75, -0.53), head=(8, 0, 0))),
    ]),
    # 触手を伸ばす：少し引いて 0-2・肩ごと前へ突き出す 2-6（最後の姿勢のまま）
    ('tentacle_reach', 'mekagirl_atk_tentacle_reach_%s', 3801, 3901, [
        (0, P()),
        (2, P(torso=(-6, -14, 2), arm=(-0.5, -0.4, -0.77), off=(0.4, -0.6, 0.69))),
        (4, P(torso=(18, 38, -6), hips=(0, 12, 0), arm=(-0.02, 0.14, 0.99), off=(0.45, -0.6, -0.66), head=(-6, -16, 0)), 'lin'),
        (6, P(**REACH)),
    ]),
    # 触手で投げる：つかんで横へ大きく引きつけ振りかぶる 0-8・反対の前へ一気に振り抜く 8-16
    ('tentacle_throw', 'mekagirl_atk_tentacle_throw_%s', 4001, 4101, [
        (0, P(**REACH)),
        (8, P(torso=(-10, -52, 8), hips=(0, -20, 0), arm=(-0.9, 0.3, -0.3), off=(0.6, -0.4, 0.69), head=(0, 22, 0))),
        (11, P(torso=(8, 0, 0), arm=(-0.4, 0.2, 0.9), off=(0.5, -0.6, 0.62)), 'lin'),
        (13, P(torso=(24, 56, -8), hips=(0, 22, 0), arm=(0.75, 0.05, 0.66), off=(0.2, -0.6, -0.77), head=(4, -12, 0)), 'lin'),
        (16, P(torso=(20, 50, -6), hips=(0, 20, 0), arm=(0.8, -0.15, 0.58), off=(0.25, -0.65, -0.72), head=(4, -10, 0))),
    ]),
    # 触手の長押し：腕の側へ深くひねって溜め 0-6・外へ傾きながら1回転 6-16・戻る 16-21（全身）
    ('tentacle_spin', 'mekagirl_atk_tentacle_spin_%s', 4201, 4301, [
        (0, P(root=(0, 0, 0))),
        (6, P(torso=(12, -60, 6), hips=(0, -24, 0), ty=-14, stance='wide', arm=(-0.9, 0.1, -0.42), off=(0.5, -0.6, 0.62), head=(0, 24, 0), root=(0, 0, 0))),
        (8, P(torso=(10, -10, -16), hips=(0, -6, 0), ty=-12, stance='wide', arm=SPIN_ARM, off=(0.5, -0.8, 0.33), root=(0, 72, 0)), 'lin'),
        (10, P(torso=(10, 0, -18), ty=-12, stance='wide', arm=SPIN_ARM, off=(0.5, -0.8, 0.33), root=(0, 144, 0)), 'lin'),
        (12, P(torso=(10, 4, -18), ty=-12, stance='wide', arm=SPIN_ARM, off=(0.5, -0.8, 0.33), root=(0, 216, 0)), 'lin'),
        (14, P(torso=(10, 8, -18), ty=-12, stance='wide', arm=SPIN_ARM, off=(0.5, -0.8, 0.33), root=(0, 288, 0)), 'lin'),
        (16, P(torso=(12, 20, -10), hips=(0, 6, 0), ty=-12, stance='wide', arm=(-0.8, 0.05, 0.6), off=(0.5, -0.8, 0.33), root=(0, 360, 0)), 'lin'),
        (21, P(torso=(4, 0, 0), ty=-4, stance='low', root=(0, 360, 0))),
    ]),
    # --- No.140 ガード・パリィ ---
    # 構える 0-4（一気に上げて、最後の姿勢のまま構え続ける）
    ('guard_raise', 'mekagirl_guard_raise_%s', 4401, 4501, [
        (0, P()),
        (2, P(torso=(22, 16, 0), arm=(0.12, 0.6, 0.79), hand=GUARD_HAND, off=(0.4, -0.5, 0.77), head=(12, 0, 0)), 'lin'),
        (4, P(**GUARD_POSE)),
    ]),
    # 受ける：大きく押される 0-1・こらえる 1-3・戻る 3-8
    ('guard_hit', 'mekagirl_guard_hit_%s', 4601, 4701, [
        (0, P(**GUARD_POSE)),
        (1, P(torso=(-22, 6, 6), arm=(0.0, 0.75, 0.66), hand=(0.05, 0.9, -0.43), off=(0.6, -0.2, 0.77), head=(-20, 0, 0)), 'lin'),
        (3, P(torso=(-16, 6, 4), arm=(0.02, 0.72, 0.69), hand=(0.06, 0.94, -0.33), off=(0.55, -0.3, 0.78), head=(-14, 0, 0))),
        (8, P(**GUARD_POSE)),
    ]),
    # パリィ：外へ一気に払う 0-2・胸を張って見せる 2-6・解く 6-12
    ('guard_parry', 'mekagirl_guard_parry_%s', 4801, 4901, [
        (0, P(**GUARD_POSE)),
        (2, P(torso=(-14, -30, 4), arm=(-0.95, 0.25, 0.18), hand=(-0.6, 0.8, -0.1), off=(0.7, -0.3, -0.65), head=(-12, -10, 0)), 'lin'),
        (6, P(torso=(-18, -34, 4), arm=(-0.96, 0.22, 0.16), hand=(-0.62, 0.76, -0.2), off=(0.72, -0.3, -0.63), head=(-14, -10, 0))),
        (12, P(torso=(4, -4, 0), arm=(-0.4, -0.8, 0.45), off=(0.35, -0.86, 0.36))),
    ]),
    # --- No.141 横走り・空中の技 ---
    # 横走り（右へ。左は反転）：ループ 0-16。進む側へ傾き、上下に弾む
    ('strafe', 'mekagirl_strafe_%s', 5101, 5001, [
        (0, P(torso=(8, 0, 12), hips=(0, 0, 6), ty=-8, stance='sr0', arm=(-0.6, -0.76, 0.24), off=(0.4, -0.8, 0.45))),
        (4, P(torso=(8, 0, 8), hips=(0, 0, 4), ty=-2, stance='sr4', arm=(-0.5, -0.8, 0.33), off=(0.5, -0.78, 0.37))),
        (8, P(torso=(8, 0, 12), hips=(0, 0, 6), ty=-7, stance='sr8', arm=(-0.4, -0.8, 0.45), off=(0.6, -0.76, 0.24))),
        (12, P(torso=(8, 0, 8), hips=(0, 0, 4), ty=-2, stance='sr12', arm=(-0.5, -0.8, 0.33), off=(0.5, -0.78, 0.37))),
        (16, P(torso=(8, 0, 12), hips=(0, 0, 6), ty=-8, stance='sr0', arm=(-0.6, -0.76, 0.24), off=(0.4, -0.8, 0.45))),
    ]),
    # ハンマーの空中の跳ね：膝を抱えて大きく振りかぶる 0-7・足元へ叩く 7-10・伸び上がる 10-20（全身）
    ('hammer_hop', 'mekagirl_atk_hammer_hop_%s', 5201, 5301, [
        (0, P(arm=DOWN, hand=HOLD)),
        (7, P(torso=(-34, 6, 0), hips=(-10, 0, 0), stance='tuck', arm=(-0.12, 0.9, -0.42), hand=(0.04, 0.2, -0.98), off=(0.6, 0.4, 0.69), head=(-14, 0, 0))),
        (8, P(torso=(16, 2, 0), stance='tuck', arm=(-0.1, 0.42, 0.9), hand=(-0.04, 0.8, 0.6), off=(0.5, -0.2, 0.84)), 'lin'),
        (10, P(torso=(62, 0, 0), hips=(12, 0, 0), ty=8, stance='straight', arm=(-0.06, -0.9, 0.43), hand=(-0.02, -0.99, 0.12), off=(0.45, -0.6, 0.66), head=(14, 0, 0)), 'lin'),
        (14, P(torso=(50, 0, 0), hips=(10, 0, 0), ty=8, stance='straight', arm=(-0.08, -0.88, 0.47), hand=(-0.03, -0.98, 0.2), off=(0.45, -0.7, 0.55), head=(10, 0, 0))),
        (20, P(torso=(-14, 0, 0), ty=4, stance='straight', arm=(-0.4, -0.7, -0.59), hand=HOLD, off=(0.4, -0.7, -0.59), head=(-10, 0, 0))),
    ]),
    # ナックルの空中突進：体を丸めて拳を深く引く 0-10・斜め上へ爆発的に突き出して飛ぶ 10-18・戻る 18-26（全身）
    ('knuckle_airrush', 'mekagirl_atk_knuckle_airrush_%s', 5401, 5501, [
        (0, P()),
        (6, P(torso=(16, 40, -6), hips=(4, 10, 0), stance='tuck_half', arm=(-0.58, -0.1, -0.81), off=(0.62, 0.14, 0.77), head=(6, -10, 0))),
        (10, P(torso=(22, 48, -8), hips=(6, 14, 0), stance='tuck', arm=(-0.62, -0.06, -0.78), off=(0.64, 0.16, 0.75), head=(8, -12, 0))),
        (11, P(torso=(-34, -26, 4), hips=(-14, -10, 0), stance='trail', arm=(-0.02, 0.64, 0.77), off=(0.4, -0.5, -0.77), head=(16, 0, 0)), 'lin'),
        (18, P(torso=(-30, -22, 4), hips=(-12, -8, 0), stance='trail', arm=(0.0, 0.66, 0.75), off=(0.4, -0.5, -0.77), head=(14, 0, 0))),
        (22, P(torso=(14, -8, 0), stance='tuck_half', arm=(-0.1, -0.3, 0.95), off=(0.4, -0.7, 0.59))),
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
