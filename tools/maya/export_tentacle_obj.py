# -*- coding: utf-8 -*-
"""触腕のモデル（Maya の .mb）を、ゲームで読む OBJ（付け根・途中の節・先っぽの3つ）に書き出す。

mayapy で実行する（tools/export-tentacle-obj.ps1 から呼ぶ）:
    mayapy export_tentacle_obj.py <入力の .mb> <書き出すフォルダ>

- 座標は Maya のまま（右手系・Y が上・単位 cm）。左手系への変換はゲーム側（objcalc）で行う。
- 各ファイルは「つなぎ目」が原点、腕が伸びる向きが -Y になるように平行移動するだけ（回転・拡大はしない）。
- 書き出すのは頂点（v）・法線（vn）・面（f v//vn）だけ。テクスチャは使わないので UV は書かない。
- シーンのスクリプトは実行しない（executeScriptNodes=False）。
"""
import os
import sys

# 日本語の表示が文字化けしないように UTF-8 で出す（受け取る側は export-tentacle-obj.ps1 で UTF-8 にしている）
sys.stdout.reconfigure(encoding="utf-8")

import maya.standalone
maya.standalone.initialize(name="python")
import maya.cmds as cmds
import maya.api.OpenMaya as om

# ---- 調整用の値 ----
BASE_PARTS = ("tako", "polySurface23", "pCylinder31")  # 付け根の部品（transform の名前）
SEGMENT_NAME = "TAKO2"  # 途中の節の見本（TAKO1〜5 は同じ形。TAKO2 は次の節までの間隔がほかとそろっている）
NEXT_SEGMENT_NAME = "TAKO3"  # 節の間隔を測る相手
FIRST_SEGMENT_NAME = "TAKO1"  # 付け根の次につながる節
TIP_NAME = "TAKO6"  # 先っぽ


def mesh_shapes(transform):
    """transform 自身が持つメッシュ（中間オブジェクトは除く）"""
    return cmds.listRelatives(transform, shapes=True, fullPath=True, noIntermediate=True, type="mesh") or []


def find_transform(name):
    """短い名前から、メッシュを持つ transform を1つ探す"""
    found = [n for n in cmds.ls(name, long=True, type="transform") if mesh_shapes(n)]
    if len(found) != 1:
        raise RuntimeError("%s が 1 つに決まりません: %s" % (name, found))
    return found[0]


def segment_shapes(transform):
    """節の本体と、その節の直下にある吸盤（pCylinder〜）のメッシュ"""
    shapes = list(mesh_shapes(transform))
    for child in cmds.listRelatives(transform, children=True, fullPath=True, type="transform") or []:
        if child.split("|")[-1].startswith("pCylinder"):
            shapes += mesh_shapes(child)
    return shapes


def pivot(transform):
    return om.MVector(cmds.xform(transform, q=True, ws=True, rotatePivot=True))


def write_obj(path, shapes, origin, title):
    """メッシュをワールド座標のまま origin を原点にずらして1つの OBJ に書く。範囲を返す"""
    lines = ["# %s（tools/maya/export_tentacle_obj.py で書き出し。Maya の座標・単位 cm・-Y が腕の伸びる向き）" % title]
    v_base = 1
    n_base = 1
    lo = [1e9] * 3
    hi = [-1e9] * 3
    tris = 0
    for shape in shapes:
        sel = om.MSelectionList()
        sel.add(shape)
        fn = om.MFnMesh(sel.getDagPath(0))
        points = fn.getPoints(om.MSpace.kWorld)
        normals = fn.getNormals(om.MSpace.kWorld)
        lines.append("o %s" % shape.split("|")[-2])
        for p in points:
            q = (p.x - origin.x, p.y - origin.y, p.z - origin.z)
            for i in range(3):
                lo[i] = min(lo[i], q[i])
                hi[i] = max(hi[i], q[i])
            lines.append("v %.6f %.6f %.6f" % q)
        for n in normals:
            lines.append("vn %.6f %.6f %.6f" % (n.x, n.y, n.z))
        for face in range(fn.numPolygons):
            verts = fn.getPolygonVertices(face)
            nids = fn.getFaceNormalIds(face)
            lines.append("f " + " ".join("%d//%d" % (v + v_base, n + n_base) for v, n in zip(verts, nids)))
            tris += len(verts) - 2
        v_base += len(points)
        n_base += len(normals)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    print("%s: %d 個のメッシュ、三角形 %d、範囲 min(%.3f, %.3f, %.3f) max(%.3f, %.3f, %.3f)"
          % (os.path.basename(path), len(shapes), tris, lo[0], lo[1], lo[2], hi[0], hi[1], hi[2]))
    return lo, hi


def sucker_side(transform, origin):
    """吸盤が節の軸から見てどちら側（X・Z）に付いているか"""
    total = om.MVector()
    count = 0
    for child in cmds.listRelatives(transform, children=True, fullPath=True, type="transform") or []:
        if not child.split("|")[-1].startswith("pCylinder"):
            continue
        for shape in mesh_shapes(child):
            sel = om.MSelectionList()
            sel.add(shape)
            for p in om.MFnMesh(sel.getDagPath(0)).getPoints(om.MSpace.kWorld):
                total += om.MVector(p.x - origin.x, 0.0, p.z - origin.z)
                count += 1
    return total / count if count else total


def main():
    src, out_dir = sys.argv[1], sys.argv[2]
    cmds.file(src, open=True, force=True, executeScriptNodes=False, ignoreVersion=True)
    if not os.path.isdir(out_dir):
        os.makedirs(out_dir)

    base_shapes = []
    for name in BASE_PARTS:
        base_shapes += mesh_shapes(find_transform(name))
    base_top = max(cmds.exactWorldBoundingBox(base_shapes)[4], 0.0)
    first = pivot(find_transform(FIRST_SEGMENT_NAME))
    base_origin = om.MVector(first.x, base_top, first.z)

    segment = find_transform(SEGMENT_NAME)
    seg_origin = pivot(segment)
    seg_next = pivot(find_transform(NEXT_SEGMENT_NAME))
    tip = find_transform(TIP_NAME)
    tip_origin = pivot(tip)

    write_obj(os.path.join(out_dir, "tentacle_base.obj"), base_shapes, base_origin, "触腕の付け根")
    write_obj(os.path.join(out_dir, "tentacle_segment.obj"), segment_shapes(segment), seg_origin, "触腕の途中の節")
    _, tip_hi = write_obj(os.path.join(out_dir, "tentacle_tip.obj"), segment_shapes(tip), tip_origin, "触腕の先っぽ")
    tip_lo = cmds.exactWorldBoundingBox(segment_shapes(tip))[1] - tip_origin.y

    print("付け根の長さ（原点→次の節のつなぎ目）: %.3f" % (base_origin.y - first.y))
    print("節の間隔（原点→次の節のつなぎ目）: %.3f" % (seg_origin.y - seg_next.y))
    print("先っぽの長さ（原点→いちばん先）: %.3f" % (-tip_lo))
    side = sucker_side(segment, seg_origin)
    print("吸盤のある側（節の軸から XZ の平均）: (%.3f, %.3f)" % (side.x, side.z))


main()
