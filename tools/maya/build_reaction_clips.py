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
    'recoil': ((-2, 0, -6), (2, 0, 4)),  # 押されて片足が下がる
    'kick_a': ((-3, 24, 16), (3, 10, -8)),  # 空中で右足を蹴り上げる
    'kick_b': ((-3, 10, -8), (3, 24, 16)),  # 空中で左足を蹴り上げる
    'slump': ((-2, 0, 4), (2, 0, -2)),  # 膝が抜けてぐったり
    'dangle_a': ((-3, 6, 12), (3, 16, -8)),  # つかまれて足をばたつかせる
    'dangle_b': ((-3, 16, -8), (3, 6, 12)),
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
CLIPS = [
    # のけぞり（当たった瞬間）：はじかれる 0-2・のけぞる 2-4・戻る 4-8
    ('flinch', [
        (0, rpose()),
        (2, rpose(torso=(-24, 0, 4), hips=(-6, 0, 0), ty=-4, tz=-4, stance='recoil', head=(-24, 0, 0),
                  arm_r=(-0.42, 0.28, 0.86), arm_l=(0.42, 0.24, 0.87)), 'lin'),
        (4, rpose(torso=(-30, 4, 6), hips=(-8, 0, 0), ty=-5, tz=-5, stance='recoil', head=(-28, 0, 0),
                  arm_r=(-0.56, 0.42, 0.71), arm_l=(0.54, 0.36, 0.76))),
        (8, rpose(torso=(-8, 0, 0), ty=-2, stance='low', head=(-6, 0, 0),
                   arm_r=(-0.4, -0.78, 0.48), arm_l=(0.4, -0.78, 0.48))),
    ]),
    # 吹っ飛び中（ループ）：反ったまま手足をばたつかせる
    ('launched', [
        (0, rpose(torso=(-30, 0, 6), hips=(-10, 0, 0), stance='kick_a', head=(-16, 0, 0),
                  arm_r=(-0.7, 0.62, 0.35), arm_l=(0.6, 0.3, 0.74))),
        (6, rpose(torso=(-34, 8, 0), hips=(-12, 4, 0), stance='low', head=(-20, 6, 0),
                  arm_r=(-0.82, 0.48, -0.3), arm_l=(0.72, 0.64, 0.26))),
        (12, rpose(torso=(-30, 0, -6), hips=(-10, 0, 0), stance='kick_b', head=(-16, 0, 0),
                   arm_r=(-0.6, 0.3, 0.74), arm_l=(0.7, 0.62, 0.35))),
        (18, rpose(torso=(-34, -8, 0), hips=(-12, -4, 0), stance='low', head=(-20, -6, 0),
                   arm_r=(-0.72, 0.64, 0.26), arm_l=(0.82, 0.48, -0.3))),
        (24, rpose(torso=(-30, 0, 6), hips=(-10, 0, 0), stance='kick_a', head=(-16, 0, 0),
                   arm_r=(-0.7, 0.62, 0.35), arm_l=(0.6, 0.3, 0.74))),
    ]),
    # しびれ（ループ）：前へ折れて膝が抜け、腕をだらりと下げてゆっくり揺れる
    ('stun', [
        (0, rpose(torso=(28, 0, 6), hips=(8, 0, 0), ty=-12, stance='slump', head=(24, 0, 6),
                  arm_r=(-0.12, -0.98, 0.16), arm_l=(0.12, -0.98, 0.16))),
        (12, rpose(torso=(32, 4, 0), hips=(9, 2, 0), ty=-13, stance='slump', head=(30, 8, 0),
                   arm_r=(-0.04, -0.96, 0.28), arm_l=(0.22, -0.96, 0.06))),
        (24, rpose(torso=(28, 0, -6), hips=(8, 0, 0), ty=-12, stance='slump', head=(24, 0, -6),
                   arm_r=(-0.12, -0.98, 0.16), arm_l=(0.12, -0.98, 0.16))),
        (36, rpose(torso=(26, -4, 0), hips=(7, -2, 0), ty=-11, stance='slump', head=(20, -8, 0),
                   arm_r=(-0.22, -0.96, 0.06), arm_l=(0.04, -0.96, 0.28))),
        (48, rpose(torso=(28, 0, 6), hips=(8, 0, 0), ty=-12, stance='slump', head=(24, 0, 6),
                   arm_r=(-0.12, -0.98, 0.16), arm_l=(0.12, -0.98, 0.16))),
    ]),
    # つかまれ中（ループ）：体をよじり、腕で押しのけ、足をばたつかせる
    ('held', [
        (0, rpose(torso=(8, 18, 0), hips=(0, 8, 0), stance='dangle_a', head=(-12, -8, 0),
                  arm_r=(-0.2, 0.7, 0.68), arm_l=(0.5, -0.2, 0.84))),
        (8, rpose(torso=(8, -18, 0), hips=(0, -8, 0), stance='dangle_b', head=(-12, 8, 0),
                  arm_r=(-0.5, -0.2, 0.84), arm_l=(0.2, 0.7, 0.68))),
        (16, rpose(torso=(8, 18, 0), hips=(0, 8, 0), stance='dangle_a', head=(-12, -8, 0),
                   arm_r=(-0.2, 0.7, 0.68), arm_l=(0.5, -0.2, 0.84))),
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
    cmds.cutKey(joints, time=(START, START + CLIP_GAP * len(CLIPS)), clear=True)
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
