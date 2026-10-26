# -*- coding: utf-8 -*-
"""新しいアーム5種を OBJ に書き出す（tools/export-arm-obj.ps1 から mayapy で実行）。

Maya の右手系・cm を保ち、各部品の原点を引いて回転する。拡大・鏡写しはしない。
v / vn / f v//vn のみを書き、面は三角形に分割しない（ゲームの objcalc が行う）。
元の .mb は保存しない。シーンのスクリプトも実行しない。
"""
import argparse
import math
import os
import sys

sys.stdout.reconfigure(encoding="utf-8")

import maya.standalone
import maya.cmds as cmds
import maya.api.OpenMaya as om

# ---- 調整用の値 ----
# parts は root の中の transform 自身のメッシュだけ（子を自動で含めない）。
# origin は Maya のワールド座標で求める。pivot は作者が置いた関節の軸。
# wire_end はワイヤーの下端中央、axis_back は X 回転後の軸の後端面の中央。
# メカ3種は肩・上腕が別形状なので共通化しない。別の腕の残骸も混ざっているため
# root 外は採用しない。非表示の |polySurface12 / |polySurface14 は製作用の見本。
ARMS = {
    "grapple": {
        "source": "grappling.mb", "root": "|gurappuru",
        "omit": ("pCylinder32",),  # 既存ワイヤー。ゲームで長さを変えて描く。
        "files": {
            "arm_grapple_upper.obj": {
                "parts": ("pCube28", "polySurface16", "pCube26"),
                "origin": ("pivot", "pCube28"), "rotate_x": 0.0,
            },
            "arm_grapple_lower.obj": {
                "parts": ("polySurface24", "polySurface9", "pCylinder18"),
                "origin": ("pivot", "polySurface24"), "rotate_x": 0.0,
            },
            "arm_grapple_hook.obj": {
                "parts": ("pCylinder17", "polySurface26", "polySurface27"),
                "origin": ("wire_end", "pCylinder32"), "rotate_x": 0.0,
            },
        },
    },
    "drill": {
        "source": "doriru.mb", "root": "|doriru", "omit": (),
        "files": {
            "arm_drill_base.obj": {
                "parts": ("polySurface4", "pCylinder10", "pCylinder11", "pCylinder12", "pCube8", "pCube7", "pCube5", "pCylinder9", "polySurface5"),
                "origin": ("pivot", "polySurface4"), "rotate_x": 0.0,
            },
            "arm_drill_bit.obj": {
                "parts": ("polySurface1", "pCylinder8"),
                # 軸は元の (0,-1,+1) 方向。X に +45度回し、先端を -Y にする。
                # 後ろへ伸びる軸も削らず含め、軸の後端を原点にする。
                "origin": ("axis_back", "pCylinder8"), "rotate_x": 45.0,
            },
        },
    },
    "shield": {
        "source": "dekaitate.mb", "root": "|tanku", "omit": (),
        "files": {
            "arm_shield_base.obj": {
                "parts": ("pCube14", "pCylinder20", "polySurface11", "polySurface17", "pCube15", "pCylinder22", "polySurface18", "pCube13", "pCube16", "pCube17", "pCube18", "pCube22", "pCube23", "pCube19", "pCube20", "pCube21"),
                "origin": ("pivot", "pCube14"), "rotate_x": 0.0,
            },
            "arm_shield_plate.obj": {
                # 同名の非表示の見本は root 外。腕についている盾だけを採用。
                # 盾は取付点より肩側にも張り出す。元の構えを保つため回さない。
                "parts": ("polySurface14",),
                "origin": ("pivot", "polySurface14"), "rotate_x": 0.0,
            },
        },
    },
    "magnet": {
        "source": "magnet.mb", "root": "|magu", "omit": (),
        "files": {
            "arm_magnet.obj": {
                "parts": ("polySurface22", "polySurface19", "pCube25", "polySurface21"),
                "origin": ("pivot", "polySurface22"), "rotate_x": 0.0,
            },
        },
    },
    "hasami": {
        "source": "hasami.mb", "root": "|hasami", "omit": (),
        "files": {
            "arm_hasami_base.obj": {
                "parts": ("polySurface8", "pCylinder15", "polySurface6", "pCylinder14", "polySurface7", "pCylinder16", "pCube12", "pCylinder13", "pCube10", "pCube11"),
                "origin": ("pivot", "polySurface8"), "rotate_x": 0.0,
            },
            "arm_hasami_blade_a.obj": {
                "parts": ("sweep2",),
                # 刃の pivot ではなく、共通のちょうつがい pCylinder13 の中心。
                # 両方の刃とも X 軸の周りに回せる。
                "origin": ("pivot", "pCylinder13"), "rotate_x": 0.0,
            },
            "arm_hasami_blade_b.obj": {
                "parts": ("sweep3",),
                "origin": ("pivot", "pCylinder13"), "rotate_x": 0.0,
            },
        },
    },
}
AXIS_FACE_TOLERANCE = 0.00001  # 後端面の頂点を集める許容差（cm）
TRIANGLE_WARNING = 5000


def mesh_shapes(transform):
    """transform 自身の、表示用メッシュ（中間オブジェクトを除く）。"""
    return cmds.listRelatives(transform, shapes=True, fullPath=True, noIntermediate=True, type="mesh") or []


def mesh_function(shape):
    selection = om.MSelectionList()
    selection.add(shape)
    return om.MFnMesh(selection.getDagPath(0))


def find_transform(root, name):
    found = [n for n in cmds.ls(name, long=True, type="transform")
             if n.startswith(root + "|") and mesh_shapes(n)]
    if len(found) != 1:
        raise RuntimeError("%s 内の %s が1つに決まりません: %s" % (root, name, found))
    return found[0]


def rotate_x(point, degrees):
    angle = math.radians(degrees)
    c, s = math.cos(angle), math.sin(angle)
    return (point[0], c * point[1] - s * point[2], s * point[1] + c * point[2])


def piece_origin(root, spec):
    kind, name = spec["origin"]
    node = find_transform(root, name)
    if kind == "pivot":
        return tuple(cmds.xform(node, q=True, ws=True, rotatePivot=True))
    points = [tuple(p)[:3] for shape in mesh_shapes(node)
              for p in mesh_function(shape).getPoints(om.MSpace.kWorld)]
    if kind == "wire_end":
        # ワイヤー下端の面の頂点平均（中心頂点も同じ中心なので含めてよい）。
        end = min(p[1] for p in points)
        face = [p for p in points if abs(p[1] - end) < AXIS_FACE_TOLERANCE]
    elif kind == "axis_back":
        rotated = [rotate_x(p, spec["rotate_x"]) for p in points]
        end = max(p[1] for p in rotated)
        face = [p for p, q in zip(points, rotated) if abs(q[1] - end) < AXIS_FACE_TOLERANCE]
    else:
        raise RuntimeError("知らない原点の決め方: " + kind)
    return tuple(sum(p[i] for p in face) / len(face) for i in range(3))


def inspect_scene():
    """各 transform の親・自身のメッシュの範囲・三角形数を出す。"""
    for node in sorted(cmds.ls(type="transform", long=True)):
        shapes = mesh_shapes(node)
        parent = cmds.listRelatives(node, parent=True, fullPath=True) or ["なし"]
        bounds = tuple(round(v, 6) for v in cmds.exactWorldBoundingBox(shapes)) if shapes else None
        triangles = sum(len(mesh_function(s).getPolygonVertices(i)) - 2
                        for s in shapes for i in range(mesh_function(s).numPolygons))
        print("  %s 親=%s 範囲=%s 三角形=%d" % (node, parent[0], bounds, triangles))


def write_obj(path, shapes, origin, degrees):
    lines = []
    lo, hi = [float("inf")] * 3, [float("-inf")] * 3
    v_base, n_base, triangles = 1, 1, 0
    for shape in shapes:
        fn = mesh_function(shape)
        points = fn.getPoints(om.MSpace.kWorld)
        normals = fn.getNormals(om.MSpace.kWorld)
        for p in points:
            q = rotate_x(tuple(p[i] - origin[i] for i in range(3)), degrees)
            for i in range(3):
                lo[i], hi[i] = min(lo[i], q[i]), max(hi[i], q[i])
            lines.append("v %.6f %.6f %.6f" % q)
        # 面積 0 の面は法線が 0 になり、ゲームの objcalc が OBJ ごと読み込みを止める。
        # その面は書かず、使われない法線は番号をずらさないよう +Y で埋める。
        zero = set()
        for i, n in enumerate(normals):
            if n.length() < 1e-6:
                zero.add(i)
                lines.append("vn 0.000000 1.000000 0.000000")
                continue
            q = om.MVector(rotate_x((n.x, n.y, n.z), degrees)).normal()
            lines.append("vn %.6f %.6f %.6f" % (q.x, q.y, q.z))
        for face in range(fn.numPolygons):
            verts, nids = fn.getPolygonVertices(face), fn.getFaceNormalIds(face)
            if any(n in zero for n in nids):
                continue
            lines.append("f " + " ".join("%d//%d" % (v + v_base, n + n_base) for v, n in zip(verts, nids)))
            triangles += len(verts) - 2
        v_base += len(points)
        n_base += len(normals)
    with open(path, "w", encoding="utf-8", newline="\n") as stream:
        stream.write("\n".join(lines) + "\n")
    print("%s: 三角形=%d 原点=%s X回転=%.1f度 範囲 min%s max%s 原点→-Y先端=%.6f cm"
          % (os.path.basename(path), triangles, tuple(round(v, 6) for v in origin), degrees,
             tuple(round(v, 6) for v in lo), tuple(round(v, 6) for v in hi), -lo[1]))
    if triangles > TRIANGLE_WARNING:
        print("  注意: 三角形が5000を超えています（減らしていません）。")


def export_arm(name, source_dir, output_dir, inspect):
    arm = ARMS[name]
    cmds.file(os.path.join(source_dir, arm["source"]), open=True, force=True,
              executeScriptNodes=False, ignoreVersion=True)
    unit = cmds.currentUnit(q=True, linear=True)
    print("\n%s: 単位=%s 骨=%d アニメ曲線=%d 画像テクスチャ=%d" % (
        name, unit, len(cmds.ls(type="joint")), len(cmds.ls(type="animCurve")), len(cmds.ls(type="file"))))
    if unit != "cm":
        raise RuntimeError("cm 以外のシーンです。単位を変更せず停止します。")
    if inspect:
        inspect_scene()
    # 表にない追加部品を黙って取りこぼさず、同じメッシュの二重採用も止める。
    assigned = []
    jobs = []
    for filename, spec in arm["files"].items():
        nodes = [find_transform(arm["root"], n) for n in spec["parts"]]
        assigned.extend(nodes)
        jobs.append((filename, spec, nodes, piece_origin(arm["root"], spec)))
    omitted = [find_transform(arm["root"], n) for n in arm["omit"]]
    actual = {n for n in cmds.ls(type="transform", long=True)
              if n.startswith(arm["root"] + "|") and mesh_shapes(n)}
    covered = assigned + omitted
    if len(covered) != len(set(covered)) or actual != set(covered):
        raise RuntimeError("部品の表とシーンが違います（二重登録または未登録）: %s" % (actual.symmetric_difference(covered),))
    for filename, spec, nodes, origin in jobs:
        print("  %s ← %s" % (filename, ", ".join(spec["parts"])))
        write_obj(os.path.join(output_dir, filename), [s for n in nodes for s in mesh_shapes(n)], origin, spec["rotate_x"])
    ignored = sorted(n for n in cmds.ls(type="transform", long=True)
                     if mesh_shapes(n) and n not in assigned)
    print("  除外（別の腕・非表示の見本・ワイヤー）: " + ", ".join(ignored))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source_dir")
    parser.add_argument("output_dir")
    parser.add_argument("arm", nargs="?", choices=["all"] + list(ARMS), default="all")
    parser.add_argument("--inspect", action="store_true", help="transform ごとの情報も出す")
    args = parser.parse_args()
    names = list(ARMS) if args.arm == "all" else [args.arm]
    for name in names:
        if not os.path.isfile(os.path.join(args.source_dir, ARMS[name]["source"])):
            raise RuntimeError("元の .mb がありません: " + ARMS[name]["source"])
    os.makedirs(args.output_dir, exist_ok=True)
    maya.standalone.initialize(name="python")
    try:
        for name in names:
            export_arm(name, args.source_dir, args.output_dir, args.inspect)
    finally:
        maya.standalone.uninitialize()


if __name__ == "__main__":
    main()
