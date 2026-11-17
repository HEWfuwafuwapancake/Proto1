# -*- coding: utf-8 -*-
"""単押しの段階攻撃（No.167）のクリップを MekaGirl のリグに作り、FBX に書き出す。

使い方:
  mayapy tools/maya/build_combo_clips.py <mekagirl_motion.mb> build
  mayapy tools/maya/build_combo_clips.py <mekagirl_motion.mb> export <assets/models>
  （Maya の画面から: import build_combo_clips as c; c.build(); c.export('<assets/models>')）

姿勢の書き方は build_weapon_clips.py と同じ（+Z が前、-X がキャラの右、+Y が上）。
右手（右へ動く）として書き、左手のクリップは左右反転して作る（左手に付けたときは左右が逆の動きになる）。
フレームは 6001 から（右手・左手の順に 100 フレームずつ。24fps）。
クリップの区切りは各腕の .cpp の「調整用の値」の CLIP_SECONDS にそろえる。
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build_more_clips as m  # noqa: E402  （作り方・書き出し方と、追加の足の置き方を使う）
import build_reaction_clips as r  # noqa: E402

DOWN = r.DOWN_R
DOWN_L = r.DOWN_L
HOLD = m.HOLD  # ハンマーの持ち方
HANG = (-0.2, -0.3, 0.93)  # 刀を下げた持ち方


def P(torso=(0, 0, 0), hips=(0, 0, 0), ty=0.0, tz=0.0, stance=None, arm=DOWN, fore=None, hand=None, off=DOWN_L, head=(0, 0, 0)):
    """右手の攻撃の姿勢（fore は前腕の向きで、省くとひじを伸ばす。hand は手先・武器の向き）。"""
    p = r.rpose(torso=torso, hips=hips, ty=ty, tz=tz, stance=stance, arm_r=arm, arm_l=off, head=head, fore_r=fore)
    p['weapon'] = hand
    return p


PUNCH_END = dict(torso=(4, 6, 0), arm=(-0.24, -0.72, 0.65))  # knuckle_punch の終わり（つなぎ）
KATANA_END = dict(torso=(10, 37.5, 5), hips=(0, 22.5, 0), ty=-10.8, tz=6, stance='deep', arm=(-0.8, -0.1, 0.59), hand=(-0.92, 0.1, -0.38))
REACH = m.REACH

# name, ファイル名（%s に r / l）, 右の開始, 左の開始, [(フレーム, 姿勢, 'lin')]
CLIPS = [
    # ナックルの2段目（フック）：右の外へ引く 0-3・ひじを曲げて内へ振り抜く 3-6・戻る 6-12
    ('knuckle_hook', 'mekagirl_atk_knuckle_hook_%s', 6001, 6101, [
        (0, P(**PUNCH_END)),
        (3, P(torso=(6, -34, -6), hips=(0, -12, 0), arm=(-0.92, 0.0, -0.38), fore=(-0.5, 0.1, 0.86), off=(0.45, -0.5, 0.74))),
        (5, P(torso=(14, 18, 4), hips=(0, 8, 0), arm=(-0.82, 0.05, 0.57), fore=(0.2, 0.05, 0.98), off=(0.5, -0.7, 0.5)), 'lin'),
        (6, P(torso=(18, 44, 6), hips=(0, 16, 0), arm=(-0.5, 0.05, 0.86), fore=(0.62, 0.02, 0.78), off=(0.4, -0.8, 0.45), head=(0, -10, 0)), 'lin'),
        (9, P(torso=(16, 40, 6), hips=(0, 14, 0), arm=(-0.52, 0.0, 0.85), fore=(0.58, 0.0, 0.81), off=(0.4, -0.8, 0.45), head=(0, -8, 0))),
        (12, P(**PUNCH_END)),
    ]),
    # ナックルの3段目（アッパー）：膝を曲げて拳を腰へ沈める 0-5・伸び上がって斜め上へ突き上げる 5-9・残す 9-13・戻る 13-19（全身）
    ('knuckle_upper', 'mekagirl_atk_knuckle_upper_%s', 6201, 6301, [
        (0, P(**PUNCH_END)),
        (5, P(torso=(28, -30, -8), hips=(6, -12, 0), ty=-18, stance='wide', arm=(-0.4, -0.85, -0.34), fore=(-0.2, -0.2, 0.96),
              off=(0.5, -0.4, 0.77), head=(-10, 10, 0))),
        (7, P(torso=(-6, 6, 0), hips=(-2, 2, 0), ty=-6, stance='low', arm=(-0.25, 0.1, 0.96), fore=(-0.1, 0.9, 0.42), off=(0.5, -0.7, 0.5)), 'lin'),
        (9, P(torso=(-26, 32, 6), hips=(-8, 12, 0), ty=4, stance='straight', arm=(-0.15, 0.72, 0.68), fore=(-0.02, 0.98, 0.2),
              off=(0.45, -0.75, -0.48), head=(-22, -6, 0)), 'lin'),
        (13, P(torso=(-24, 30, 6), hips=(-7, 11, 0), ty=4, stance='straight', arm=(-0.15, 0.74, 0.66), fore=(-0.02, 0.99, 0.15),
               off=(0.45, -0.75, -0.48), head=(-20, -6, 0))),
        (19, P(torso=(4, 0, 0), ty=-2, stance='low')),
    ]),
    # 刀の2段目（斬り返し）：右へ振り抜いた形から刀を返して振りかぶる 0-4・左へ一気に斬り返す 4-9・残心 9-13・戻る 13-18（全身）
    ('katana_return', 'mekagirl_atk_katana_return_%s', 6401, 6501, [
        (0, P(**KATANA_END)),
        (4, P(torso=(6, 46, 6), hips=(0, 24, 0), ty=-10, tz=4, stance='deep', arm=(-0.86, 0.22, 0.46), hand=(-0.55, 0.78, -0.3))),
        (6, P(torso=(14, 4, 0), hips=(0, 2, 0), ty=-12, tz=6, stance='deep', arm=(-0.12, 0.05, 0.99), hand=(0.2, -0.05, 0.98)), 'lin'),
        (9, P(torso=(16, -40, -5), hips=(0, -20, 0), ty=-12.5, tz=7, stance='deep', arm=(0.74, -0.18, 0.65), hand=(0.9, -0.3, -0.3),
              off=(0.2, -0.85, -0.48), head=(0, 12, 0)), 'lin'),
        (13, P(torso=(14, -36, -5), hips=(0, -18, 0), ty=-11, tz=6, stance='deep', arm=(0.72, -0.24, 0.65), hand=(0.88, -0.34, -0.33),
               off=(0.2, -0.85, -0.48), head=(0, 10, 0))),
        (18, P(torso=(4, -6, 0), ty=-2, stance='low', arm=(0.4, -0.8, 0.45), hand=(0.5, -0.3, -0.8))),
    ]),
    # 刀の3段目（通り抜け斬り）：低く構えて刀を後ろへ 0-6・前へ飛び出して駆け抜ける 6-10・
    #   抜けた先で右へ大きく振り抜く 10-12・残心 12-17・戻る 17-22（全身）
    ('katana_pierce', 'mekagirl_atk_katana_pierce_%s', 6601, 6701, [
        (0, P(torso=(4, -6, 0), ty=-2, stance='low', arm=(0.4, -0.8, 0.45), hand=(0.5, -0.3, -0.8))),
        (6, P(torso=(32, -28, -4), hips=(4, -14, 0), ty=-18, stance='lunge', arm=(0.3, -0.7, 0.65), hand=(0.2, -0.25, -0.95),
              off=(0.5, -0.4, 0.77), head=(-14, 12, 0))),
        (8, P(torso=(58, 0, 0), ty=-12, tz=8, stance='sprint', arm=(-0.2, -0.46, 0.86), hand=(-0.12, -0.22, -0.97)), 'lin'),
        (10, P(torso=(60, 4, 0), ty=-12, tz=9, stance='sprint', arm=(-0.18, -0.44, 0.88), hand=(-0.12, -0.2, -0.97))),
        (12, P(torso=(18, 44, 6), hips=(0, 22, 0), ty=-15, tz=6, stance='deep', arm=(-0.84, -0.04, 0.54), hand=(-0.95, 0.15, -0.27),
               off=(0.6, -0.5, -0.62), head=(0, -12, 0)), 'lin'),
        (17, P(torso=(16, 40, 6), hips=(0, 20, 0), ty=-13, tz=5, stance='deep', arm=(-0.82, -0.1, 0.56), hand=(-0.93, 0.1, -0.35),
               off=(0.6, -0.5, -0.62), head=(0, -10, 0))),
        (22, P(torso=(4, 0, 0), ty=-2, stance='low', arm=DOWN, hand=HANG)),
    ]),
    # ハンマーの1段目：右の斜め上へ振りかぶる 0-8・左の斜め下へ叩きつける 8-11・止める 11-14・戻る 14-18
    ('hammer_swing_a', 'mekagirl_atk_hammer_swing_a_%s', 6801, 6901, [
        (0, P(arm=DOWN, hand=HOLD)),
        (8, P(torso=(-14, -30, -8), hips=(0, -10, 0), arm=(-0.6, 0.76, -0.25), hand=(-0.35, 0.55, -0.76), off=(0.5, -0.3, 0.8), head=(-8, 8, 0))),
        (9, P(torso=(4, -6, 0), arm=(-0.45, 0.5, 0.74), hand=(-0.3, 0.76, 0.58)), 'lin'),
        (11, P(torso=(38, 30, 8), hips=(4, 10, 0), ty=-6, stance='low', arm=(0.36, -0.55, 0.75), hand=(0.46, -0.7, 0.55),
               off=(0.45, -0.7, -0.55), head=(14, -8, 0)), 'lin'),
        (14, P(torso=(34, 28, 8), hips=(4, 9, 0), ty=-5, stance='low', arm=(0.34, -0.58, 0.74), hand=(0.44, -0.72, 0.53),
               off=(0.45, -0.7, -0.55), head=(12, -8, 0))),
        (18, P(torso=(10, 12, 2), arm=(0.05, -0.75, 0.66), hand=(0.15, -0.5, 0.85))),
    ]),
    # ハンマーの2段目：左の斜め上へ振りかぶる 0-8・右の斜め下へ叩きつける 8-11・止める 11-14・戻る 14-18
    ('hammer_swing_b', 'mekagirl_atk_hammer_swing_b_%s', 7001, 7101, [
        (0, P(torso=(10, 12, 2), arm=(0.05, -0.75, 0.66), hand=(0.15, -0.5, 0.85))),
        (8, P(torso=(-14, 34, 8), hips=(0, 10, 0), arm=(0.55, 0.8, -0.18), hand=(0.35, 0.6, -0.72), off=(0.55, -0.6, 0.58), head=(-8, -8, 0))),
        (9, P(torso=(4, 8, 0), arm=(0.36, 0.55, 0.75), hand=(0.25, 0.76, 0.6)), 'lin'),
        (11, P(torso=(38, -28, -8), hips=(4, -10, 0), ty=-6, stance='low', arm=(-0.46, -0.55, 0.7), hand=(-0.56, -0.68, 0.47),
               off=(0.5, -0.6, 0.62), head=(14, 8, 0)), 'lin'),
        (14, P(torso=(34, -26, -8), hips=(4, -9, 0), ty=-5, stance='low', arm=(-0.44, -0.58, 0.69), hand=(-0.54, -0.7, 0.46),
               off=(0.5, -0.6, 0.62), head=(12, 8, 0))),
        (18, P(torso=(6, 0, 0), arm=DOWN, hand=HOLD)),
    ]),
    # 触手の2段目（たたきつけ）：つかんだ腕を頭の上へ振り上げる 0-3・足元へ振り下ろす 3-5・押さえつける 5-8
    ('tentacle_slam', 'mekagirl_atk_tentacle_slam_%s', 7201, 7301, [
        (0, P(**REACH)),
        (3, P(torso=(-22, 12, 0), arm=(-0.15, 0.95, 0.27), off=(0.5, -0.3, 0.81), head=(-16, 0, 0))),
        (4, P(torso=(10, 6, 0), arm=(-0.1, 0.4, 0.91), off=(0.5, -0.5, 0.71)), 'lin'),
        (5, P(torso=(42, 10, 2), hips=(6, 0, 0), ty=-10, stance='low', arm=(-0.1, -0.76, 0.64), off=(0.5, -0.6, 0.62), head=(18, 0, 0)), 'lin'),
        (8, P(torso=(38, 8, 2), hips=(5, 0, 0), ty=-9, stance='low', arm=(-0.1, -0.78, 0.62), off=(0.5, -0.6, 0.62), head=(14, 0, 0))),
    ]),
]


def build(path=None):
    """クリップのキーを打って保存する（path を省くと、開いているシーンに作る）。"""
    m.CLIPS = CLIPS
    m.build(path)


def export(out_dir, path=None):
    """クリップを1つずつ FBX に書き出す（書き出したあと、元のシーンを開き直す）。"""
    m.CLIPS = CLIPS
    m.export(out_dir, path)


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
