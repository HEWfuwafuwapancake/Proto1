#pragma once

#include <math.h>

/// @file vec3.h
/// @brief 3D の点・向き（ベクトル）と、その小さな計算（足す・引く・掛ける・内積・長さ）
/// @details DirectX を使わないので、src/physics の計算と単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief 3D の点・向き
struct VEC3
{
	float X;	///< X（右がプラス）
	float Y;	///< Y（上がプラス）
	float Z;	///< Z（奥がプラス）
};

/// @brief 2つのベクトルを足す
/// @param a 1つ目
/// @param b 2つ目
/// @return a + b
inline VEC3 Vec3Add(const VEC3& a, const VEC3& b)
{
	return { a.X + b.X, a.Y + b.Y, a.Z + b.Z };
}

/// @brief ベクトルを引く
/// @param a 引かれる方
/// @param b 引く方
/// @return a - b（b から a へ向かう向き）
inline VEC3 Vec3Sub(const VEC3& a, const VEC3& b)
{
	return { a.X - b.X, a.Y - b.Y, a.Z - b.Z };
}

/// @brief ベクトルを何倍かする
/// @param v ベクトル
/// @param s 倍率
/// @return v × s
inline VEC3 Vec3Scale(const VEC3& v, float s)
{
	return { v.X * s, v.Y * s, v.Z * s };
}

/// @brief 内積
/// @param a 1つ目
/// @param b 2つ目
/// @return a・b（同じ向きならプラス、直角なら 0、反対向きならマイナス）
inline float Vec3Dot(const VEC3& a, const VEC3& b)
{
	return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
}

/// @brief 長さの2乗（比べるだけなら平方根がいらないので速い）
/// @param v ベクトル
/// @return 長さの2乗
inline float Vec3LengthSq(const VEC3& v)
{
	return Vec3Dot(v, v);
}

/// @brief 長さ
/// @param v ベクトル
/// @return 長さ
inline float Vec3Length(const VEC3& v)
{
	return sqrtf(Vec3LengthSq(v));
}
