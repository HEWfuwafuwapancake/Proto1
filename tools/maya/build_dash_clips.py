# -*- coding: utf-8 -*-
"""ダッシュのクリップ（地面・空中）を MekaGirl のリグに作り、FBX に書き出す（No.144）。

使い方（どちらでもよい）:
  Maya の画面（Maya MCP）から: mekagirl_motion.mb を開いたまま、
      import build_dash_clips as d; d.build(); d.export('<assets/models>')
  mayapy から:
      mayapy tools/maya/build_dash_clips.py <mekagirl_motion.mb> build
      mayapy tools/maya/build_dash_clips.py <mekagirl_motion.mb> export <assets/models>

姿勢の作り方・向きの書き方は build_weapon_clips.py と同じ（+Z が前、-X がキャラの右、+Y が上）。
全身のクリップで左右の組はない。24fps・ループしない。区切りは 飛び出し 0-2・進む 2-5・止まる 5-9。
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
    'dash_push': ((-2, 4, 22), (2, 18, -34)),  # 右膝を前へ突き出し、左足で後ろへ蹴る
    'dash_brace': ((-6, 0, 18), (6, 0, -16)),  # 足を前後に開いて踏ん張る
    'arrow': ((-1, 22, -44), (1, 30, -54)),  # 空中で脚をそろえて後ろへ伸ばす
    'arrow_end': ((-3, 20, 8), (3, 26, -10)),  # 空中で膝を軽く曲げて立て直す
})

# name, ファイル名, .mb の開始フレーム, [(フレーム, 姿勢, 'lin' ならこのキーへ等速で動く)]
CLIPS = [
    # 地面のダッシュ：一瞬で低く沈んで前へ飛び出す 0-2・約40°前傾で両腕を後ろへ流して進む 2-5・軽く踏ん張って止まる 5-9
    ('dash', 'mekagirl_dash', 5601, [
        (0, r.rpose()),
        (2, r.rpose(torso=(40, 0, 0), hips=(16, 0, 0), ty=-16, tz=6, stance='dash_push', head=(-44, 0, 0),
                    arm_r=(-0.3, -0.42, -0.86), arm_l=(0.3, -0.42, -0.86)), 'lin'),
        (5, r.rpose(torso=(42, 0, 0), hips=(14, 0, 0), ty=-13, tz=8, stance='dash_push', head=(-46, 0, 0),
                    arm_r=(-0.26, -0.24, -0.93), arm_l=(0.26, -0.24, -0.93))),
        (9, r.rpose(torso=(8, 0, 0), hips=(-4, 0, 0), ty=-10, tz=2, stance='dash_brace', head=(-6, 0, 0),
                    arm_r=(-0.5, -0.66, 0.56), arm_l=(0.5, -0.66, 0.56))),
    ]),
    # 空中のダッシュ：体を水平近くまで倒す 0-2・右腕を前、左腕と脚を後ろへ伸ばして矢のように飛ぶ 2-5・立て直す 5-9
    ('air_dash', 'mekagirl_air_dash', 5701, [
        (0, r.rpose()),
        (2, r.rpose(torso=(62, 0, 0), hips=(38, 0, 0), stance='arrow', head=(-70, 0, 0),
                    arm_r=(-0.12, 0.12, 0.98), arm_l=(0.22, -0.18, -0.96)), 'lin'),
        (5, r.rpose(torso=(66, 0, 0), hips=(40, 0, 0), stance='arrow', head=(-74, 0, 0),
                    arm_r=(-0.08, 0.18, 0.98), arm_l=(0.18, -0.12, -0.98))),
        (9, r.rpose(torso=(18, 0, 0), hips=(8, 0, 0), stance='arrow_end', head=(-14, 0, 0),
                    arm_r=(-0.6, -0.3, 0.74), arm_l=(0.6, -0.3, 0.74))),
    ]),
]


def build(path=None):
    """クリップのキーを打って保存する（path を省くと、開いているシーンに作る）。"""
    if path:
        cmds.file(path, open=True, force=True, executeScriptNodes=False, prompt=False)
    joints = cmds.ls(type='joint')
    for _, _, start, _ in CLIPS:
        cmds.cutKey(joints, time=(start, start + 99), clear=True)
    idle = {j: tuple(cmds.getAttr(j + '.rotate', time=1)[0]) for j in joints}
    pelvis_t = tuple(cmds.getAttr('pelvis.translate', time=1)[0])
    cmds.currentTime(1)
    w.RIG['t'] = {j: tuple(cmds.getAttr(j + '.translate', time=1)[0]) for j in joints}
    w.RIG['ankle'] = {j: tuple(cmds.xform(j, q=True, ws=True, t=True)) for j in ('foot_r', 'foot_l')}
    for name, _, start, keys in CLIPS:
        w.write_clip(w.sample_clip(keys, idle, pelvis_t), start, False, idle, r.MOVED)
        print('built', name, start, keys[-1][0])
    cmds.file(save=True, type='mayaBinary', force=True)
    print('saved', cmds.file(q=True, sceneName=True))


def export(out_dir, path=None):
    """クリップを1つずつ FBX に書き出す（書き出したあと、元のシーンを開き直す）。"""
    path = path or cmds.file(q=True, sceneName=True)
    cmds.loadPlugin('fbxmaya', quiet=True)
    for name, file_name, start, keys in CLIPS:
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
