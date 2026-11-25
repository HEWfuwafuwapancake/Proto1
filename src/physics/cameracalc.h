#pragma once

#include "physics/stagecalc.h"
#include "physics/vec3.h"

/// @file cameracalc.h
/// @brief TPS カメラの計算（位置・浮島との接触・臨界減衰バネ・No.33 用のフォーカス）
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief 見ている点のまわりを回るカメラの位置を出す
/// @details 見ている向きは (cos(pitch)·sin(yaw), sin(pitch), cos(pitch)·cos(yaw))。カメラはその反対へ distance 下がったところ
/// @param target 見ている点
/// @param yaw 左右の向き（ラジアン。0 で +Z 方向を見る、プラスで右回り）
/// @param pitch 上下の向き（ラジアン。プラスで上を見る）
/// @param distance 見ている点からカメラまでの距離
/// @return カメラの位置
VEC3 CameraCalcOrbitPosition(const VEC3& target, float yaw, float pitch, float distance);

/// @brief 見ている点からカメラへの線が浮島にぶつかるなら、ぶつかる手前までカメラを近づける
/// @details 浮島の箱を margin だけ大きくして調べる（カメラが面にぴったり付いて、近くのものが切れて見えないように）。
///          見ている点が浮島の箱の中にあるときは、近づけようがないので camera をそのまま返す
/// @param target 見ている点
/// @param camera カメラを置きたい位置
/// @param stage 浮島
/// @param margin 浮島の箱を大きくする量（0 以上）
/// @return めり込まない位置（ぶつからなければ camera のまま）
VEC3 CameraCalcClipToStage(const VEC3& target, const VEC3& camera, const STAGE& stage, float margin);

/// @brief 見ている点のまわりを回るカメラを、地形にめり込まずに置ける距離を出す
/// @details 地形のマップでは、見ている点からカメラへ向かう線を少しずつ調べ、実体の区画の上面＋margin より下に入る手前で止める。
///          見ている点から nearRadius 以内は margin を付けずに調べる（背中が低い段差に触れただけで寄らないように。壁の中には入れない）。
///          浮島（地形のないマップ）では CameraCalcClipToStage と同じ。
/// @param target 見ている点
/// @param yaw 左右の向き（ラジアン。0 で +Z 方向を見る、プラスで右回り）
/// @param pitch 上下の向き（ラジアン。プラスで上を見る）
/// @param distance 置きたい距離
/// @param stage 地形
/// @param margin 地形の面からカメラを離す量（0 以上）
/// @param nearRadius 見ている点から、margin を付けずに調べる距離（0 以上）
/// @return めり込まずに置ける距離（0〜distance。ぶつからなければ distance）
float CameraCalcClearDistance(const VEC3& target, float yaw, float pitch, float distance, const STAGE& stage, float margin, float nearRadius);

/// @brief 地形を上から越えるために、カメラの上下の向きをどこまで下げるか（見下ろす角度を深くするか）を出す
/// @details pitch から minPitch まで step ずつ下げ、distance のまま置ける最初の角度を返す。どれも置けなければ、いちばん遠くまで置ける角度
///          （同じなら pitch に近い方）を返す。浮島（地形のないマップ）では pitch のまま。
/// @param target 見ている点
/// @param yaw 左右の向き（ラジアン）
/// @param pitch 今の上下の向き（ラジアン。プラスで上を見る）
/// @param distance 置きたい距離
/// @param minPitch いちばん下を向く角度（ラジアン。pitch より大きければ pitch のまま）
/// @param step 角度を下げる刻み（ラジアン。0 以下なら pitch のまま）
/// @param stage 地形
/// @param margin 地形の面からカメラを離す量（0 以上）
/// @param nearRadius 見ている点から、margin を付けずに調べる距離（0 以上）
/// @return 地形を越えられる上下の向き（ラジアン。pitch 以下）
float CameraCalcClearPitch(const VEC3& target, float yaw, float pitch, float distance, float minPitch, float step,
	const STAGE& stage, float margin, float nearRadius);

/// @brief 臨界減衰バネで目標へ近づけ、行き過ぎるときは目標で止める
/// @details 速度は呼ぶ側で値ごとに持ち、目標が変わっても初期化せず次の呼び出しへ渡す
/// @param current 今の値
/// @param target 目標の値
/// @param[in,out] velocity 1秒あたりの速度（初回・瞬間移動時は0）
/// @param smoothTime 追いつくまでのおよその秒数（0以下は0.0001秒として扱う）
/// @param dt 1フレームの秒数（0以下なら値も速度も変えない）
/// @return 近づけたあとの値
float CameraCalcSmoothDamp(float current, float target, float& velocity, float smoothTime, float dt);

/// @brief 角度の差を最短方向に直し、臨界減衰バネで目標へ近づける
/// @param current 今の角度（ラジアン）
/// @param target 目標の角度（ラジアン）
/// @param[in,out] velocity 角速度（ラジアン/秒。初回・瞬間移動時は0）
/// @param smoothTime 追いつくまでのおよその秒数（0以下は0.0001秒として扱う）
/// @param dt 1フレームの秒数（0以下なら値も速度も変えない）
/// @return 近づけたあとの角度（-π〜π。dtが0以下ならcurrentのまま）
float CameraCalcSmoothDampAngle(float current, float target, float& velocity, float smoothTime, float dt);

/// @brief フォーカス候補（配列の添字がダミー番号）
struct CAMERA_FOCUS_TARGET
{
	VEC3 Position; ///< 敵の中心位置
	bool Available; ///< true：選べる、false：選べない
};

/// @brief from から to への水平角を返す（水平距離がほぼ0なら fallbackYaw）
/// @param from 向きの始点
/// @param to 向きの終点
/// @param fallbackYaw 水平距離がほぼ0のときの角度（ラジアン）
/// @return 水平角（ラジアン。0で+Z方向、プラスで右回り）
float CameraCalcYawTo(const VEC3& from, const VEC3& to, float fallbackYaw);
/// @brief yaw から maxAngle ラジアン・3D距離 maxDistance 以内の最も中央の敵を選ぶ。current は bonus ラジアン優先（なしは -1）
/// @param yaw カメラの水平角（ラジアン）
/// @param player プレイヤーの注視点
/// @param targets 候補の配列（countが0ならnullptrでもよい）
/// @param count 候補の数
/// @param current 今の候補番号（なしは-1）
/// @param maxAngle 中央から選べる角度（ラジアン）
/// @param maxDistance 選べる3D距離
/// @param bonus 今の候補を優先する角度（ラジアン）
/// @return 選んだ候補の添字（選べる敵がなければ-1）
int CameraCalcPickFocus(float yaw, const VEC3& player, const CAMERA_FOCUS_TARGET* targets,
	int count, int current, float maxAngle, float maxDistance, float bonus);

/// @brief 今のターゲットへの向きを基準に、指定した側で角度が最も近い敵へ切り替える
/// @param player プレイヤーの位置（高さは使わない）
/// @param targets 候補の配列（countが0ならnullptrでもよい）
/// @param count 候補の数
/// @param current 今のターゲット番号
/// @param direction 切り替える側（負：左、正：右、0：切り替えない）
/// @return 次の敵の添字。その側に敵がいなければcurrent、currentが無効・選択不可なら-1
int CameraCalcNextTarget(const VEC3& player, const CAMERA_FOCUS_TARGET* targets,
	int count, int current, int direction);

/// @brief player から enemy へ rate の割合だけ視点をずらす。縦横の視野とプレイヤーを囲む半径で上限を決める
/// @param player プレイヤーの注視点（頭付近）
/// @param enemy 敵の中心
/// @param yaw 左右角（ラジアン）
/// @param pitch 上下角（ラジアン）
/// @param distance カメラ距離
/// @param fov 縦視野角（ラジアン）
/// @param aspect 画面の横幅/高さ
/// @param margin 画面端に残す割合（0〜1、各半画面に対する割合）
/// @param radius 注視点を中心にプレイヤー全身を囲む半径
/// @param rate 敵へずらす割合（0〜1）
/// @return プレイヤー全身が画面に収まる範囲で敵へずらした注視点
VEC3 CameraCalcFocusPoint(const VEC3& player, const VEC3& enemy, float yaw, float pitch,
	float distance, float fov, float aspect, float margin, float radius, float rate);

/// @brief 肩越しにカメラを横へずらす量の目標を出す
/// @details ロックオン中は shoulder だけ右へ寄せ（キャラは画面の左へ寄る）、フリーカメラ・相手がいないときは真ん中に戻す。
///          左右へ動いている間は、動く向きと反対へカメラを残し、キャラだけ画面の端へ流す
/// @param lockOn 相手にロックオンしているか（true：横へ寄せる、false：真ん中に戻す）
/// @param moveX 左右の移動入力（-1〜1。プラスで右。範囲の外は-1〜1に収める）
/// @param shoulder ロックオン中にカメラを右へ寄せる量（ワールドの長さ）
/// @param drift 左右へいっぱいに動いたとき、キャラを動く向きへ流す量（ワールドの長さ）
/// @return カメラを右へずらす量（マイナスで左）
float CameraCalcSideOffsetGoal(bool lockOn, float moveX, float shoulder, float drift);

/// @brief 点をカメラの右の向き（水平）へずらす
/// @param point ずらす前の点
/// @param yaw カメラの左右の向き（ラジアン。0 で +Z 方向を見る、プラスで右回り）
/// @param offset ずらす量（プラスで右、マイナスで左）
/// @return ずらしたあとの点（高さは変えない）
VEC3 CameraCalcSideShift(const VEC3& point, float yaw, float offset);

/// @brief カメラの状態（右スティック押し込み・F で順に切り替える）
enum CAMERA_MODE
{
	CAMERA_MODE_HARD,	///< ハードロック：カメラが相手を向き続ける（左右の弾きで相手を切り替える）
	CAMERA_MODE_SOFT,	///< ソフトロック：カメラは自由に回せ、画面に見えている相手を狙う
	CAMERA_MODE_FREE,	///< フリー：カメラは自由に回せ、相手を狙わない
	CAMERA_MODE_COUNT,	///< 状態の数
};

/// @brief 次のカメラの状態を返す（ハード → ソフト → フリー → ハードの順）
/// @param mode 今の状態
/// @return 次の状態
CAMERA_MODE CameraCalcNextMode(CAMERA_MODE mode);

/// @brief 画面に映っている相手のうち、画面の真ん中にいちばん近い相手を選ぶ（ソフトロック）
/// @details 画面の端から margin の割合の内側に中心が入り、eye から maxDistance 以内の相手だけを選ぶ。
///          current がまだ映っていれば bonus（画面の半分の幅を1とした距離）だけ優先し、狙いがちらつかないようにする
/// @param eye カメラの位置
/// @param yaw カメラの左右の向き（ラジアン。0 で +Z 方向を見る、プラスで右回り）
/// @param pitch カメラの上下の向き（ラジアン。プラスで上を見る）
/// @param fov 縦の視野角（ラジアン）
/// @param aspect 画面の横幅 ÷ 高さ
/// @param margin 画面の端に残す割合（0〜1、各半画面に対する割合）
/// @param maxDistance 選ぶ相手までのいちばん遠い距離
/// @param targets 候補の配列（Available が false の相手は選ばない）
/// @param count 候補の数
/// @param current 今狙っている相手の添字（なしは -1）
/// @param bonus 今狙っている相手を優先する量（画面の半分の幅を1とした距離）
/// @return 選んだ候補の添字（映っている相手がいなければ -1）
int CameraCalcPickVisible(const VEC3& eye, float yaw, float pitch, float fov, float aspect, float margin,
	float maxDistance, const CAMERA_FOCUS_TARGET* targets, int count, int current, float bonus);
