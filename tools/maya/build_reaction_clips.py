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
    'trail_a': ((-5, 26, 30), (5, 34, 20)),  # 吹っ飛び中：脚をそろえて前へ流す
    'trail_b': ((-5, 32, 22), (5, 28, 28)),
    'buckle': ((-5, 0, 5), (5, 0, -3)),  # 膝ががくっと抜ける
    'kick_s_a': ((-4, 10, 12), (4, 18, -6)),  # つかまれて小さく蹴る
    'kick_s_b': ((-4, 18, -6), (4, 10, 12)),
})

DOWN_R = (-0.28, -0.94, 0.2)
DOWN_L = (0.28, -0.94, 0.2)


def rpose(torso=(0, 0, 0), hips=(0, 0, 0), ty=0.0, tz=0.0, stance=None, arm_r=DOWN_R, arm_l=DOWN_L, head=(0, 0, 0),
          fore_r=None, fore_l=None):
    """両腕と頭の向きも決める姿勢（head は首と頭に半分ずつ足す度数。fore_r・fore_l は前腕の向きで、省くとひじを伸ばす）。"""
    p = w.pose(torso=torso, hips=hips, ty=ty, tz=tz, stance=stance, arm=arm_r, off=arm_l, fore=fore_r, off_fore=fore_l)
    for joint in ('neck', 'head'):
        base = p['joints'][joint]
        p['joints'][joint] = (base[0] + head[0] * 0.5, base[1] + head[1] * 0.5, base[2] + head[2] * 0.5)
    return p


# name, [(フレーム, 姿勢, 'lin' ならこのキーへ等速で動く)]。ループするクリップは最初と最後を同じ姿勢にする。
# No.145 で作り直した：手足を振り回さず、はじかれる瞬間を鋭く・止めて見せ、吹っ飛び中は体ごと流れる姿勢にする。
CLIPS = [
    # のけぞり（当たった瞬間）：1フレームで体ごとはじかれる 0-1・止める 1-3（ヒットストップ風）・吹っ飛びの姿勢へ流れる 3-8
    #   右肩を後ろへひねり、右腕で顔をかばい、左腕は後ろへ流す
    ('flinch', [
        (0, rpose()),
        (1, rpose(torso=(-30, -24, -8), hips=(-10, -10, 0), ty=-10, tz=-8, stance='recoil', head=(-30, -10, 0),
                  arm_r=(0.15, 0.55, 0.82), arm_l=(0.55, -0.35, -0.76)), 'lin'),
        (3, rpose(torso=(-34, -28, -9), hips=(-12, -12, 0), ty=-11, tz=-10, stance='recoil', head=(-34, -12, 0),
                  arm_r=(0.17, 0.58, 0.8), arm_l=(0.58, -0.3, -0.76))),
        (8, rpose(torso=(-28, -10, -4), hips=(-12, -4, 0), ty=-6, tz=-6, stance='recoil', head=(-16, -4, 0),
                  arm_r=(-0.35, 0.15, 0.92), arm_l=(0.35, 0.1, 0.93))),
    ]),
    # 吹っ飛び中（ループ）：体を弓なりに反らし、腕と脚をそろえて前（飛ばされる向きの反対）へまっすぐ流す。手足は小さく揺らすだけ
    ('launched', [
        (0, rpose(torso=(-40, 0, 0), hips=(-24, 0, 0), stance='trail_a', head=(-8, 0, 0),
                  arm_r=(-0.3, 0.35, 0.89), arm_l=(0.3, 0.42, 0.86))),
        (12, rpose(torso=(-46, 4, 5), hips=(-26, 2, 0), stance='trail_b', head=(-12, 3, 0),
                   arm_r=(-0.34, 0.44, 0.83), arm_l=(0.26, 0.34, 0.9))),
        (24, rpose(torso=(-40, 0, 0), hips=(-24, 0, 0), stance='trail_a', head=(-8, 0, 0),
                   arm_r=(-0.3, 0.35, 0.89), arm_l=(0.3, 0.42, 0.86))),
    ]),
    # しびれ（ループ）：膝ががくっと抜けて半分しゃがみ、頭がゆっくり円を描いてふらつく。腕はだらりと揺れる
    ('stun', [
        (0, rpose(torso=(16, 6, 10), hips=(4, 4, 0), ty=-24, stance='buckle', head=(20, 0, 18),
                  arm_r=(-0.24, -0.94, 0.24), arm_l=(0.18, -0.96, 0.2))),
        (12, rpose(torso=(24, 10, 0), hips=(6, 6, 0), ty=-28, stance='buckle', head=(34, 18, 0),
                   arm_r=(-0.18, -0.96, 0.2), arm_l=(0.24, -0.94, 0.24))),
        (24, rpose(torso=(16, -6, -10), hips=(4, -4, 0), ty=-24, stance='buckle', head=(20, 0, -18),
                   arm_r=(-0.18, -0.96, 0.2), arm_l=(0.24, -0.94, 0.24))),
        (36, rpose(torso=(10, -10, 0), hips=(2, -6, 0), ty=-20, stance='buckle', head=(6, -18, 0),
                   arm_r=(-0.24, -0.94, 0.24), arm_l=(0.18, -0.96, 0.2))),
        (48, rpose(torso=(16, 6, 10), hips=(4, 4, 0), ty=-24, stance='buckle', head=(20, 0, 18),
                   arm_r=(-0.24, -0.94, 0.24), arm_l=(0.18, -0.96, 0.2))),
    ]),
    # つかまれ中（ループ）：両手で胸の前の触手をつかんで引きはがそうとし、体を左右に強くよじる。足は小さく蹴る
    ('held', [
        (0, rpose(torso=(-10, 30, 6), hips=(0, 12, 0), stance='kick_s_a', head=(-14, -12, 0),
                  arm_r=(0.1, 0.22, 0.97), arm_l=(-0.06, 0.26, 0.96))),
        (4, rpose(torso=(-16, 0, 0), hips=(-4, 0, 0), stance='kick_s_b', head=(-20, 0, 0),
                  arm_r=(0.02, 0.3, 0.95), arm_l=(-0.02, 0.3, 0.95)), 'lin'),
        (8, rpose(torso=(-10, -30, -6), hips=(0, -12, 0), stance='kick_s_a', head=(-14, 12, 0),
                  arm_r=(0.06, 0.26, 0.96), arm_l=(-0.1, 0.22, 0.97))),
        (12, rpose(torso=(-16, 0, 0), hips=(-4, 0, 0), stance='kick_s_b', head=(-20, 0, 0),
                   arm_r=(0.02, 0.3, 0.95), arm_l=(-0.02, 0.3, 0.95)), 'lin'),
        (16, rpose(torso=(-10, 30, 6), hips=(0, 12, 0), stance='kick_s_a', head=(-14, -12, 0),
                   arm_r=(0.1, 0.22, 0.97), arm_l=(-0.06, 0.26, 0.96))),
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
