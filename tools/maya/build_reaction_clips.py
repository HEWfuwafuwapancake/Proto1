# -*- coding: utf-8 -*-
"""やられ・吹っ飛びのクリップを MekaGirl のリグに作り、FBX に書き出す（No.138）。

使い方（どちらでもよい）:
  Maya の画面（Maya MCP）から: mekagirl_motion.mb を開いたまま、
      import build_reaction_clips as r; r.build(); r.export('<assets/models>')
  mayapy から:
      mayapy tools/maya/build_reaction_clips.py <mekagirl_motion.mb> build
      mayapy tools/maya/build_reaction_clips.py <mekagirl_motion.mb> export <assets/models>

姿勢の作り方・向きの書き方は build_weapon_clips.py と同じ（+Z が前、-X がキャラの右、+Y が上）。
全身のクリップで左右の組はない。フレームは 3001 から 100 フレームずつ（24fps）。
ゲームの秒数は fighter.cpp の FLINCH_ANIM_SECONDS（hit_flinch の長さ）にそろえる。
クリップの名前・フレーム・長さの約束は docs/plan/07_アニメの追加/138_やられのアニメ.md。
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build_weapon_clips as w  # noqa: E402
import maya.cmds as cmds  # noqa: E402
import maya.mel as mel  # noqa: E402

START = 3001
CLIP_GAP = 100

# 足首を待機の位置からずらす量（cm。右足・左足）
w.STANCES.update({
    'recoil': ((-4, 0, -16), (4, 8, 12)),  # 押されて右足が下がり、左足が浮く
    'kick_a': ((-6, 40, 24), (6, 14, -16)),  # 空中で右足を大きく蹴り上げる
    'kick_b': ((-6, 14, -16), (6, 40, 24)),  # 空中で左足を大きく蹴り上げる
    'flail': ((-10, 24, -10), (10, 26, 4)),  # 空中で両足を開いてもがく
    'slump': ((-1, 0, 8), (1, 0, -4)),  # 膝が抜けて内股でぐったり
    'dangle_a': ((-4, 6, 20), (4, 28, -14)),  # つかまれて足をばたつかせる
    'dangle_b': ((-4, 28, -14), (4, 6, 20)),
})

DOWN_R = (-0.28, -0.94, 0.2)
DOWN_L = (0.28, -0.94, 0.2)


def rpose(torso=(0, 0, 0), hips=(0, 0, 0), ty=0.0, tz=0.0, stance=None, arm_r=DOWN_R, arm_l=DOWN_L, head=(0, 0, 0)):
    """両腕と頭の向きも決める姿勢（head は首と頭に半分ずつ足す度数）。"""
    p = w.pose(torso=torso, hips=hips, ty=ty, tz=tz, stance=stance, arm=arm_r, off=arm_l)
    for joint in ('neck', 'head'):
        base = p['joints'][joint]
        p['joints'][joint] = (base[0] + head[0] * 0.5, base[1] + head[1] * 0.5, base[2] + head[2] * 0.5)
    return p


# name, [(フレーム, 姿勢, 'lin' ならこのキーへ等速で動く)]。ループするクリップは最初と最後を同じ姿勢にする。
# 大きく・速く見せるため、当たる瞬間は 'lin' で一気に動かし、行き過ぎてから戻す。
CLIPS = [
    # のけぞり（当たった瞬間）：はじかれる 0-1・行き過ぎる 1-3・戻りかける 3-8
    ('flinch', [
        (0, rpose()),
        (1, rpose(torso=(-38, 10, 10), hips=(-14, 4, 0), ty=-8, tz=-10, stance='recoil', head=(-40, 6, 0),
                  arm_r=(-0.5, 0.55, 0.67), arm_l=(0.45, 0.6, 0.66)), 'lin'),
        (3, rpose(torso=(-46, 14, 12), hips=(-16, 6, 0), ty=-10, tz=-12, stance='recoil', head=(-48, 8, 0),
                  arm_r=(-0.62, 0.7, 0.35), arm_l=(0.55, 0.75, 0.37))),
        (8, rpose(torso=(-16, 4, 4), hips=(-6, 0, 0), ty=-6, tz=-4, stance='low', head=(-12, 0, 0),
                  arm_r=(-0.5, -0.4, 0.77), arm_l=(0.5, -0.35, 0.79))),
    ]),
    # 吹っ飛び中（ループ）：大きく反ってきりもみ気味によじれ、手足を大きく振り回す
    ('launched', [
        (0, rpose(torso=(-48, 16, 14), hips=(-20, 10, 0), stance='kick_a', head=(-30, 10, 0),
                  arm_r=(-0.85, 0.5, 0.15), arm_l=(0.5, 0.75, 0.43))),
        (6, rpose(torso=(-40, 0, 0), hips=(-16, 0, 0), stance='flail', head=(-36, 0, 0),
                  arm_r=(-0.7, 0.68, -0.2), arm_l=(0.7, 0.68, -0.2))),
        (12, rpose(torso=(-48, -16, -14), hips=(-20, -10, 0), stance='kick_b', head=(-30, -10, 0),
                   arm_r=(-0.5, 0.75, 0.43), arm_l=(0.85, 0.5, 0.15))),
        (18, rpose(torso=(-40, 0, 0), hips=(-16, 0, 0), stance='flail', head=(-36, 0, 0),
                   arm_r=(-0.9, 0.2, 0.38), arm_l=(0.9, 0.2, 0.38))),
        (24, rpose(torso=(-48, 16, 14), hips=(-20, 10, 0), stance='kick_a', head=(-30, 10, 0),
                   arm_r=(-0.85, 0.5, 0.15), arm_l=(0.5, 0.75, 0.43))),
    ]),
    # しびれ（ループ）：深く前へ折れて膝が抜け、腕をぶらぶら振り子のように揺らしながら大きくふらつく
    ('stun', [
        (0, rpose(torso=(40, 8, 12), hips=(10, 4, 0), ty=-20, stance='slump', head=(36, 10, 14),
                  arm_r=(-0.1, -0.94, 0.32), arm_l=(0.1, -0.96, -0.26))),
        (12, rpose(torso=(46, 0, 0), hips=(12, 0, 0), ty=-23, stance='slump', head=(44, 0, 0),
                   arm_r=(-0.06, -1.0, 0.0), arm_l=(0.06, -1.0, 0.0))),
        (24, rpose(torso=(40, -8, -12), hips=(10, -4, 0), ty=-20, stance='slump', head=(36, -10, -14),
                   arm_r=(-0.1, -0.96, -0.26), arm_l=(0.1, -0.94, 0.32))),
        (36, rpose(torso=(34, 0, 0), hips=(8, 0, 0), ty=-17, stance='slump', head=(28, 0, 0),
                   arm_r=(-0.06, -1.0, 0.0), arm_l=(0.06, -1.0, 0.0))),
        (48, rpose(torso=(40, 8, 12), hips=(10, 4, 0), ty=-20, stance='slump', head=(36, 10, 14),
                   arm_r=(-0.1, -0.94, 0.32), arm_l=(0.1, -0.96, -0.26))),
    ]),
    # つかまれ中（ループ）：体を大きくよじり、腕を振り上げて押しのけ、足を大きくばたつかせる
    ('held', [
        (0, rpose(torso=(-6, 32, 8), hips=(0, 14, 0), stance='dangle_a', head=(-18, -16, 0),
                  arm_r=(-0.25, 0.9, 0.36), arm_l=(0.7, -0.1, 0.7))),
        (4, rpose(torso=(-14, 0, 0), hips=(-4, 0, 0), stance='flail', head=(-24, 0, 0),
                  arm_r=(-0.6, 0.6, 0.53), arm_l=(0.6, 0.6, 0.53)), 'lin'),
        (8, rpose(torso=(-6, -32, -8), hips=(0, -14, 0), stance='dangle_b', head=(-18, 16, 0),
                  arm_r=(-0.7, -0.1, 0.7), arm_l=(0.25, 0.9, 0.36))),
        (12, rpose(torso=(-14, 0, 0), hips=(-4, 0, 0), stance='flail', head=(-24, 0, 0),
                   arm_r=(-0.6, 0.6, 0.53), arm_l=(0.6, 0.6, 0.53)), 'lin'),
        (16, rpose(torso=(-6, 32, 8), hips=(0, 14, 0), stance='dangle_a', head=(-18, -16, 0),
                   arm_r=(-0.25, 0.9, 0.36), arm_l=(0.7, -0.1, 0.7))),
    ]),
]

MOVED = {'pelvis', 'spine_01', 'spine_02', 'chest', 'neck', 'head', 'upperarm_r', 'lowerarm_r', 'hand_r',
         'upperarm_l', 'lowerarm_l', 'hand_l', 'thigh_r', 'thigh_l', 'knee_upper_r', 'knee_upper_l',
         'knee_lower_r', 'knee_lower_l', 'foot_r', 'foot_l'}


def clip_starts():
    for index, (name, keys) in enumerate(CLIPS):
        yield name, START + CLIP_GAP * index, keys[-1][0] - keys[0][0]


def build(path=None):
    """クリップのキーを打って保存する（path を省くと、開いているシーンに作る）。"""
    if path:
        cmds.file(path, open=True, force=True, executeScriptNodes=False, prompt=False)
    joints = cmds.ls(type='joint')
    cmds.cutKey(joints, time=(START, START + CLIP_GAP * len(CLIPS) - 1), clear=True)
    idle = {j: tuple(cmds.getAttr(j + '.rotate', time=1)[0]) for j in joints}
    pelvis_t = tuple(cmds.getAttr('pelvis.translate', time=1)[0])
    cmds.currentTime(1)
    w.RIG['t'] = {j: tuple(cmds.getAttr(j + '.translate', time=1)[0]) for j in joints}
    w.RIG['ankle'] = {j: tuple(cmds.xform(j, q=True, ws=True, t=True)) for j in ('foot_r', 'foot_l')}
    for name, start, length in clip_starts():
        keys = dict((c[0], c[1]) for c in CLIPS)[name]
        w.write_clip(w.sample_clip(keys, idle, pelvis_t), start, False, idle, MOVED)
        print('built', name, start, length)
    cmds.file(save=True, type='mayaBinary', force=True)
    print('saved', cmds.file(q=True, sceneName=True))


def export(out_dir, path=None):
    """クリップを1つずつ FBX に書き出す（書き出したあと、元のシーンを開き直す）。"""
    path = path or cmds.file(q=True, sceneName=True)
    cmds.loadPlugin('fbxmaya', quiet=True)
    for name, start, length in clip_starts():
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
        out = '%s/mekagirl_hit_%s.fbx' % (out_dir.replace('\\', '/'), name)
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
