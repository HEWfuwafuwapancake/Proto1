#include "game/ringoutwall.h"
#include "core/dx11.h"
#include "core/shader.h"
#include "render/texture.h"
#include <math.h>


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
// 帯1組の画像（文字の高さ:上下の帯 = 3:1、横は文字の2倍。3枚とも同じ大きさ）。赤の DEAD LINE（deadline.png）は使わずに残してある。
static constexpr const char* TEXTURE_PATHS[RING_OUT_WALL_LOOK_COUNT] = {
	"assets/textures/deadline_green.png",	// ふだんの帯（緑の DEAD LINE）
	"assets/textures/ringout.png",			// 場外になった所の帯（赤の RING OUT!）
};
static constexpr float CHARACTER_HEIGHT = 1.8f;		// キャラ1体分の高さ（MEKAGIRL）
static constexpr float BAND_HEIGHT = CHARACTER_HEIGHT * 224.0f / 118.0f;	// 帯の高さ（画像の文字の高さ 118px をキャラ1体分にする）
static constexpr float BAND_WIDTH = BAND_HEIGHT * 1982.0f / 224.0f;	// 帯1組の横幅（画像の縦横の比のまま）
static constexpr float BAND_ALPHA = 0.6f;			// 帯全体の濃さ（半透明）
static constexpr int ROUND_SEGMENTS = 48;			// 円い壁を何枚の板で作るか
static constexpr int MAX_CORNERS = 64;				// 壁の角の最大数
static constexpr int VERTICES_PER_PANEL = 6;		// 板1枚の頂点（三角形2枚）
static constexpr int MAX_UNITS = 64;				// 一周に並べる組の最大数
// 1回に描く板の最大数（板は組の境目と角で区切る）
static constexpr int MAX_PANELS = MAX_CORNERS + MAX_UNITS;


void RingOutWall::Initialize(void)
{
	for (int i = 0; i < RING_OUT_WALL_LOOK_COUNT; ++i) m_Textures[i] = TextureLoadFromFile(TEXTURE_PATHS[i]);
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX_3D) * MAX_PANELS * VERTICES_PER_PANEL;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DirectXGetDevice()->CreateBuffer(&bd, nullptr, &m_VertexBuffer);
}

void RingOutWall::Finalize(void)
{
	SAFE_RELEASE(m_VertexBuffer);
	for (auto& texture : m_Textures) SAFE_RELEASE(texture);
}

// 1組（一周の長さ from〜to、最初の角からの m）の帯を、角で折りながら板にして書き込む。書いた頂点の数を返す。
// 画像は拡大・縮小せず、組の幅に合わせて左右の端を同じだけ切る（文字は画像の真ん中の半分にあるので切れない）。
static int AddUnit(const float (*corners)[2], int count, float from, float to, float bottom, float top, VERTEX_3D* vertex, int room)
{
	const float margin = (1.0f - (to - from) / BAND_WIDTH) * 0.5f;	// 画像の左右それぞれから切る割合
	static constexpr XMFLOAT4 COLOR = { 1.0f, 1.0f, 1.0f, BAND_ALPHA };
	int n = 0;
	float along = 0.0f;
	for (int i = 0; i < count && n + VERTICES_PER_PANEL <= room; ++i) {
		const int next = (i + 1) % count;
		const float x0 = corners[i][0], z0 = corners[i][1];
		const float dx = corners[next][0] - x0, dz = corners[next][1] - z0;
		const float length = hypotf(dx, dz);
		const float start = along > from ? along : from;
		const float end = along + length < to ? along + length : to;
		along += length;
		if (length <= 0.0f || end <= start) continue;
		const float t0 = (start - (along - length)) / length, t1 = (end - (along - length)) / length;
		const float ax = x0 + dx * t0, az = z0 + dz * t0, bx = x0 + dx * t1, bz = z0 + dz * t1;
		// 内側（ステージの中心の向き）を表にする。角は上から見て反時計回りなので、左手が内側。
		// 内側から見ると一周の向きは右から左なので、画像の横を逆に貼って文字を左から読めるようにする。
		const XMFLOAT3 normal = { -dz / length, 0.0f, dx / length };
		const float u0 = 1.0f - margin - (start - from) / BAND_WIDTH, u1 = 1.0f - margin - (end - from) / BAND_WIDTH;
		const VERTEX_3D a{ { ax, top, az }, normal, COLOR, { u0, 0.0f } };
		const VERTEX_3D b{ { bx, top, bz }, normal, COLOR, { u1, 0.0f } };
		const VERTEX_3D c{ { ax, bottom, az }, normal, COLOR, { u0, 1.0f } };
		const VERTEX_3D d{ { bx, bottom, bz }, normal, COLOR, { u1, 1.0f } };
		vertex[n++] = a; vertex[n++] = c; vertex[n++] = b;
		vertex[n++] = b; vertex[n++] = c; vertex[n++] = d;
	}
	return n;
}

void RingOutWall::Draw(const STAGE& stage, const VEC3* outPositions, int outCount) const
{
	if (!m_VertexBuffer) return;
	float corners[MAX_CORNERS][2];
	const int count = StageCalcRingOutOutline(stage, corners, MAX_CORNERS, ROUND_SEGMENTS);
	if (count < 3) return;
	const float bottom = StageCalcLowestSpawnY(stage) + CHARACTER_HEIGHT;
	const float top = bottom + BAND_HEIGHT;
	float total = 0.0f;
	for (int i = 0; i < count; ++i) {
		const int next = (i + 1) % count;
		total += hypotf(corners[next][0] - corners[i][0], corners[next][1] - corners[i][1]);
	}
	// 一周がちょうど埋まる組の数（切り上げ）にし、1組の幅を一周 ÷ 組の数にそろえる（画像の幅以下になる。隙間はできない）。
	int units = static_cast<int>(ceilf(total / BAND_WIDTH));
	if (units < 1) units = 1;
	if (units > MAX_UNITS) units = MAX_UNITS;
	const float unitWidth = total / units;

	// 場外になった人ごとに、壁のいちばん近い所の組を RING OUT! にする（復活するまで）。
	bool marked[MAX_UNITS] = {};
	for (int i = 0; i < outCount && i < RING_OUT_WALL_MARK_MAX; ++i) {
		const int unit = static_cast<int>(StageCalcOutlineAlong(corners, count, outPositions[i].X, outPositions[i].Z) / unitWidth);
		marked[unit < units ? unit : units - 1] = true;
	}

	// ふだんの組と RING OUT! の組を、画像ごとにまとめて書き込む。
	ID3D11DeviceContext* context = DirectXGetDeviceContext();
	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (FAILED(context->Map(m_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;
	VERTEX_3D* vertex = static_cast<VERTEX_3D*>(mapped.pData);
	const int room = MAX_PANELS * VERTICES_PER_PANEL;
	int counts[RING_OUT_WALL_LOOK_COUNT] = {};
	int n = 0;
	for (int look = 0; look < RING_OUT_WALL_LOOK_COUNT; ++look) {
		const bool wantMarked = look == RING_OUT_WALL_LOOK_RING_OUT;
		const int first = n;
		for (int unit = 0; unit < units; ++unit) {
			if (marked[unit] != wantMarked) continue;
			n += AddUnit(corners, count, unit * unitWidth, (unit + 1) * unitWidth, bottom, top, vertex + n, room - n);
		}
		counts[look] = n - first;
	}
	context->Unmap(m_VertexBuffer, 0);

	Shader_Begin();
	Shader_SetWorldMatrix(XMMatrixIdentity());
	MATERIAL material{};
	material.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	material.Ambient = { 1.0f, 1.0f, 1.0f, 1.0f };
	material.TextureEnable = TRUE;
	Shader_SetMaterial(material);
	Shader_SetLightEnable(false);
	const UINT stride = sizeof(VERTEX_3D), offset = 0;
	ID3D11Buffer* buffer = m_VertexBuffer;
	context->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 地形・柱の奥なら隠れるよう、深度を見て描く（景観のモデルの飾りは、呼ぶ前に深度を消してある）。
	int start = 0;
	for (int look = 0; look < RING_OUT_WALL_LOOK_COUNT; ++look) {
		ID3D11ShaderResourceView* texture = m_Textures[look];
		if (texture && counts[look] > 0) {
			context->PSSetShaderResources(0, 1, &texture);
			context->Draw(counts[look], start);
		}
		start += counts[look];
	}
	ID3D11ShaderResourceView* empty = nullptr;
	context->PSSetShaderResources(0, 1, &empty);
	Shader_SetLightEnable(true);
}
