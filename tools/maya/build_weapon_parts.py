# -*- coding: utf-8 -*-
"""腕に足す武器の部品を作り、assets/models/source/weapon_parts.mb に保存する（mayapy で実行）。

使い方: mayapy -I tools/maya/build_weapon_parts.py <保存先の .mb>
腕のモデルと同じく cm・右手系で、-Y が腕の進む向き、原点が腕につなぐ所。
形を変えたらこのスクリプトを実行し直し、.\\tools\\export-arm-obj.ps1 weapons で OBJ に書き出す。
"""
import sys

sys.stdout.reconfigure(encoding="utf-8")

import maya.standalone
import maya.cmds as cmds

ROOT = "weapons"


def cube(name, width, height, depth, center):
    node = cmds.polyCube(name=name, width=width, height=height, depth=depth, constructionHistory=False)[0]
    cmds.move(center[0], center[1], center[2], node, absolute=True)
    return node


def cylinder(name, radius, height, center, axis=(0, 1, 0), sides=16):
    node = cmds.polyCylinder(name=name, radius=radius, height=height, axis=axis,
                             subdivisionsAxis=sides, subdivisionsHeight=1, subdivisionsCaps=1,
                             constructionHistory=False)[0]
    cmds.move(center[0], center[1], center[2], node, absolute=True)
    return node


def build_hammer():
    # 柄の先に頭。頭の長い向きは Z（腕を前へ出すと上下になり、叩く向きと合う）。
    return [cylinder("hammer_handle", 0.8, 24.0, (0, -12.0, 0)),
            cube("hammer_head", 8.0, 9.0, 16.0, (0, -28.0, 0)),
            cube("hammer_face_a", 6.0, 7.0, 1.5, (0, -28.0, 8.75)),
            cube("hammer_face_b", 6.0, 7.0, 1.5, (0, -28.0, -8.75))]


def build_fist():
    # 手首の輪 → 拳 → 指の4つのこぶ（X に並ぶ）。
    parts = [cylinder("fist_wrist", 3.6, 2.0, (0, -1.0, 0)),
             cube("fist_palm", 9.0, 7.5, 7.5, (0, -5.75, 0))]
    for i, x in enumerate((-3.3, -1.1, 1.1, 3.3)):
        parts.append(cube("fist_knuckle%d" % i, 2.0, 2.2, 7.5, (x, -10.6, 0)))
    return parts


def build_c4():
    # 上の面が原点、-Y へ厚みがある平たい箱。上に起爆装置。
    return [cube("c4_block", 12.0, 4.0, 8.0, (0, -2.0, 0)),
            cube("c4_band_a", 12.4, 4.2, 1.0, (0, -2.0, 2.2)),
            cube("c4_band_b", 12.4, 4.2, 1.0, (0, -2.0, -2.2)),
            cube("c4_detonator", 4.0, 1.5, 3.0, (2.5, 0.75, 0))]


def build_barrel():
    # 原点から -Y へ伸びる砲身。先と中ほどに太い輪。
    return [cylinder("barrel_tube", 2.0, 24.0, (0, -12.0, 0)),
            cylinder("barrel_ring_mid", 2.6, 2.0, (0, -13.0, 0)),
            cylinder("barrel_muzzle", 3.0, 3.0, (0, -22.5, 0))]


def build_blade():
    # つば → 細長い刀身。刃は -Z 側で、切っ先は背（+Z）へ寄せてとがらせる。
    guard = cylinder("blade_guard", 3.2, 0.7, (0, -0.35, 0))
    blade = cmds.polyCube(name="blade_body", width=0.5, height=38.0, depth=3.2,
                          subdivisionsHeight=4, constructionHistory=False)[0]
    cmds.move(0, -19.7, 0, blade, absolute=True)
    # 切っ先：いちばん先の頂点をすべて背の側へ集める。
    points = cmds.ls(blade + ".vtx[*]", flatten=True)
    tip_y = min(cmds.pointPosition(p, world=True)[1] for p in points)
    for point in points:
        position = cmds.pointPosition(point, world=True)
        if abs(position[1] - tip_y) < 0.001:
            cmds.move(position[0], position[1], 1.6, point, absolute=True, worldSpace=True)
    return [guard, blade]


def build_katana_fittings():
    # 原点はつば（刀身と同じ）。+Y へつかと柄頭、-Y へ刀身を包む鞘。
    return [cylinder("katana_grip", 1.1, 9.0, (0, 4.5, 0)),
            cylinder("katana_pommel", 1.4, 0.8, (0, 9.4, 0)),
            cube("katana_sheath", 1.6, 39.0, 4.4, (0, -19.6, 0))]


def main():
    if len(sys.argv) != 2:
        raise RuntimeError("保存先の .mb を指定してください")
    maya.standalone.initialize(name="python")
    try:
        cmds.file(new=True, force=True)
        cmds.currentUnit(linear="cm")
        group = cmds.group(empty=True, name=ROOT, world=True)
        for node in build_hammer() + build_fist() + build_c4() + build_barrel() + build_blade() + build_katana_fittings():
            # 書き出しの原点は pivot なので、全部品の pivot をワールドの原点にそろえる。
            cmds.xform(node, worldSpace=True, pivots=(0, 0, 0))
            cmds.parent(node, group)
        cmds.file(rename=sys.argv[1])
        cmds.file(save=True, type="mayaBinary", force=True)
        print("保存しました: " + sys.argv[1])
    finally:
        maya.standalone.uninitialize()


if __name__ == "__main__":
    main()
