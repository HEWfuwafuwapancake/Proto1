# -*- coding: utf-8 -*-
"""武器の攻撃クリップを MekaGirl のリグに作り、FBX に書き出す（No.103）。

使い方（Maya の画面は要らない）:
  mayapy tools/maya/build_weapon_clips.py <mekagirl_motion.mb> build
  mayapy tools/maya/build_weapon_clips.py <mekagirl_motion.mb> export <assets/models>

右手の動きを CLIPS に、左手の動きを LEFT_CLIPS に書く（左手は別の振り方。LEFT_CLIPS は右手として
書いた形を左右反転して左手のクリップにする）。フレームは右手が 1001 から、左手が 2001 から
（1クリップ 100 フレームずつ）。ゲームの段階とクリップの区切りの対応は、各腕の .cpp の
「調整用の値」の CLIP_SECONDS にそろえる（左右で区切りは同じにする）。
off は攻撃していない方の腕の向き（省略すると体のひねりと逆に振る）。
体の左右のひねりは、当たる瞬間に攻撃する側の肩が前へ出る向きにする（No.142 で、右手の居合・ハンマー2種・殴り・突進と、
左手のアサルト・叩きつけ・射撃の符号を直した）。

向きはモデルの空間で書く：+Z が前、-X がキャラの右、+Y が上。腕は肩→手、武器は手→手先の向き。
体の値は待機の姿勢に足す度数（rx +：前へ倒す、ry +：左へ回す、rz +：右へ倒す）。
"""
import sys
import maya.standalone
if __name__ == '__main__':
    # mayapy から動かすときだけ Maya を起こす（Maya の画面から import するときは要らない）。
    maya.standalone.initialize(name='python')
import maya.cmds as cmds
import maya.mel as mel
import maya.api.OpenMaya as om

RIGHT_START = 1001
LEFT_START = 2001
CLIP_GAP = 100

ROOT_CHAIN = {
    'pelvis': 'root', 'spine_01': 'pelvis', 'spine_02': 'spine_01', 'chest': 'spine_02',
    'clavicle_r': 'chest', 'upperarm_r': 'clavicle_r', 'lowerarm_r': 'upperarm_r', 'hand_r': 'lowerarm_r',
    'clavicle_l': 'chest', 'upperarm_l': 'clavicle_l', 'lowerarm_l': 'upperarm_l', 'hand_l': 'lowerarm_l',
    'thigh_r': 'pelvis', 'knee_upper_r': 'thigh_r', 'knee_lower_r': 'knee_upper_r', 'foot_r': 'knee_lower_r',
    'thigh_l': 'pelvis', 'knee_upper_l': 'thigh_l', 'knee_lower_l': 'knee_upper_l', 'foot_l': 'knee_lower_l',
}
CHILD = {'upperarm_r': 'lowerarm_r', 'lowerarm_r': 'hand_r', 'upperarm_l': 'lowerarm_l', 'lowerarm_l': 'hand_l'}


# 足首を待機の位置からずらす量（cm。右足 (x, y, z)・左足 (x, y, z)）。腰の位置から IK で脚を解く。
STANCES = {
    'low': ((-1, 0, 2), (1, 0, -2)),  # 腰を落とす
    'lunge': ((-1, 0, 16), (1, 0, -20)),  # 右足を前、左足を後ろへ踏み込む
    'deep': ((-2, 0, 25), (2, 0, -32)),  # 深く踏み込む
    'sprint': ((-1, 5, 24), (1, 14, -40)),  # 前へ飛び出す（後ろの足は浮かせる）
    'hop': ((-1, 16, 6), (1, 12, -8)),  # 跳び上がる（両足を浮かせる）
}
RIG = {}  # build() で、骨の親からの位置と待機の足首の位置を入れる。


def counter_arm(torso, stance):
    """攻撃していない腕の向きを、体のひねりと逆へ振った形で求める。"""
    if stance == 'sprint':
        return (0.38, -0.3, -0.88)  # 飛び出す間は後ろへ流す
    twist = torso[1]
    return (0.32 + abs(twist) * 0.004, -0.9 + abs(twist) * 0.012, 0.18 - twist * 0.02)


def pose(torso=(0, 0, 0), hips=(0, 0, 0), ty=0.0, tz=0.0, stance=None, arm=None, weapon=None, off=None):
    """1つのキーの姿勢を作る。"""
    joints = {}
    joints['pelvis'] = hips
    for name in ('spine_01', 'spine_02', 'chest'):
        joints[name] = (torso[0] / 3.0, torso[1] / 3.0, torso[2] / 3.0)
    # 顔は正面に残す（体のひねり・倒れの半分だけ戻す）。
    joints['neck'] = (-torso[0] * 0.3, -torso[1] * 0.3, -torso[2] * 0.3)
    joints['head'] = (-torso[0] * 0.2, -torso[1] * 0.2, -torso[2] * 0.2)
    return {'joints': joints, 'ty': ty, 'tz': tz, 'stance': stance, 'arm': arm, 'weapon': weapon,
            'off': off or counter_arm(torso, stance)}


# 垂らした腕（攻撃の前後のつなぎ）
DOWN = (-0.28, -0.94, 0.2)
# name, full（全身か）, [(フレーム, 姿勢, 'lin' ならこのキーへ等速で動く)]
CLIPS = [
    # 刀の居合：構え 0-8・一閃 8-14・残心 14-20・納刀 20-26
    ('katana_iai', True, [
        (0, pose(arm=DOWN, weapon=(-0.2, -0.3, 0.93))),
        (6, pose(torso=(17.5, -27.5, -5), hips=(0, -17.5, 0), ty=-8.4, stance='lunge', arm=(0.62, -0.68, 0.39), weapon=(0.6, -0.16, -0.78))),
        (8, pose(torso=(20, -32.5, -5), hips=(0, -20, 0), ty=-9.6, stance='lunge', arm=(0.64, -0.66, 0.39), weapon=(0.62, -0.16, -0.77))),
        (10, pose(torso=(17.5, -12.5, -2.5), hips=(0, -10, 0), ty=-12, tz=4, stance='deep', arm=(0.42, -0.26, 0.87), weapon=(0.86, 0.04, 0.5)), 'lin'),
        (12, pose(torso=(15, 20, 2.5), hips=(0, 12.5, 0), ty=-13.2, tz=7, stance='deep', arm=(-0.34, -0.1, 0.93), weapon=(-0.4, 0.12, 0.91)), 'lin'),
        (14, pose(torso=(12.5, 42.5, 5), hips=(0, 25, 0), ty=-13.2, tz=8, stance='deep', arm=(-0.82, -0.02, 0.57), weapon=(-0.94, 0.16, -0.3)), 'lin'),
        (20, pose(torso=(10, 37.5, 5), hips=(0, 22.5, 0), ty=-10.8, tz=6, stance='deep', arm=(-0.8, -0.1, 0.59), weapon=(-0.92, 0.1, -0.38))),
        (26, pose(torso=(4, -6, 0), ty=-2, stance='low', arm=(0.5, -0.78, 0.38), weapon=(0.55, -0.18, -0.82))),
    ]),
    # 刀のアサルト：構え 0-8（狙う間は 8 で止める）・空間を斬る 8-20・飛ぶ 20-24・着地 24-34
    ('katana_assault', True, [
        (0, pose(arm=DOWN, weapon=(-0.2, -0.3, 0.93))),
        (5, pose(torso=(-5, -17.5, 0), hips=(0, -10, 0), ty=-3.6, stance='lunge', arm=(-0.34, 0.5, 0.8), weapon=(-0.2, 0.92, 0.33))),
        (8, pose(torso=(-10, -25, 0), hips=(0, -15, 0), ty=-6, stance='lunge', arm=(-0.32, 0.72, 0.62), weapon=(-0.16, 0.9, -0.4))),
        (11, pose(torso=(-12.5, -30, 0), hips=(0, -17.5, 0), ty=-7.2, stance='lunge', arm=(-0.42, 0.78, 0.46), weapon=(-0.3, 0.8, -0.52))),
        (14, pose(torso=(7.5, -5, 0), hips=(0, -2.5, 0), ty=-10.8, tz=3, stance='deep', arm=(-0.18, 0.38, 0.91), weapon=(-0.3, 0.72, 0.62)), 'lin'),
        (16, pose(torso=(22.5, 15, 0), hips=(0, 7.5, 0), ty=-13.2, tz=6, stance='deep', arm=(0.14, -0.2, 0.97), weapon=(0.42, -0.12, 0.9)), 'lin'),
        (18, pose(torso=(30, 30, 0), hips=(0, 15, 0), ty=-14.4, tz=7, stance='deep', arm=(0.42, -0.56, 0.71), weapon=(0.72, -0.62, 0.32)), 'lin'),
        (20, pose(torso=(30, 32.5, 0), hips=(0, 15, 0), ty=-14.4, tz=7, stance='deep', arm=(0.46, -0.6, 0.65), weapon=(0.74, -0.64, 0.2))),
        (24, pose(torso=(57.5, 0, 0), ty=-12, tz=8, stance='sprint', arm=(-0.2, -0.46, 0.86), weapon=(-0.12, -0.22, -0.97))),
        (28, pose(torso=(27.5, -7.5, 0), ty=-14.4, tz=2, stance='low', arm=(-0.46, -0.52, 0.72), weapon=(-0.5, -0.1, 0.86))),
        (34, pose(torso=(4, 0, 0), ty=-2, stance='low', arm=DOWN, weapon=(-0.2, -0.3, 0.93))),
    ]),
    # ハンマーの叩き（単押し）：振り上げ 0-16・叩く 16-20・休み 20-30
    ('hammer_tap', False, [
        (0, pose(arm=DOWN, weapon=(-0.1, 0.4, 0.91))),
        (6, pose(torso=(-10, -17.5, -2.5), arm=(-0.32, 0.44, 0.84), weapon=(-0.12, 0.86, 0.5))),
        (14, pose(torso=(-22.5, -25, -5), arm=(-0.2, 0.94, -0.28), weapon=(-0.06, 0.62, -0.78))),
        (16, pose(torso=(-25, -27.5, -5), arm=(-0.16, 0.96, -0.24), weapon=(-0.04, 0.6, -0.8))),
        (18, pose(torso=(12.5, 0, 0), arm=(-0.14, 0.42, 0.9), weapon=(-0.08, 0.72, 0.69)), 'lin'),
        (20, pose(torso=(42.5, 15, 2.5), arm=(-0.12, -0.62, 0.77), weapon=(-0.04, -0.82, 0.57)), 'lin'),
        (24, pose(torso=(37.5, 12.5, 2.5), arm=(-0.14, -0.66, 0.74), weapon=(-0.05, -0.84, 0.54))),
        (30, pose(torso=(6, 0, 0), arm=DOWN, weapon=(-0.1, 0.4, 0.91))),
    ]),
    # ハンマーの叩きつけ（長押し）：ため 0-22・叩きつけ 22-26・休み 26-40
    ('hammer_slam', True, [
        (0, pose(arm=DOWN, weapon=(-0.1, 0.4, 0.91))),
        (8, pose(torso=(-12.5, -22.5, -2.5), ty=-7.2, stance='low', arm=(-0.3, 0.6, 0.74), weapon=(-0.1, 0.9, 0.42))),
        (18, pose(torso=(-35, -32.5, -7.5), hips=(-7.5, -10, 0), ty=-9.6, stance='low', arm=(-0.14, 0.96, -0.24), weapon=(0.02, 0.42, -0.91))),
        (22, pose(torso=(-42.5, -37.5, -7.5), hips=(-10, -12.5, 0), ty=-12, stance='low', arm=(-0.1, 0.94, -0.33), weapon=(0.06, 0.32, -0.95))),
        (24, pose(torso=(10, -5, 0), hips=(7.5, 0, 0), ty=-15.6, tz=3, stance='lunge', arm=(-0.1, 0.5, 0.86), weapon=(-0.06, 0.8, 0.6)), 'lin'),
        (26, pose(torso=(62.5, 17.5, 2.5), hips=(15, 7.5, 0), ty=-24, tz=7, stance='deep', arm=(-0.08, -0.8, 0.6), weapon=(-0.02, -0.96, 0.28)), 'lin'),
        (32, pose(torso=(57.5, 15, 2.5), hips=(12.5, 7.5, 0), ty=-22.8, tz=6, stance='deep', arm=(-0.1, -0.82, 0.56), weapon=(-0.03, -0.96, 0.27))),
        (40, pose(torso=(6, 0, 0), ty=-3, stance='low', arm=DOWN, weapon=(-0.1, 0.4, 0.91))),
    ]),
    # ガンランスの射撃：構える 0-6・撃つ反動 6-14
    ('gunlance_shot', False, [
        (0, pose(arm=DOWN, weapon=(-0.05, 0.26, 0.96))),
        (4, pose(torso=(5, 15, -2.5), arm=(-0.1, -0.24, 0.96), weapon=(-0.02, 0.0, 1.0))),
        (6, pose(torso=(7.5, 17.5, -2.5), arm=(-0.08, -0.2, 0.98), weapon=(-0.02, 0.02, 1.0))),
        (8, pose(torso=(-17.5, 22.5, -5), arm=(-0.08, 0.02, 1.0), weapon=(-0.02, 0.36, 0.93)), 'lin'),
        (11, pose(torso=(-7.5, 18.8, -2.5), arm=(-0.08, -0.12, 0.99), weapon=(-0.02, 0.18, 0.98))),
        (14, pose(torso=(4, 14, -2), arm=(-0.08, -0.2, 0.98), weapon=(-0.02, 0.02, 1.0))),
    ]),
    # ガンランスの突進：構える 0-5・押し込み（ループ）5-17
    ('gunlance_rush', False, [
        (0, pose(arm=DOWN, weapon=(-0.05, 0.26, 0.96))),
        (5, pose(torso=(32.5, 12.5, -2.5), arm=(-0.04, -0.14, 0.99), weapon=(-0.01, 0.06, 1.0))),
        (8, pose(torso=(37.5, 17.5, -3.8), arm=(-0.04, -0.1, 0.99), weapon=(-0.01, 0.1, 0.99))),
        (11, pose(torso=(30, 10, -1.2), arm=(-0.04, -0.18, 0.98), weapon=(-0.01, 0.02, 1.0))),
        (14, pose(torso=(37.5, 17.5, -3.8), arm=(-0.04, -0.1, 0.99), weapon=(-0.01, 0.1, 0.99))),
        (17, pose(torso=(26, 10, -2), arm=(-0.04, -0.14, 0.99), weapon=(-0.01, 0.06, 1.0))),
    ]),
    # ナックルの殴り（単押し）：引く 0-4・殴る 4-7・休み 7-12
    ('knuckle_punch', False, [
        (0, pose(arm=DOWN)),
        (4, pose(torso=(2.5, -37.5, -5), arm=(-0.46, -0.3, -0.84))),
        (6, pose(torso=(15, 17.5, 2.5), arm=(-0.1, -0.14, 0.98)), 'lin'),
        (7, pose(torso=(20, 37.5, 5), arm=(0.04, -0.04, 1.0)), 'lin'),
        (12, pose(torso=(4, 6, 0), arm=(-0.24, -0.72, 0.65))),
    ]),
    # ナックルの爆速突進：ため 0-10・飛ぶ 10-18・止まる 18-26
    ('knuckle_rush', True, [
        (0, pose(arm=DOWN)),
        (6, pose(torso=(27.5, -37.5, -5), hips=(0, -15, 0), ty=-14.4, stance='lunge', arm=(-0.52, -0.14, -0.84))),
        (10, pose(torso=(37.5, -45, -7.5), hips=(5, -20, 0), ty=-19.2, stance='deep', arm=(-0.56, -0.08, -0.82))),
        (12, pose(torso=(70, 25, 2.5), hips=(15, 10, 0), ty=-16.8, tz=8, stance='sprint', arm=(-0.06, -0.08, 0.99)), 'lin'),
        (18, pose(torso=(75, 32.5, 2.5), hips=(17.5, 12.5, 0), ty=-16.8, tz=9, stance='sprint', arm=(0.0, -0.02, 1.0))),
        (22, pose(torso=(30, 17.5, 2.5), ty=-14.4, tz=2, stance='low', arm=(-0.1, -0.42, 0.9))),
        (26, pose(torso=(4, 0, 0), ty=-2, stance='low', arm=DOWN)),
    ]),
    # ガンランスの突き上げ：刺す 0・持ち上げ 0-8・1発の反動 8-13
    ('gunlance_impale', False, [
        (0, pose(torso=(30, 14, -3), arm=(-0.04, -0.10, 0.99), weapon=(-0.01, 0.10, 0.99))),
        (3, pose(torso=(17.5, 15, -3.8), arm=(-0.06, 0.25, 0.97), weapon=(-0.02, 0.38, 0.92))),
        (8, pose(torso=(-15, 12.5, -2.5), arm=(-0.08, 0.65, 0.76), weapon=(-0.02, 0.86, 0.51))),
        (9, pose(torso=(-25, 17.5, -3.8), arm=(-0.10, 0.76, 0.65), weapon=(-0.02, 0.95, 0.31)), 'lin'),
        (13, pose(torso=(-12, 10, -2), arm=(-0.08, 0.65, 0.76), weapon=(-0.02, 0.86, 0.51))),
    ]),
]


# 左手の振り方（右手として書く。書き出すときに左右反転する）。区切りのフレームは CLIPS と同じ。
HANG = (-0.2, -0.3, 0.93)
LEFT_CLIPS = {
    # 逆手で腰から斬り上げる居合
    'katana_iai': [
        (0, pose(arm=DOWN, weapon=HANG)),
        (6, pose(torso=(12.5, -20, 2.5), hips=(0, -12.5, 0), ty=-9.6, stance='lunge', arm=(-0.72, -0.66, -0.2), weapon=(-0.25, -0.5, -0.83))),
        (8, pose(torso=(15, -25, 2.5), hips=(0, -15, 0), ty=-10.8, stance='lunge', arm=(-0.74, -0.64, -0.2), weapon=(-0.25, -0.46, -0.85))),
        (10, pose(torso=(15, -5, 0), ty=-12, tz=4, stance='deep', arm=(-0.52, -0.62, 0.59), weapon=(-0.82, -0.55, 0.15)), 'lin'),
        (12, pose(torso=(12.5, 20, -2.5), hips=(0, 10, 0), ty=-13.2, tz=7, stance='deep', arm=(0.1, -0.22, 0.97), weapon=(0.35, -0.2, 0.92)), 'lin'),
        (14, pose(torso=(7.5, 45, -5), hips=(0, 22.5, 0), ty=-12, tz=8, stance='deep', arm=(0.66, 0.32, 0.68), weapon=(0.72, 0.66, 0.2)), 'lin'),
        (20, pose(torso=(5, 40, -5), hips=(0, 20, 0), ty=-9.6, tz=6, stance='deep', arm=(0.6, 0.26, 0.75), weapon=(0.7, 0.6, 0.38))),
        (26, pose(torso=(4, -6, 0), ty=-2, stance='low', arm=(-0.62, -0.72, 0.3), weapon=(-0.35, -0.45, -0.82))),
    ],
    # 下段に構えて、下から空間を斬り上げる
    'katana_assault': [
        (0, pose(arm=DOWN, weapon=HANG)),
        (5, pose(torso=(7.5, -22.5, 0), hips=(0, -10, 0), ty=-7.2, stance='lunge', arm=(0.34, -0.62, 0.7), weapon=(0.18, -0.86, -0.47))),
        (8, pose(torso=(12.5, -30, 0), hips=(0, -15, 0), ty=-9.6, stance='lunge', arm=(0.4, -0.7, 0.59), weapon=(0.25, -0.85, -0.46))),
        (11, pose(torso=(15, -32.5, 0), hips=(0, -17.5, 0), ty=-10.8, stance='lunge', arm=(0.46, -0.74, 0.49), weapon=(0.35, -0.86, -0.36))),
        (14, pose(torso=(5, -7.5, 0), ty=-10.8, tz=3, stance='deep', arm=(0.18, -0.24, 0.95), weapon=(0.28, -0.62, 0.73)), 'lin'),
        (16, pose(torso=(-7.5, 15, 0), hips=(0, 7.5, 0), ty=-8.4, tz=6, stance='deep', arm=(-0.18, 0.24, 0.95), weapon=(-0.34, 0.4, 0.85)), 'lin'),
        (18, pose(torso=(-15, 32.5, 0), hips=(0, 15, 0), ty=-6, tz=7, stance='deep', arm=(-0.46, 0.58, 0.67), weapon=(-0.6, 0.76, 0.25)), 'lin'),
        (20, pose(torso=(-15, 35, 0), hips=(0, 15, 0), ty=-6, tz=7, stance='deep', arm=(-0.5, 0.62, 0.6), weapon=(-0.62, 0.76, 0.18))),
        (24, pose(torso=(57.5, 0, 0), ty=-12, tz=8, stance='sprint', arm=(-0.2, -0.46, 0.86), weapon=(-0.12, -0.22, -0.97))),
        (28, pose(torso=(27.5, -7.5, 0), ty=-14.4, tz=2, stance='low', arm=(-0.34, -0.62, 0.7), weapon=(-0.6, -0.3, 0.74))),
        (34, pose(torso=(4, 0, 0), ty=-2, stance='low', arm=DOWN, weapon=HANG)),
    ],
    # 体の横から振り抜く横殴り
    'hammer_tap': [
        (0, pose(arm=DOWN, weapon=(-0.1, 0.4, 0.91))),
        (6, pose(torso=(2.5, -20, 0), arm=(-0.72, 0.02, -0.1), weapon=(-0.6, 0.5, -0.62))),
        (14, pose(torso=(5, -47.5, 2.5), arm=(-0.82, 0.22, -0.52), weapon=(-0.5, 0.38, -0.78))),
        (16, pose(torso=(5, -52.5, 2.5), arm=(-0.8, 0.24, -0.55), weapon=(-0.46, 0.36, -0.81))),
        (18, pose(torso=(10, -7.5, 0), arm=(-0.35, 0.0, 0.94), weapon=(-0.82, 0.02, 0.57)), 'lin'),
        (20, pose(torso=(20, 32.5, -2.5), arm=(0.4, -0.1, 0.91), weapon=(0.75, -0.02, 0.66)), 'lin'),
        (24, pose(torso=(17.5, 42.5, -2.5), arm=(0.55, -0.16, 0.82), weapon=(0.86, -0.08, 0.5))),
        (30, pose(torso=(4, 0, 0), arm=DOWN, weapon=(-0.1, 0.4, 0.91))),
    ],
    # 跳び上がって叩きつける
    'hammer_slam': [
        (0, pose(arm=DOWN, weapon=(-0.1, 0.4, 0.91))),
        (8, pose(torso=(20, 7.5, 0), ty=-15.6, stance='low', arm=(-0.25, -0.5, 0.82), weapon=(-0.1, -0.2, 0.97))),
        (18, pose(torso=(-27.5, -10, 0), ty=14.4, stance='hop', arm=(-0.1, 0.92, 0.38), weapon=(0.0, 0.72, -0.69))),
        (22, pose(torso=(-35, -12.5, 0), ty=19.2, stance='hop', arm=(-0.08, 0.94, 0.33), weapon=(0.02, 0.6, -0.8))),
        (24, pose(torso=(12.5, -2.5, 0), ty=2.4, stance='low', arm=(-0.06, 0.2, 0.98), weapon=(-0.03, 0.4, 0.92)), 'lin'),
        (26, pose(torso=(70, 7.5, 2.5), hips=(15, 0, 0), ty=-26.4, tz=6, stance='deep', arm=(-0.04, -0.84, 0.54), weapon=(-0.01, -0.98, 0.2)), 'lin'),
        (32, pose(torso=(62.5, 7.5, 2.5), hips=(12.5, 0, 0), ty=-24, tz=5, stance='deep', arm=(-0.06, -0.84, 0.54), weapon=(-0.02, -0.98, 0.2))),
        (40, pose(torso=(6, 0, 0), ty=-3, stance='low', arm=DOWN, weapon=(-0.1, 0.4, 0.91))),
    ],
    # 腰だめで撃つ
    'gunlance_shot': [
        (0, pose(arm=DOWN, weapon=(-0.05, 0.26, 0.96))),
        (4, pose(torso=(10, 12.5, 2.5), arm=(-0.14, -0.66, 0.74), weapon=(-0.02, -0.02, 1.0))),
        (6, pose(torso=(12.5, 15, 2.5), arm=(-0.12, -0.62, 0.78), weapon=(-0.02, -0.02, 1.0))),
        (8, pose(torso=(-10, 22.5, 5), arm=(-0.14, -0.74, 0.66), weapon=(-0.02, 0.34, 0.94)), 'lin'),
        (11, pose(torso=(2.5, 17.5, 2.5), arm=(-0.13, -0.68, 0.72), weapon=(-0.02, 0.14, 0.99))),
        (14, pose(torso=(10, 12, 2), arm=(-0.12, -0.62, 0.78), weapon=(-0.02, -0.02, 1.0))),
    ],
    # 槍を何度も突き出しながら進む
    'gunlance_rush': [
        (0, pose(arm=DOWN, weapon=(-0.05, 0.26, 0.96))),
        (5, pose(torso=(22.5, -7.5, 0), arm=(-0.06, -0.34, 0.94), weapon=(-0.01, 0.02, 1.0))),
        (8, pose(torso=(37.5, 15, -2.5), arm=(-0.02, -0.08, 1.0), weapon=(0.0, 0.04, 1.0))),
        (11, pose(torso=(20, -10, 0), arm=(-0.14, -0.5, 0.85), weapon=(-0.03, 0.0, 1.0))),
        (14, pose(torso=(37.5, 15, -2.5), arm=(-0.02, -0.08, 1.0), weapon=(0.0, 0.04, 1.0))),
        (17, pose(torso=(18, -6, 0), arm=(-0.06, -0.34, 0.94), weapon=(-0.01, 0.02, 1.0))),
    ],
    # 下から打ち上げるアッパー
    'knuckle_punch': [
        (0, pose(arm=DOWN)),
        (4, pose(torso=(12.5, -25, 2.5), arm=(-0.3, -0.92, -0.25))),
        (6, pose(torso=(7.5, 0, 0), arm=(-0.2, -0.4, 0.89)), 'lin'),
        (7, pose(torso=(-10, 22.5, -2.5), arm=(-0.05, 0.6, 0.8)), 'lin'),
        (12, pose(torso=(2, 4, 0), arm=(-0.22, -0.66, 0.72))),
    ],
    # 低く沈んでから斜め上へ打ち上げて飛ぶ
    'knuckle_rush': [
        (0, pose(arm=DOWN)),
        (6, pose(torso=(37.5, -22.5, 2.5), hips=(0, -10, 0), ty=-19.2, stance='lunge', arm=(-0.42, -0.84, -0.34))),
        (10, pose(torso=(47.5, -30, 2.5), hips=(5, -15, 0), ty=-24, stance='deep', arm=(-0.46, -0.84, -0.3))),
        (12, pose(torso=(37.5, 20, -2.5), hips=(7.5, 7.5, 0), ty=-9.6, tz=8, stance='sprint', arm=(-0.06, 0.34, 0.94)), 'lin'),
        (18, pose(torso=(32.5, 27.5, -2.5), hips=(5, 10, 0), ty=-7.2, tz=9, stance='sprint', arm=(0.0, 0.5, 0.86))),
        (22, pose(torso=(20, 10, 0), ty=-14.4, tz=2, stance='low', arm=(-0.1, -0.3, 0.95))),
        (26, pose(torso=(4, 0, 0), ty=-2, stance='low', arm=DOWN)),
    ],
    # 左手は少し外側から槍を持ち上げる
    'gunlance_impale': [
        (0, pose(torso=(30, 12, -2), arm=(-0.02, -0.08, 1.0), weapon=(0.0, 0.04, 1.0))),
        (3, pose(torso=(20, 12.5, -2.5), arm=(-0.16, 0.22, 0.96), weapon=(-0.08, 0.35, 0.93))),
        (8, pose(torso=(-12.5, 10, -2.5), arm=(-0.16, 0.63, 0.76), weapon=(-0.08, 0.86, 0.50))),
        (9, pose(torso=(-22.5, 15, -3.8), arm=(-0.18, 0.74, 0.65), weapon=(-0.08, 0.95, 0.30)), 'lin'),
        (13, pose(torso=(-10, 8, -2), arm=(-0.16, 0.63, 0.76), weapon=(-0.08, 0.86, 0.50))),
    ],
}


def quat_from_rotate(values):
    return om.MEulerRotation([om.MAngle(v, om.MAngle.kDegrees).asRadians() for v in values], om.MEulerRotation.kXYZ).asQuaternion()


def add_rotate(base, delta):
    return (base[0] + delta[0], base[1] + delta[1], base[2] + delta[2])


def unit(v):
    d = om.MVector(v[0], v[1], v[2])
    d.normalize()
    return d


def solve_leg(side, offset, p, local, world, pelvis_t):
    """足首を目標の位置に置くように、腿・すね・足の回転を解く（膝は前へ曲げる）。"""
    t = RIG['t']
    thigh, knee_upper, knee_lower, foot = ['%s_%s' % (n, side) for n in ('thigh', 'knee_upper', 'knee_lower', 'foot')]
    hip = om.MVector(pelvis_t[0], pelvis_t[1] + p['ty'], pelvis_t[2] + p['tz']) + om.MVector(t[thigh]).rotateBy(world('pelvis'))
    ankle = om.MVector(RIG['ankle'][foot]) + om.MVector(offset)
    # 腿はつけ根から膝の下まで、1本の骨として扱う（knee_upper は回さない）。
    upper = om.MVector(t[knee_upper]) + om.MVector(t[knee_lower])
    lower = om.MVector(t[foot])
    l1 = upper.length()
    l2 = lower.length()
    to_ankle = ankle - hip
    d = min(to_ankle.length(), l1 + l2 - 0.05)
    n = to_ankle.normal()
    a = (l1 * l1 - l2 * l2 + d * d) / (2.0 * d)
    h = max(0.0, l1 * l1 - a * a) ** 0.5
    pole = om.MVector(-0.12 if side == 'r' else 0.12, 0.0, 1.0)
    bend = (pole - n * (pole * n)).normal()
    knee = hip + n * a + bend * h
    local[knee_upper] = om.MQuaternion()
    local[thigh] = upper.normal().rotateTo((knee - hip).normal().rotateBy(world('pelvis').inverse()))
    target = (hip + n * d - knee).normal().rotateBy(world(knee_upper).inverse())
    local[knee_lower] = lower.normal().rotateTo(target)
    # 足の裏は水平に保つ（world(foot) が単位の回転になるようにする）。
    local[foot] = world(knee_lower).inverse()


def solve_pose(p, idle, pelvis_t):
    """姿勢を、骨ごとのローカルの回転（クォータニオン）と腰の移動にする。"""
    local = {}
    for joint, base in idle.items():
        local[joint] = quat_from_rotate(add_rotate(base, p['joints'].get(joint, (0, 0, 0))))

    def world(joint):
        q = om.MQuaternion()
        chain = []
        while joint in ROOT_CHAIN:
            chain.append(joint)
            joint = ROOT_CHAIN[joint]
        for name in reversed(chain):
            q = local[name] * q
        return q

    if p['stance']:
        offsets = STANCES[p['stance']]
        solve_leg('r', offsets[0], p, local, world, pelvis_t)
        solve_leg('l', offsets[1], p, local, world, pelvis_t)
    def solve_arm(side, arm, weapon):
        upper, lower, hand = ['%s_%s' % (n, side) for n in ('upperarm', 'lowerarm', 'hand')]
        for joint in (upper, lower):
            target = unit(arm).rotateBy(world(ROOT_CHAIN[joint]).inverse())
            # 骨は軸に沿っていないので、子の骨への実際の向きを合わせる。
            local[joint] = om.MVector(RIG['t'][CHILD[joint]]).normal().rotateTo(target)
        target = unit(weapon or arm).rotateBy(world(lower).inverse())
        local[hand] = om.MVector(RIG['t']['hand_end_' + side]).normal().rotateTo(target)

    if p['arm']:
        solve_arm('r', p['arm'], p['weapon'])
    solve_arm('l', p['off'], None)
    return local


def slerp(a, b, t):
    return om.MQuaternion.slerp(a, b, t)


def smooth(t):
    return t * t * (3.0 - 2.0 * t)


def sample_clip(keys, idle, pelvis_t):
    """1フレームごとの、骨のクォータニオンと腰の移動の表を返す。"""
    solved = [(k[0], solve_pose(k[1], idle, pelvis_t), (k[1]['ty'], k[1]['tz']), len(k) > 2) for k in keys]
    frames = []
    for frame in range(keys[0][0], keys[-1][0] + 1):
        for i in range(len(solved) - 1):
            if solved[i][0] <= frame <= solved[i + 1][0]:
                a, b = solved[i], solved[i + 1]
                raw = float(frame - a[0]) / (b[0] - a[0])
                t = raw if b[3] else smooth(raw)
                pose_q = {j: slerp(a[1][j], b[1][j], t) for j in idle}
                ty = a[2][0] + (b[2][0] - a[2][0]) * t
                tz = a[2][1] + (b[2][1] - a[2][1]) * t
                frames.append((frame, pose_q, (pelvis_t[0], pelvis_t[1] + ty, pelvis_t[2] + tz)))
                break
    return frames


def mirror_name(joint):
    if joint.endswith('_r'):
        return joint[:-2] + '_l'
    if joint.endswith('_l'):
        return joint[:-2] + '_r'
    return joint


def to_degrees(q):
    e = q.asEulerRotation()
    return [om.MAngle(v, om.MAngle.kRadians).asDegrees() for v in (e.x, e.y, e.z)]


def write_clip(frames, start, mirror, idle_values, moved):
    """フレームの表をキーにする。動かない骨は最初と最後だけ打って、隣のクリップから切り離す。"""
    first = frames[0][0]
    last = frames[-1][0]
    for frame, q_table, t in frames:
        time = start + frame - first
        for joint, q in q_table.items():
            if joint not in moved and frame not in (first, last):
                continue
            target = joint
            value = to_degrees(q)
            if mirror:
                target = mirror_name(joint)
                value = [value[0], -value[1], -value[2]]
            for axis, v in zip('XYZ', value):
                cmds.setKeyframe(target, attribute='rotate' + axis, t=time, v=v)
        x = -t[0] if mirror else t[0]
        for axis, v in zip('XYZ', (x, t[1], t[2])):
            cmds.setKeyframe('pelvis', attribute='translate' + axis, t=time, v=v)


def clip_starts():
    for index, clip in enumerate(CLIPS):
        length = clip[2][-1][0] - clip[2][0][0]
        yield clip[0], 'r', RIGHT_START + CLIP_GAP * index, length
        yield clip[0], 'l', LEFT_START + CLIP_GAP * index, length


def build(path):
    cmds.file(path, open=True, force=True, executeScriptNodes=False, prompt=False)
    joints = cmds.ls(type='joint')
    # やり直せるように、作るクリップの場所のキーを先に消す。
    cmds.cutKey(joints, time=(RIGHT_START, LEFT_START + CLIP_GAP * len(CLIPS)), clear=True)
    idle = {j: tuple(cmds.getAttr(j + '.rotate', time=1)[0]) for j in joints}
    pelvis_t = tuple(cmds.getAttr('pelvis.translate', time=1)[0])
    cmds.currentTime(1)
    RIG['t'] = {j: tuple(cmds.getAttr(j + '.translate', time=1)[0]) for j in joints}
    RIG['ankle'] = {j: tuple(cmds.xform(j, q=True, ws=True, t=True)) for j in ('foot_r', 'foot_l')}
    moved = {'pelvis', 'spine_01', 'spine_02', 'chest', 'neck', 'head', 'upperarm_r', 'lowerarm_r', 'hand_r',
             'upperarm_l', 'lowerarm_l', 'hand_l', 'thigh_r', 'thigh_l', 'knee_upper_r', 'knee_upper_l', 'knee_lower_r', 'knee_lower_l', 'foot_r', 'foot_l'}
    for index, (name, full, keys) in enumerate(CLIPS):
        left_keys = LEFT_CLIPS[name]
        if left_keys[-1][0] != keys[-1][0]:
            raise RuntimeError('左右のクリップの長さがちがいます: ' + name)
        write_clip(sample_clip(keys, idle, pelvis_t), RIGHT_START + CLIP_GAP * index, False, idle, moved)
        write_clip(sample_clip(left_keys, idle, pelvis_t), LEFT_START + CLIP_GAP * index, True, idle, moved)
        print('built', name, keys[-1][0])
    cmds.file(save=True, type='mayaBinary', force=True)
    print('saved', path)


def export(path, out_dir):
    cmds.loadPlugin('fbxmaya', quiet=True)
    for name, side, start, length in clip_starts():
        cmds.file(path, open=True, force=True, executeScriptNodes=False, prompt=False)
        joints = cmds.ls(type='joint')
        # このクリップだけを残し、0 フレームから始まるようにずらす。
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
        out = '%s/mekagirl_atk_%s_%s.fbx' % (out_dir.replace('\\', '/'), name, side)
        mel.eval('FBXExport -f "%s" -s' % out)
        print('exported', out)


if __name__ == '__main__':
    try:
        if sys.argv[2] == 'build':
            build(sys.argv[1])
        else:
            export(sys.argv[1], sys.argv[3])
    finally:
        maya.standalone.uninitialize()
