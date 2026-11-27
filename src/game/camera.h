#pragma once

#include "core/dx11.h"
#include "physics/cameracalc.h"

class Stage;
class Fighter;
class FighterManager;
class ArmManager;
class Match;

/// @file camera.h
/// @brief TPS カメラ（ハードロック・ソフトロック・フリーの3つ。F / R3 で順に切り替える）
/// @details
///   - ハードロック：カメラが相手を向き続ける。ソフトロック：カメラは自由に回せ、画面に見えている相手を狙う。フリー：狙わない
///   - 開幕は正面の敵。左右の弾き入力で次の敵へ切り替え、角度・注視点をバネで追う
///   - ロックオン中は上下だけ手動で回せる。保持中・敵がいないときは左右も自由に回せる
///   - 自由カメラでは試合中の横移動について回り、自分で回したあとは少し止める
///   - 注視点の左右には遊びがあり、ジャンプ中は最後に立っていた高さを保つ。場外への落下は追う
///   - 浮島にめり込むときはすぐ近づき、離れるときはバネで戻る。復活・やり直しでは速度を0にしてすぐ合わせる
///   - ロックオン中は肩越しに右へ寄せ（キャラは画面の左）、左右へ動く間はキャラを画面の端へ流す（AC6 風）
///   - レーザーを溜めている間は、どのカメラでも肩越しに右へ寄せて近づける（ソフトロックは狙う相手を固定する）
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

	/// @brief 描く範囲の大きさを設定する（縦横比と WorldToScreen に使う。はじめは画面全体）
	/// @param width 幅（ピクセル）
	/// @param height 高さ（ピクセル）
	void SetViewSize(float width, float height);

	/// @brief 状態を最初に戻す（参照はそのまま）
	void Reset(void);

	/// @brief ハードロックのカメラに戻す（オンラインを始めるとき。相手とカメラの状態をそろえる）
	void ResetFreeLook(void) { m_Mode = CAMERA_MODE_HARD; m_ModeSeconds = MODE_SHOW_NEVER; }

	/// @brief カメラを自由に回せる状態か取得する（true：ソフトロック・フリー、false：ハードロック）
	bool IsFreeLook(void) const { return m_Mode != CAMERA_MODE_HARD; }

	/// @brief 今のカメラの状態（ハードロック・ソフトロック・フリー）を取得する
	/// @return カメラの状態
	CAMERA_MODE GetMode(void) const { return m_Mode; }

	/// @brief 状態を切り替えてからの秒数を取得する（切り替えた直後に状態の名前を大きく出すのに使う）
	/// @return 秒数（試合の始めなど、まだ切り替えていなければ大きな値）
	float GetModeSeconds(void) const { return m_ModeSeconds; }

	/// @brief カメラの終了処理（今は何もしない）
	void Finalize(void);

	/// @brief ターゲット・手動入力・横移動から目標を作り、角度・距離・注視点を別々のバネで追う
	/// @details 毎フレーム、FighterManager::Update のあとに呼ぶ（動いたあとの自分の体を追いかける）。浮島にめり込むときは手前まで近づける
	void Update(void);

	/// @brief 画面の揺れ（画ぶれ）を足す（描画だけ。向き・入力の向きは変えない）
	/// @param strength 足す強さ（0〜1。重ねても上限 1）
	void AddShake(float strength) { m_Shake.Add(strength); }

	/// @brief ビュー行列とプロジェクション行列をシェーダーに設定する
	/// @details 3D描画の前に呼ぶ
	void Draw(void) const;

	/// @brief ワールド座標の点が、画面のどこに映るかを取得する（頭の上の % の表示に使う）
	/// @param world 点（ワールド座標）
	/// @param[out] screen 描く範囲の中の位置（範囲の左上が原点のピクセル。範囲の外になることもある）
	/// @return 点がカメラの前にあれば true、後ろ（映らない）なら false（そのとき screen は変えない）
	bool WorldToScreen(const XMFLOAT3& world, XMFLOAT2* screen) const;

	/// @brief カメラの位置を取得する
	/// @return カメラの位置（ワールド座標）
	XMFLOAT3 GetPosition(void) const;

	/// @brief カメラの左右の向きを取得する（プレイヤーの移動の向きに使う）
	/// @return 左右の向き（ラジアン。0 で +Z 方向、プラスで右回り）
	float GetYaw(void) const;

	/// @brief 操作で決めたカメラの上下の向きを取得する（ナックルの空中の突進・モーニングスターの投げの角度に使う）
	/// @details 低い地形を越えるために画面の向きを下げている分は含めない（画面の向きは GetDirection）
	/// @return 上下の向き（ラジアン。プラスで上を見る）
	float GetPitch(void) const { return m_Pitch; }

	/// @brief 現在狙っている相手（ハードロック・ソフトロック）のスロット番号を取得する
	/// @return スロット番号（0〜3。自分を除く）。ターゲットなし・フリー・敵をつかんでいる間は-1
	int GetTarget(void) const;

	/// @brief その相手をロックオンできるかを取得する（CPU が、カメラで狙えない相手を選ばないために使う）
	/// @param slot 相手のスロット番号（0〜3）
	/// @return true：ロックオンできる、false：自分・復活待ち・場外へ落ちている・隠れている相手
	bool CanLockOn(int slot) const;

private:
	VEC3 PlayerHead(void) const;
	void CollectTargets(CAMERA_FOCUS_TARGET* targets) const;
	bool IsTargetAvailable(int target, const CAMERA_FOCUS_TARGET* targets) const;
	int PickFrontTarget(const CAMERA_FOCUS_TARGET* targets) const;
	VEC3 LockFocusPoint(const VEC3& player, const CAMERA_FOCUS_TARGET* targets) const;
	void UpdateLockTarget(const CAMERA_FOCUS_TARGET* targets);
	int PickSoftTarget(const CAMERA_FOCUS_TARGET* targets) const; // ソフトロック：画面に見えている相手を選ぶ
	void PlaceCamera(float dt, float desiredDistance, bool snap);
	float GetViewPitch(void) const; // 見せる上下の向き（操作で決める m_Pitch に、地形を越えるために見下ろす分を足したもの）
	void UpdateWaiting(float dt); // 復活待ちの間、浮島の高さへ戻して場外にした相手を映す
	void SnapToPlayer(void);
	void MakeMatrices(XMMATRIX* view, XMMATRIX* projection) const;
	XMFLOAT3 m_Position = {};		// カメラの位置
	CameraShakeCalc m_Shake;		// 画面の揺れ（最後のカメラの位置に上乗せする。描画だけ）
	VEC3 m_Target = {};			// 今の見ている点
	VEC3 m_TargetGoal = {};		// 遊び・接地した高さを反映した、見ている点の目標
	VEC3 m_TargetVelocity = {};	// 見ている点のバネの速度（軸ごとに持つ）
	float m_Yaw = {};				// 左右の向き（ラジアン。0 で +Z 方向を見る）
	float m_Pitch = {};			// 上下の向き（ラジアン。プラスで上を見る）
	float m_YawGoal = {};			// 入力を足す左右の目標角度
	float m_PitchGoal = {};		// 入力を足す上下の目標角度
	float m_YawVelocity = {};		// 左右角のバネの速度
	float m_PitchVelocity = {};	// 上下角のバネの速度
	float m_ViewPitchOffset = {};			// 低い地形を上から越えるために、見せる角度を下げている量（ラジアン。0 以下）
	float m_ViewPitchOffsetVelocity = {};	// 見せる角度を下げる量のバネの速度
	float m_Distance = {};			// 今のカメラまでの距離
	float m_DistanceVelocity = {};	// 距離のバネの速度
	float m_ManualPause = {};		// 横移動について回るのを止める残り秒数
	float m_SideOffset = {};		// 肩越しにカメラを右へずらしている量（マイナスで左）
	float m_SideVelocity = {};		// 横へのずれのバネの速度
	float m_AimZoom = {};			// レーザーを溜めている間の肩越しの寄せ具合（0：ふつう、1：寄せきった）
	float m_AimZoomVelocity = {};	// 寄せ具合のバネの速度
	VEC3 m_PlayerGoal = {};			// 自由カメラ用の遊び・最後に立っていた高さ（敵の注視点と分ける）
	static constexpr float MODE_SHOW_NEVER = 1000.0f;	// まだ切り替えていないときの m_ModeSeconds
	CAMERA_MODE m_Mode = CAMERA_MODE_HARD;	// カメラの状態（ハードロック・ソフトロック・フリー）
	float m_ModeSeconds = MODE_SHOW_NEVER;	// 状態を切り替えてからの秒数
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
	float m_ViewWidth = (float)SCREEN_WIDTH;	// 描く範囲の幅
	float m_ViewHeight = (float)SCREEN_HEIGHT;	// 描く範囲の高さ
};
