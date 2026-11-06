#include <gtest/gtest.h>
#include "render/objcalc.h"
#include <Windows.h>
#include <fstream>
#include <iterator>
namespace Proto1Tests
{
	static constexpr float OBJ_EPSILON = 0.0001f;
	static const char* TRIANGLE = "v 0 0 2\nv 2 0 2\nv 0 3 2\n";
	TEST(ObjCalcTest, AssetModels_AllObjFilesParse)
	{
		// プロジェクトの UseFullPaths（/FC）で __FILE__ は絶対パスになる。
		const std::string sourcePath = __FILE__;
		const std::string directory = sourcePath.substr(0, sourcePath.find_last_of("/\\") + 1)
			+ "..\\..\\assets\\models\\";
		WIN32_FIND_DATAA entry{};
		const HANDLE search = FindFirstFileA((directory + "*.obj").c_str(), &entry);
		EXPECT_TRUE(search != INVALID_HANDLE_VALUE) << L"assets/models/ の OBJ が見つかりません";
		std::vector<std::string> paths;
		do {
			if (!(entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) paths.push_back(directory + entry.cFileName);
		} while (FindNextFileA(search, &entry));
		const DWORD error = GetLastError();
		FindClose(search);
		EXPECT_TRUE(error == ERROR_NO_MORE_FILES) << L"assets/models/ の OBJ 一覧を読めません";
		EXPECT_FALSE(paths.empty()) << L"assets/models/ に OBJ がありません";
		for (const std::string& path : paths) {
			const std::string& message = path;
			std::ifstream file(path, std::ios::binary);
			EXPECT_TRUE(file.is_open()) << message.c_str();
			const std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			EXPECT_FALSE(file.bad()) << message.c_str();
			std::vector<OBJ_VERTEX> vertices;
			EXPECT_TRUE(ObjCalcParse(source, vertices)) << message.c_str();
		}
	}

	TEST(ObjCalcTest, Triangle_ReflectsZAndReversesWinding)
	{
		std::vector<OBJ_VERTEX> v;
		EXPECT_TRUE(ObjCalcParse(std::string(TRIANGLE) + "f 1 2 3\n", v));
		EXPECT_EQ(size_t(3), v.size());
		EXPECT_NEAR(-2.0f, v[0].Position.Z, OBJ_EPSILON);
		EXPECT_NEAR(3.0f, v[1].Position.Y, OBJ_EPSILON);
		EXPECT_NEAR(2.0f, v[2].Position.X, OBJ_EPSILON);
		for (const auto& vertex : v) {
			EXPECT_NEAR(0.0f, vertex.Normal.X, OBJ_EPSILON);
			EXPECT_NEAR(0.0f, vertex.Normal.Y, OBJ_EPSILON);
			EXPECT_NEAR(-1.0f, vertex.Normal.Z, OBJ_EPSILON);
		}
	}

	TEST(ObjCalcTest, QuadAndPentagon_FanTriangles)
	{
		std::vector<OBJ_VERTEX> v;
		const std::string points = "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nv -1 0 0\n";
		EXPECT_TRUE(ObjCalcParse(points + "f 1 2 3 4", v));
		EXPECT_EQ(size_t(6), v.size());
		EXPECT_NEAR(0.0f, v[3].Position.X, OBJ_EPSILON);
		EXPECT_NEAR(0.0f, v[4].Position.X, OBJ_EPSILON);
		EXPECT_NEAR(1.0f, v[4].Position.Y, OBJ_EPSILON);
		EXPECT_NEAR(1.0f, v[5].Position.X, OBJ_EPSILON);
		EXPECT_TRUE(ObjCalcParse(points + "f 1 2 3 4 5", v));
		EXPECT_EQ(size_t(9), v.size());
		EXPECT_NEAR(-1.0f, v[7].Position.X, OBJ_EPSILON);
	}

	TEST(ObjCalcTest, NormalForms_ReflectAndNormalize)
	{
		for (const char* face : { "f 1//1 2//1 3//1", "f 1/1/1 2/2/1 3/3/1",
			"f -3//-1 -2//-1 -1//-1", "f -3/-3/-1 -2/-2/-1 -1/-1/-1" }) {
			std::vector<OBJ_VERTEX> v;
			EXPECT_TRUE(ObjCalcParse(std::string(TRIANGLE) + "vn 0 3 4\n" + face, v));
			EXPECT_NEAR(3.0f, v[1].Position.Y, OBJ_EPSILON);
			EXPECT_NEAR(0.6f, v[1].Normal.Y, OBJ_EPSILON);
			EXPECT_NEAR(-0.8f, v[1].Normal.Z, OBJ_EPSILON);
		}
	}

	TEST(ObjCalcTest, NegativePositions_AndIgnoredLines)
	{
		std::vector<OBJ_VERTEX> v;
		EXPECT_TRUE(ObjCalcParse(std::string("# comment\r\no arm\ng part\ns off\nmtllib a\nusemtl b\nvt 0 0\n")
			+ TRIANGLE + "f -3 -2 -1 # comment\n", v));
		EXPECT_EQ(size_t(3), v.size());
		EXPECT_NEAR(3.0f, v[1].Position.Y, OBJ_EPSILON);
	}

	TEST(ObjCalcTest, MirrorX_FlipsXAndKeepsFrontFace)
	{
		std::vector<OBJ_VERTEX> v;
		EXPECT_TRUE(ObjCalcParse(std::string("v 1 0 0\nv 3 0 0\nv 1 2 0\nvn 1 0 0\nf 1//1 2//1 3//1\n"), v));
		ObjCalcMirrorX(v);
		EXPECT_EQ(size_t(3), v.size());
		// 読んだ直後の並びは (1,0,0)・(1,2,0)・(3,0,0)。X を反転し、2番目と3番目を入れ替える
		EXPECT_NEAR(-1.0f, v[0].Position.X, OBJ_EPSILON);
		EXPECT_NEAR(-3.0f, v[1].Position.X, OBJ_EPSILON);
		EXPECT_NEAR(-1.0f, v[2].Position.X, OBJ_EPSILON);
		EXPECT_NEAR(2.0f, v[2].Position.Y, OBJ_EPSILON);
		EXPECT_NEAR(-1.0f, v[0].Normal.X, OBJ_EPSILON);
		// 面の向きから求めた法線（表の向き）が、鏡写しの前後で同じ -Z のまま
		std::vector<OBJ_VERTEX> plain;
		EXPECT_TRUE(ObjCalcParse(std::string("v 1 0 0\nv 3 0 0\nv 1 2 0\nf 1 2 3\n"), plain));
		ObjCalcMirrorX(plain);
		const VEC3 u = Vec3Sub(plain[1].Position, plain[0].Position);
		const VEC3 w = Vec3Sub(plain[2].Position, plain[0].Position);
		EXPECT_NEAR(-1.0f, (u.X * w.Y - u.Y * w.X) / Vec3Length(u) / Vec3Length(w), OBJ_EPSILON);
	}

	TEST(ObjCalcTest, Materials_KdAndUsemtlSetVertexMaterial)
	{
		std::vector<OBJ_MATERIAL> m;
		EXPECT_TRUE(ObjCalcParseMtl("newmtl A\nillum 4\nKd 0.5 0.25 1\nnewmtl B\nKa 0 0 0\n", m));
		EXPECT_EQ(size_t(2), m.size());
		EXPECT_EQ(std::string("B"), m[1].Name);
		EXPECT_NEAR(0.25f, m[0].Diffuse.Y, OBJ_EPSILON);
		EXPECT_NEAR(1.0f, m[1].Diffuse.X, OBJ_EPSILON); // Kd がなければ白
		const std::string obj = std::string("mtllib body.mtl\n") + TRIANGLE
			+ "f 1 2 3\nusemtl B\nf 1 2 3\nusemtl A\nf 1 2 3\nusemtl none\nf 1 2 3\n";
		EXPECT_EQ(std::string("body.mtl"), ObjCalcGetMtlLib(obj));
		std::vector<OBJ_VERTEX> v;
		EXPECT_TRUE(ObjCalcParse(obj, m, v));
		EXPECT_EQ(size_t(12), v.size());
		EXPECT_EQ(-1, v[0].Material);
		EXPECT_EQ(1, v[3].Material);
		EXPECT_EQ(0, v[8].Material);
		EXPECT_EQ(-1, v[9].Material);
	}

	TEST(ObjCalcTest, BrokenLines_FailAndClearOutput)
	{
		for (const char* broken : { "v 1 2", "v x 2 3", "v 1 2 3bad", "vn 1 2", "vn 0 0 0",
			"f 1 2", "f 0 2 3", "f 1 2 4", "f -4 -2 -1", "f 1//2 2//1 3//1",
			"f 1//-2 2//1 3//1", "f 1x 2 3", "f 1/ 2 3", "f 1///1 2 3",
			"f 1/a/1 2 3", "f 1// 2 3", "f 999999999999999999999999 2 3" }) {
			std::vector<OBJ_VERTEX> v(1);
			EXPECT_FALSE(ObjCalcParse(std::string(TRIANGLE) + "vn 0 0 1\nf 1 2 3\n" + broken, v));
			EXPECT_TRUE(v.empty());
		}
		std::vector<OBJ_VERTEX> v;
		EXPECT_FALSE(ObjCalcParse("# no faces", v));
	}
}
