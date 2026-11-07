#pragma once
#include "core/dx11.h"
#include "render/animcalc.h"
#include <string>
#include <unordered_map>

/// @brief メカのクリップ番号
enum class SKINNED_CLIP
{
	Idle, ///< 待機
	Run, ///< 走り
	Left, ///< 左腕を前に出す
	Right, ///< 右腕を前に出す
	Count ///< クリップの数
};

/// @brief Fighter ごとに持つ描画専用のアニメ状態
struct SKINNED_POSE
{
	SKINNED_CLIP Clip = SKINNED_CLIP::Idle; ///< 再生中のクリップ
	float Time = 0.0f; ///< クリップ内の秒
	float BlendElapsed = 0.0f; ///< 切り替えからの秒
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

	/// @brief 列挙の順に4つのFBXを読み、待機のFBXからモデルを作る。成功したら true
	bool Load(const std::array<const char*, static_cast<size_t>(SKINNED_CLIP::Count)>& paths);
	/// @brief このモデルとクリップを解放する
	void Finalize();
	/// @brief 描画できるモデルを読み込めたか取得する
	bool IsLoaded() const { return !m_Meshes.empty(); }
	/// @brief 待機の先頭の姿勢に戻す
	void Reset(SKINNED_POSE& pose) const;
	/// @brief 指定されたクリップへ切り替え、ゲームの経過秒で姿勢を進める
	void Update(SKINNED_POSE& pose, SKINNED_CLIP clip, float dt, float blendSeconds) const;
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
		std::vector<ANIM_TRACK> Tracks;
	};

	using NodeNames = std::unordered_map<std::string, size_t>;

	bool LoadFiles(const std::array<const char*, static_cast<size_t>(SKINNED_CLIP::Count)>& paths);
	void ReadNodes(const aiNode& source, int parent, NodeNames& names, std::vector<size_t>& meshNodes);
	static bool ReadMesh(const aiMesh& source, size_t meshNode, const NodeNames& names, Mesh& mesh);
	bool ReadClip(const aiScene& scene, const NodeNames& names, Clip& clip) const;
	void SamplePose(SKINNED_POSE& pose, float blend) const;

	std::vector<Node> m_Nodes;
	std::vector<Mesh> m_Meshes;
	std::array<Clip, static_cast<size_t>(SKINNED_CLIP::Count)> m_Clips;
};
