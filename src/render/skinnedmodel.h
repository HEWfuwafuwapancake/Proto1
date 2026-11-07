#pragma once
#include "core/dx11.h"
#include "render/animcalc.h"
#include <string>
#include <unordered_map>

/// @brief 骨の名前で指定する、キャラごとの部位
struct SKINNED_BONES_DESC
{
	const char* UpperRoot; ///< 上半身の根元の骨（この骨と子孫だけに腕のクリップを重ねる。nullptr なら重ねない）
	std::array<const char*, 3> Twist; ///< 上半身のねじれを分けて足す骨（腰に近い順。使わない欄は nullptr）
};

/// @brief 1フレーム分の再生の指定
struct SKINNED_REQUEST
{
	SKINNED_CLIP Clip = SKINNED_CLIP::Idle; ///< 全身（下半身）で再生するクリップ
	float Speed = 1.0f; ///< 全身のクリップの再生速度の倍率
	SKINNED_CLIP Upper = SKINNED_CLIP::Count; ///< 上半身に重ねるクリップ（Count なら重ねない）
	float Twist = 0.0f; ///< 上半身のねじれ（ラジアン。XMMatrixRotationY と同じ向き）
};

/// @brief Fighter ごとに持つ描画専用のアニメ状態
struct SKINNED_POSE
{
	SKINNED_CLIP Clip = SKINNED_CLIP::Idle; ///< 再生中のクリップ
	float Time = 0.0f; ///< クリップ内の秒
	float BlendElapsed = 0.0f; ///< 切り替えからの秒
	SKINNED_CLIP Upper = SKINNED_CLIP::Count; ///< 上半身に重ねているクリップ（Count ならなし）
	float UpperTime = 0.0f; ///< 上半身のクリップ内の秒
	float UpperWeight = 0.0f; ///< 上半身のクリップの重み（0〜1）
	float Twist = 0.0f; ///< 上半身のねじれ（ラジアン）
	std::vector<ANIM_TRANSFORM> Local; ///< 現在の各ノードの姿勢
	std::vector<ANIM_TRANSFORM> BlendFrom; ///< 切り替え直前に描いていた姿勢
	std::vector<ANIM_MATRIX> Global; ///< 各ノードのモデル空間の行列
};

struct aiScene;
struct aiNode;
struct aiMesh;

/// @brief 1キャラ分のメッシュ・骨・アニメを持ち、複数の Fighter の描画に使う
class SkinnedModel
{
public:
	/// @brief 未読み込みのモデルを作る
	SkinnedModel() = default;
	SkinnedModel(const SkinnedModel&) = delete;
	SkinnedModel& operator=(const SkinnedModel&) = delete;

	/// @brief 列挙の順にFBXを読み、待機のFBXからモデルを作る。成功したら true
	/// @details 待機・走り・左腕・右腕は必須。後ろ走り・ジャンプ・落下・着地は nullptr か読めなければ代わりのクリップを使う
	/// @param paths クリップごとの FBX のパス
	/// @param bones 上半身の根元とねじる骨の名前
	bool Load(const std::array<const char*, static_cast<size_t>(SKINNED_CLIP::Count)>& paths, const SKINNED_BONES_DESC& bones);
	/// @brief このモデルとクリップを解放する
	void Finalize();
	/// @brief 描画できるモデルを読み込めたか取得する
	bool IsLoaded() const { return !m_Meshes.empty(); }
	/// @brief 待機の先頭の姿勢に戻す
	void Reset(SKINNED_POSE& pose) const;
	/// @brief 指定されたクリップへ切り替え、ゲームの経過秒で姿勢を進める
	/// @param pose この Fighter の姿勢
	/// @param request 全身・上半身のクリップと速さ・ねじれ
	/// @param dt ゲームの経過秒
	/// @param blendSeconds クリップを切り替えるときと、上半身のクリップを出し入れするときにつなぐ秒数
	void Update(SKINNED_POSE& pose, const SKINNED_REQUEST& request, float dt, float blendSeconds) const;
	/// @brief 骨（ノード）の番号を名前で探す
	/// @return ノード番号（見つからなければ -1）
	int FindNode(const char* name) const;
	/// @brief 姿勢の中の、ノードの原点のモデル空間の位置を取得する
	/// @param pose この Fighter の姿勢
	/// @param node FindNode で探したノード番号
	/// @param position 位置を入れる先
	/// @return 取得できたら true（番号が範囲外・姿勢が未計算なら false）
	bool GetNodePosition(const SKINNED_POSE& pose, int node, ANIM_VEC3& position) const;
	/// @brief 姿勢とワールド行列を使い、全体を指定色で描く
	void Draw(const SKINNED_POSE& pose, const XMMATRIX& world, const XMFLOAT4& color) const;

private:
	struct Node
	{
		int Parent;
		ANIM_TRANSFORM BindPose;
	};

	struct Bone
	{
		size_t NodeIndex;
		ANIM_MATRIX Offset;
	};

	struct Mesh
	{
		ID3D11Buffer* Vertices = nullptr;
		ID3D11Buffer* Indices = nullptr;
		UINT IndexCount = 0;
		std::vector<Bone> Bones;
	};

	struct Clip
	{
		float LoopSeconds = 0;
		bool Reverse = false; // 逆に再生する（後ろ走りの代わり）
		std::vector<ANIM_TRACK> Tracks;
	};

	using NodeNames = std::unordered_map<std::string, size_t>;

	bool LoadFiles(const std::array<const char*, static_cast<size_t>(SKINNED_CLIP::Count)>& paths, const SKINNED_BONES_DESC& bones);
	void ReadBones(const NodeNames& names, const SKINNED_BONES_DESC& bones);
	const Clip& GetClip(SKINNED_CLIP clip) const { return m_Clips[static_cast<size_t>(clip)]; }
	ANIM_TRANSFORM SampleClip(SKINNED_CLIP clip, float time, size_t node) const;
	void ReadNodes(const aiNode& source, int parent, NodeNames& names, std::vector<size_t>& meshNodes);
	static bool ReadMesh(const aiMesh& source, size_t meshNode, const NodeNames& names, Mesh& mesh);
	bool ReadClip(const aiScene& scene, const NodeNames& names, Clip& clip) const;
	void SamplePose(SKINNED_POSE& pose, float blend) const;

	std::vector<Node> m_Nodes;
	NodeNames m_NodeNames; // 骨の名前 → ノード番号
	std::vector<Mesh> m_Meshes;
	std::array<Clip, static_cast<size_t>(SKINNED_CLIP::Count)> m_Clips;
	std::vector<bool> m_UpperMask; // 上半身の骨か（ノードの番号順）
	std::vector<size_t> m_TwistNodes; // ねじる骨のノード番号
};
