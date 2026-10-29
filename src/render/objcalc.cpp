#include "render/objcalc.h"
#include <cmath>
#include <utility>
#include <sstream>
#include <locale>

// 調整用の値
static constexpr float NORMAL_MIN_LENGTH = 0.000001f;

static bool ReadFloat(std::istream& line, float& value)
{
	std::string token;
	if (!(line >> token)) return false;
	std::istringstream number(token);
	number.imbue(std::locale::classic());
	return (number >> value) && number.eof() && std::isfinite(value);
}

struct FACE_INDEX
{
	size_t Position;
	size_t Normal;
	bool HasNormal;
};

static bool ReadInteger(const std::string& text, long long& value)
{
	std::istringstream stream(text);
	stream.imbue(std::locale::classic());
	return (stream >> value) && stream.eof() && value != 0;
}

static bool ReadIndex(const std::string& text, size_t count, size_t& index)
{
	long long value = 0;
	if (!ReadInteger(text, value)) return false;
	const long long size = static_cast<long long>(count);
	if (value > size || value < -size) return false;
	index = static_cast<size_t>(value > 0 ? value - 1 : size + value);
	return true;
}

static bool ReadFaceIndex(const std::string& token, size_t positions, size_t normals, FACE_INDEX& out)
{
	out = {};
	const size_t slash = token.find('/');
	if (!ReadIndex(token.substr(0, slash), positions, out.Position)) return false;
	if (slash == std::string::npos) return true;
	const size_t second = token.find('/', slash + 1);
	if (second == std::string::npos || token.find('/', second + 1) != std::string::npos) return false;
	// UV は使用しない。v/vt/vn の vt は整数の書式だけ確認する。
	const std::string uv = token.substr(slash + 1, second - slash - 1);
	long long unused = 0;
	if (!uv.empty() && !ReadInteger(uv, unused)) return false;
	out.HasNormal = true;
	return ReadIndex(token.substr(second + 1), normals, out.Normal);
}

static VEC3 FaceNormal(const VEC3& a, const VEC3& b, const VEC3& c)
{
	const VEC3 u = Vec3Sub(b, a);
	const VEC3 v = Vec3Sub(c, a);
	const VEC3 n{ u.Y * v.Z - u.Z * v.Y, u.Z * v.X - u.X * v.Z, u.X * v.Y - u.Y * v.X };
	const float length = Vec3Length(n);
	return length > NORMAL_MIN_LENGTH ? Vec3Scale(n, 1.0f / length) : VEC3{};
}

bool ObjCalcParseMtl(const std::string& source, std::vector<OBJ_MATERIAL>& materials)
{
	materials.clear();
	std::vector<OBJ_MATERIAL> result;
	std::istringstream file(source);
	std::string line;
	while (std::getline(file, line)) {
		line = line.substr(0, line.find('#'));
		std::istringstream stream(line);
		stream.imbue(std::locale::classic());
		std::string kind;
		if (!(stream >> kind)) continue;
		if (kind == "newmtl") {
			OBJ_MATERIAL material{};
			if (!(stream >> material.Name)) return false;
			material.Diffuse = { 1.0f, 1.0f, 1.0f };
			result.push_back(material);
		} else if (kind == "Kd") {
			if (result.empty()) return false;
			VEC3& kd = result.back().Diffuse;
			if (!ReadFloat(stream, kd.X) || !ReadFloat(stream, kd.Y) || !ReadFloat(stream, kd.Z)) return false;
		}
	}
	if (result.empty()) return false;
	materials.swap(result);
	return true;
}

std::string ObjCalcGetMtlLib(const std::string& source)
{
	std::istringstream file(source);
	std::string line;
	while (std::getline(file, line)) {
		std::istringstream stream(line.substr(0, line.find('#')));
		std::string kind, name;
		if ((stream >> kind) && kind == "mtllib" && (stream >> name)) return name;
	}
	return {};
}

bool ObjCalcParse(const std::string& source, std::vector<OBJ_VERTEX>& vertices)
{
	return ObjCalcParse(source, {}, vertices);
}

bool ObjCalcParse(const std::string& source, const std::vector<OBJ_MATERIAL>& materials,
	std::vector<OBJ_VERTEX>& vertices)
{
	vertices.clear();
	int material = -1;
	std::vector<VEC3> positions, normals;
	std::vector<OBJ_VERTEX> result;
	std::istringstream file(source);
	std::string line;
	while (std::getline(file, line)) {
		line = line.substr(0, line.find('#'));
		std::istringstream stream(line);
		stream.imbue(std::locale::classic());
		std::string kind;
		if (!(stream >> kind)) continue;
		if (kind == "v" || kind == "vn") {
			VEC3 v{};
			if (!ReadFloat(stream, v.X) || !ReadFloat(stream, v.Y) || !ReadFloat(stream, v.Z)) return false;
			v.Z = -v.Z;
			if (kind == "vn") {
				const float length = Vec3Length(v);
				if (!std::isfinite(length) || length <= NORMAL_MIN_LENGTH) return false;
				v = Vec3Scale(v, 1.0f / length);
				normals.push_back(v);
			} else positions.push_back(v);
		} else if (kind == "usemtl") {
			std::string name;
			stream >> name;
			material = -1;
			for (size_t i = 0; i < materials.size(); ++i) {
				if (materials[i].Name == name) material = static_cast<int>(i);
			}
		} else if (kind == "f") {
			std::vector<FACE_INDEX> face;
			std::string token;
			while (stream >> token) {
				FACE_INDEX index{};
				if (!ReadFaceIndex(token, positions.size(), normals.size(), index)) return false;
				face.push_back(index);
			}
			if (face.size() < 3) return false;
			for (size_t i = 1; i + 1 < face.size(); ++i) {
				// Z の反転に加え、扇形の各三角形の並びも逆にする。
				const FACE_INDEX triangle[]{ face[0], face[i + 1], face[i] };
				const VEC3 normal = FaceNormal(positions[triangle[0].Position],
					positions[triangle[1].Position], positions[triangle[2].Position]);
				for (const FACE_INDEX& index : triangle) {
					result.push_back({ positions[index.Position], index.HasNormal ? normals[index.Normal] : normal, material });
				}
			}
		}
	}
	if (result.empty()) return false;
	vertices.swap(result);
	return true;
}

void ObjCalcMirrorX(std::vector<OBJ_VERTEX>& vertices)
{
	for (OBJ_VERTEX& v : vertices) {
		v.Position.X = -v.Position.X;
		v.Normal.X = -v.Normal.X;
	}
	// 鏡写しで回る向きが逆になるので、2番目と3番目を入れ替えて時計回りに戻す。
	for (size_t i = 0; i + 2 < vertices.size(); i += 3) std::swap(vertices[i + 1], vertices[i + 2]);
}
