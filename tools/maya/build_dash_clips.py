# -*- coding: utf-8 -*-
"""ダッシュのクリップ（地面・空中）を MekaGirl のリグに作り、FBX に書き出す（No.144）。

使い方（どちらでもよい）:
  Maya の画面（Maya MCP）から: mekagirl_motion.mb を開いたまま、
      import build_dash_clips as d; d.build(); d.export('<assets/models>')
  mayapy から:
      mayapy tools/maya/build_dash_clips.py <mekagirl_motion.mb> build
      mayapy tools/maya/build_dash_clips.py <mekagirl_motion.mb> export <assets/models>

姿勢の作り方・向きの書き方は build_weapon_clips.py と同じ（+Z が前、-X がキャラの右、+Y が上）。
全身のクリップで左右の組はない。24fps・ループしない。区切りは 飛び出し 0-2・進む 2-5・止まる 5-9（キーはその中で自由に置く）。
ゲームの秒数は fighter.cpp の DASH_CLIP_SECONDS（9/24 秒）にそろえる。
約束は docs/plan/07_アニメの追加/142_144_ひねりの修正・横走りの取りやめ・ダッシュ.md。
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build_weapon_clips as w  # noqa: E402
import build_reaction_clips as r  # noqa: E402  （姿勢の作り方 rpose を使う）
import maya.cmds as cmds  # noqa: E402
import maya.mel as mel  # noqa: E402

# 足首を待機の位置からずらす量（cm。右足・左足）
w.STANCES.update({
    'dash_set': ((-3, 0, 6), (3, 0, -10)),  # 一瞬沈んで構える
    'dash_drive': ((-2, 30, 28), (2, 26, -58)),  # 右膝を高く前へ、左脚を後ろへ伸ばし切って蹴る
    'dash_brace': ((-6, 0, 22), (6, 0, -20)),  # 足を前後に開いて踏ん張る
    'arrow': ((-1, 26, -50), (1, 32, -58)),  # 空中で脚をそろえて後ろへ伸ばす
    'arrow_end': ((-3, 20, 8), (3, 26, -10)),  # 空中で膝を軽く曲げて立て直す
    'side_set': ((-8, 0, 0), (8, 0, 0)),  # 横へ沈んで構える
    'side_drive': ((-32, 6, 2), (34, 6, -2)),  # 右足を大きく右へ踏み出し、左脚を伸ばし切って蹴る
    'side_brace': ((-26, 0, 0), (20, 0, 0)),  # 足を横に開いて踏ん張る
})

# 横ダッシュ（右へ。左は左右反転）：体は前を向いたまま一瞬沈む 0-1・右へ大きく傾いて蹴り出す 1-3・滑る 3-5・踏ん張る 5-9（No.146・147）
SIDE_KEYS = [
    (0, r.rpose()),
    (1, r.rpose(torso=(10, 0, -6), hips=(0, 0, -4), ty=-14, stance='side_set', head=(0, 0, 4),
                arm_r=(-0.4, -0.8, 0.45), arm_l=(0.4, -0.8, 0.45)), 'lin'),
    (3, r.rpose(torso=(16, 0, 22), hips=(0, 0, 10), ty=-20, stance='side_drive', head=(0, 0, -20),
                arm_r=(-0.85, -0.3, 0.42), fore_r=(-0.3, 0.2, 0.93), arm_l=(0.55, -0.1, -0.83), fore_l=(0.1, -0.7, -0.7)), 'lin'),
    (5, r.rpose(torso=(18, 0, 24), hips=(0, 0, 11), ty=-21, stance='side_drive', head=(0, 0, -22),
                arm_r=(-0.88, -0.25, 0.4), fore_r=(-0.35, 0.25, 0.9), arm_l=(0.5, -0.05, -0.86), fore_l=(0.12, -0.65, -0.75))),
    (9, r.rpose(torso=(8, 0, -6), hips=(0, 0, -3), ty=-14, stance='side_brace', head=(0, 0, 4),
                arm_r=(-0.6, -0.72, 0.35), arm_l=(0.5, -0.78, 0.36))),
]

# name, ファイル名, .mb の開始フレーム, [(フレーム, 姿勢, 'lin' ならこのキーへ等速で動く)], 左右反転して書くか
# No.147：ひじを曲げて腕を大きく振り、一瞬沈んでから一気に伸び上がる形にした（疾走感）
CLIPS = [
    # 地面のダッシュ：一瞬沈む 0-1・約55°前傾で後ろ脚を伸ばし切って飛び出す 1-3・ひじを曲げた腕を前後へ大きく振って進む 3-5・踏ん張る 5-9
    ('dash', 'mekagirl_dash', 5601, [
        (0, r.rpose()),
        (1, r.rpose(torso=(30, 0, 0), hips=(10, 0, 0), ty=-22, stance='dash_set', head=(-30, 0, 0),
                    arm_r=(-0.3, -0.5, -0.81), arm_l=(0.3, -0.5, -0.81)), 'lin'),
        (3, r.rpose(torso=(58, 0, 0), hips=(24, 0, 0), ty=-14, tz=10, stance='dash_drive', head=(-66, 0, 0),
                    arm_r=(-0.15, -0.62, 0.77), fore_r=(-0.08, 0.35, 0.93),
                    arm_l=(0.18, 0.42, -0.89), fore_l=(0.08, -0.55, -0.83)), 'lin'),
        (5, r.rpose(torso=(60, 0, 0), hips=(24, 0, 0), ty=-13, tz=12, stance='dash_drive', head=(-68, 0, 0),
                    arm_r=(-0.12, -0.55, 0.83), fore_r=(-0.06, 0.45, 0.89),
                    arm_l=(0.16, 0.5, -0.85), fore_l=(0.06, -0.5, -0.86))),
        (9, r.rpose(torso=(16, 0, 0), hips=(-2, 0, 0), ty=-14, tz=3, stance='dash_brace', head=(-12, 0, 0),
                    arm_r=(-0.45, -0.7, 0.55), fore_r=(-0.3, -0.2, 0.93),
                    arm_l=(0.45, -0.7, 0.55), fore_l=(0.3, -0.2, 0.93))),
    ], False),
    # 空中のダッシュ：体をほぼ水平まで倒す 0-2・右腕を前、左腕と脚を後ろへ伸ばして矢のように飛ぶ 2-5・立て直す 5-9
    ('air_dash', 'mekagirl_air_dash', 5701, [
        (0, r.rpose()),
        (2, r.rpose(torso=(74, 0, 0), hips=(46, 0, 0), stance='arrow', head=(-84, 0, 0),
                    arm_r=(-0.1, 0.12, 0.99), arm_l=(0.2, -0.16, -0.97)), 'lin'),
        (5, r.rpose(torso=(78, 0, 0), hips=(48, 0, 0), stance='arrow', head=(-88, 0, 0),
                    arm_r=(-0.06, 0.16, 0.99), arm_l=(0.16, -0.1, -0.98))),
        (9, r.rpose(torso=(20, 0, 0), hips=(8, 0, 0), stance='arrow_end', head=(-16, 0, 0),
                    arm_r=(-0.6, -0.3, 0.74), arm_l=(0.6, -0.3, 0.74))),
    ], False),
    ('side_dash_r', 'mekagirl_side_dash_r', 5801, SIDE_KEYS, False),
    ('side_dash_l', 'mekagirl_side_dash_l', 5901, SIDE_KEYS, True),
]


def build(path=None):
    """クリップのキーを打って保存する（path を省くと、開いているシーンに作る）。"""
    if path:
        cmds.file(path, open=True, force=True, executeScriptNodes=False, prompt=False)
    joints = cmds.ls(type='joint')
    for _, _, start, _, _ in CLIPS:
        cmds.cutKey(joints, time=(start, start + 99), clear=True)
    idle = {j: tuple(cmds.getAttr(j + '.rotate', time=1)[0]) for j in joints}
    pelvis_t = tuple(cmds.getAttr('pelvis.translate', time=1)[0])
    cmds.currentTime(1)
    w.RIG['t'] = {j: tuple(cmds.getAttr(j + '.translate', time=1)[0]) for j in joints}
    w.RIG['ankle'] = {j: tuple(cmds.xform(j, q=True, ws=True, t=True)) for j in ('foot_r', 'foot_l')}
    for name, _, start, keys, mirror in CLIPS:
        w.write_clip(w.sample_clip(keys, idle, pelvis_t), start, mirror, idle, r.MOVED)
        print('built', name, start, keys[-1][0])
    cmds.file(save=True, type='mayaBinary', force=True)
    print('saved', cmds.file(q=True, sceneName=True))


def export(out_dir, path=None):
    """クリップを1つずつ FBX に書き出す（書き出したあと、元のシーンを開き直す）。"""
    path = path or cmds.file(q=True, sceneName=True)
    cmds.loadPlugin('fbxmaya', quiet=True)
    for name, file_name, start, keys, _ in CLIPS:
        length = keys[-1][0] - keys[0][0]
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
        out = '%s/%s.fbx' % (out_dir.replace('\\', '/'), file_name)
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
