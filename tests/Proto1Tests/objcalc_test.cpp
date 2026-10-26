#include "CppUnitTest.h"
#include "render/objcalc.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float OBJ_EPSILON = 0.0001f;
	static const char* TRIANGLE = "v 0 0 2\nv 2 0 2\nv 0 3 2\n";
	TEST_CLASS(ObjCalcTest)
	{
	public:
		TEST_METHOD(Triangle_ReflectsZAndReversesWinding)
		{
			std::vector<OBJ_VERTEX> v;
			Assert::IsTrue(ObjCalcParse(std::string(TRIANGLE) + "f 1 2 3\n", v));
			Assert::AreEqual(size_t(3), v.size());
			Assert::AreEqual(-2.0f, v[0].Position.Z, OBJ_EPSILON);
			Assert::AreEqual(3.0f, v[1].Position.Y, OBJ_EPSILON);
			Assert::AreEqual(2.0f, v[2].Position.X, OBJ_EPSILON);
			for (const auto& vertex : v) {
				Assert::AreEqual(0.0f, vertex.Normal.X, OBJ_EPSILON);
				Assert::AreEqual(0.0f, vertex.Normal.Y, OBJ_EPSILON);
				Assert::AreEqual(-1.0f, vertex.Normal.Z, OBJ_EPSILON);
			}
		}
		TEST_METHOD(QuadAndPentagon_FanTriangles)
		{
			std::vector<OBJ_VERTEX> v;
			const std::string points = "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nv -1 0 0\n";
			Assert::IsTrue(ObjCalcParse(points + "f 1 2 3 4", v));
			Assert::AreEqual(size_t(6), v.size());
			Assert::AreEqual(0.0f, v[3].Position.X, OBJ_EPSILON);
			Assert::AreEqual(0.0f, v[4].Position.X, OBJ_EPSILON);
			Assert::AreEqual(1.0f, v[4].Position.Y, OBJ_EPSILON);
			Assert::AreEqual(1.0f, v[5].Position.X, OBJ_EPSILON);
			Assert::IsTrue(ObjCalcParse(points + "f 1 2 3 4 5", v));
			Assert::AreEqual(size_t(9), v.size());
			Assert::AreEqual(-1.0f, v[7].Position.X, OBJ_EPSILON);
		}
		TEST_METHOD(NormalForms_ReflectAndNormalize)
		{
			for (const char* face : { "f 1//1 2//1 3//1", "f 1/1/1 2/2/1 3/3/1",
				"f -3//-1 -2//-1 -1//-1", "f -3/-3/-1 -2/-2/-1 -1/-1/-1" }) {
				std::vector<OBJ_VERTEX> v;
				Assert::IsTrue(ObjCalcParse(std::string(TRIANGLE) + "vn 0 3 4\n" + face, v));
				Assert::AreEqual(3.0f, v[1].Position.Y, OBJ_EPSILON);
				Assert::AreEqual(0.6f, v[1].Normal.Y, OBJ_EPSILON);
				Assert::AreEqual(-0.8f, v[1].Normal.Z, OBJ_EPSILON);
			}
		}
		TEST_METHOD(NegativePositions_AndIgnoredLines)
		{
			std::vector<OBJ_VERTEX> v;
			Assert::IsTrue(ObjCalcParse(std::string("# comment\r\no arm\ng part\ns off\nmtllib a\nusemtl b\nvt 0 0\n")
				+ TRIANGLE + "f -3 -2 -1 # comment\n", v));
			Assert::AreEqual(size_t(3), v.size());
			Assert::AreEqual(3.0f, v[1].Position.Y, OBJ_EPSILON);
		}
		TEST_METHOD(BrokenLines_FailAndClearOutput)
		{
			for (const char* broken : { "v 1 2", "v x 2 3", "v 1 2 3bad", "vn 1 2", "vn 0 0 0",
				"f 1 2", "f 0 2 3", "f 1 2 4", "f -4 -2 -1", "f 1//2 2//1 3//1",
				"f 1//-2 2//1 3//1", "f 1x 2 3", "f 1/ 2 3", "f 1///1 2 3",
				"f 1/a/1 2 3", "f 1// 2 3", "f 999999999999999999999999 2 3" }) {
				std::vector<OBJ_VERTEX> v(1);
				Assert::IsFalse(ObjCalcParse(std::string(TRIANGLE) + "vn 0 0 1\nf 1 2 3\n" + broken, v));
				Assert::IsTrue(v.empty());
			}
			std::vector<OBJ_VERTEX> v;
			Assert::IsFalse(ObjCalcParse("# no faces", v));
		}
	};
}
