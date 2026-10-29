#pragma once

#include "core/dx11.h"
#include "physics/cameracalc.h"

class Stage;
class Fighter;
class FighterManager;
class ArmManager;
class Match;

/// @file camera.h
/// @brief TPS カメラ（敵へのロックオンと、保持中・敵がいないときの自由カメラ）
/// @details
///   - 開幕は正面の敵。左右の弾き入力で次の敵へ切り替え、角度・注視点をバネで追う
///   - ロックオン中は上下だけ手動で回せる。保持中・敵がいないときは左右も自由に回せる
///   - 自由カメラでは試合中の横移動について回り、自分で回したあとは少し止める
///   - 注視点の左右には遊びがあり、ジャンプ中は最後に立っていた高さを保つ。場外への落下は追う
///   - 浮島にめり込むときはすぐ近づき、離れるときはバネで戻る。復活・やり直しでは速度を0にしてすぐ合わせる
///   - 計算は physics/cameracalc.h

/// @brief ロックオンと自由カメラの状態・行列を管理する
class Camera
{
public:
	/// @brief 描画と同じ、上下を含むカメラの向きを取得する（長さ1）
	/// @return カメラの正面の単位ベクトル（ワールド座標）
	XMFLOAT3 GetDirection(void) const;


	/// @brief 正面の相手を選び、自分の体の後ろへ置く（FighterManager::Initializeのあと）
	void Initialize(const Stage& stage, const FighterManager& fighters, int slot, const ArmManager& arms, const Match& match);

	/// @brief 状態を最初に戻す（参照はそのまま）
	void Reset(void);

	/// @brief カメラの終了処理（今は何もしない）
	void Finalize(void);

	/// @brief ターゲット・手動入力・横移動から目標を作り、角度・距離・注視点を別々のバネで追う
	/// @details 毎フレーム、FighterManager::Update のあとに呼ぶ（動いたあとの自分の体を追いかける）。浮島にめり込むときは手前まで近づける
	void Update(void);

	/// @brief ビュー行列とプロジェクション行列をシェーダーに設定する
	/// @details 3D描画の前に呼ぶ
	void Draw(void) const;

	/// @brief ワールド座標の点が、画面のどこに映るかを取得する（頭の上の % の表示に使う）
	/// @param world 点（ワールド座標）
	/// @param[out] screen 画面の位置（左上が原点のピクセル。画面の外になることもある）
	/// @return 点がカメラの前にあれば true、後ろ（映らない）なら false（そのとき screen は変えない）
	bool WorldToScreen(const XMFLOAT3& world, XMFLOAT2* screen) const;

	/// @brief カメラの位置を取得する
	/// @return カメラの位置（ワールド座標）
	XMFLOAT3 GetPosition(void) const;

	/// @brief カメラの左右の向きを取得する（プレイヤーの移動の向きに使う）
	/// @return 左右の向き（ラジアン。0 で +Z 方向、プラスで右回り）
	float GetYaw(void) const;

	/// @brief 現在ロックオンしている相手のスロット番号を取得する
	/// @return スロット番号（0〜3。自分を除く）。ターゲットなし・敵をつかんでいる間は-1
	int GetTarget(void) const;

private:
	VEC3 PlayerHead(void) const;
	void CollectTargets(CAMERA_FOCUS_TARGET* targets) const;
	bool IsTargetAvailable(int target, const CAMERA_FOCUS_TARGET* targets) const;
	int PickFrontTarget(const CAMERA_FOCUS_TARGET* targets) const;
	VEC3 LockFocusPoint(const VEC3& player, const CAMERA_FOCUS_TARGET* targets) const;
	void UpdateLockTarget(const CAMERA_FOCUS_TARGET* targets);
	void PlaceCamera(float dt);
	void SnapToPlayer(void);
	void MakeMatrices(XMMATRIX* view, XMMATRIX* projection) const;
	XMFLOAT3 m_Position = {};		// カメラの位置
	VEC3 m_Target = {};			// 今の見ている点
	VEC3 m_TargetGoal = {};		// 遊び・接地した高さを反映した、見ている点の目標
	VEC3 m_TargetVelocity = {};	// 見ている点のバネの速度（軸ごとに持つ）
	float m_Yaw = {};				// 左右の向き（ラジアン。0 で +Z 方向を見る）
	float m_Pitch = {};			// 上下の向き（ラジアン。プラスで上を見る）
	float m_YawGoal = {};			// 入力を足す左右の目標角度
	float m_PitchGoal = {};		// 入力を足す上下の目標角度
	float m_YawVelocity = {};		// 左右角のバネの速度
	float m_PitchVelocity = {};	// 上下角のバネの速度
	float m_Distance = {};			// 今のカメラまでの距離
	float m_DistanceVelocity = {};	// 距離のバネの速度
	float m_ManualPause = {};		// 横移動について回るのを止める残り秒数
	VEC3 m_PlayerGoal = {};			// 自由カメラ用の遊び・最後に立っていた高さ（敵の注視点と分ける）
	int m_LockTarget = -1;		// 現在ロックオンしている相手のスロット番号
	int m_SavedTarget = -1;	// つかむ前のターゲット番号
	bool m_WasHolding = false;	// 1つ前のフレームで敵をつかんでいたか
	float m_SwitchRemaining = 0.0f;	// 切り替え用の速い角度のバネを使う残り秒数
	const Stage* m_Stage = nullptr;
	const Fighter* m_Player = nullptr;
	const FighterManager* m_Fighters = nullptr;
	const ArmManager* m_Arms = nullptr;
	const Match* m_Match = nullptr;
	int m_Slot = 0;
};
