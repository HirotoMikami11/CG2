#pragma once
#include "MyMath.h"

///xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx//

///																		///
///								構造体
///																		///

///xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx//


/// <summary>
/// スフィア
/// </summary>
struct Sphere {
	Vector3 center;		//!<中心点
	float radius;		//!<半径
};

/// <summary>
/// 直線
/// </summary>
struct Line {
	Vector3 origin;	//!<始点
	Vector3 diff;	//!<終点への差分ベクトル
};

/// <summary>
/// 半直線
/// </summary>
struct Ray {
	Vector3 origin;	//!<始点
	Vector3 diff;	//!<終点への差分ベクトル
};

/// <summary>
/// 線分
/// </summary>
struct Segment {
	Vector3 origin;	//!<始点
	Vector3 diff;	//!<終点への差分ベクトル
};

/// <summary>
/// 平面
/// </summary>
struct Plane {
	Vector3 normal;	//!<法線
	float distance;	//!<距離
};

/// <summary>
/// 三角形
/// </summary>
struct Triangle {
	Vector3 vertices[3];//!<頂点
};

///xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx//

///																		///
///								計算関数
///																		///

///xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx//


/// <summary>
/// 正射影ベクトルを求める
/// </summary>
/// <param name="v1"></param>
/// <param name="v2"></param>
/// <returns></returns>
Vector3 Project(const Vector3& v1, const Vector3& v2);

/// <summary>
/// 最近接点を求める関数
/// </summary>
/// <param name="point"></param>
/// <param name="segment"></param>
/// <returns></returns>
Vector3 ClosestPoint(const Vector3& point, const Segment& segment);

/// <summary>
/// 垂直なベクトルを求める
/// </summary>
/// <param name="vector"></param>
/// <returns></returns>
Vector3 Perpendicular(const Vector3& vector);

/// <summary>
/// 球体と球体の当たり判定を求める関数
/// </summary>
/// <param name="s1">球体1</param>
/// <param name="s2">球体2</param>
/// <returns></returns>
bool IsCollision(const Sphere& s1, const Sphere& s2);

/// <summary>
/// 球体と平面の衝突判定
/// </summary>
/// <param name="sphere">球体</param>
/// <param name="plane">平面</param>
/// <returns></returns>
bool IsCollision(const Sphere& sphere, const Plane& plane);

/// <summary>
/// 平面と線分の衝突判定
/// </summary>
/// <param name="segment">線分</param>
/// <param name="plane">平面</param>
/// <returns></returns>
bool IsCollision(const Segment& segment, const Plane& plane);

/// <summary>
/// 線分と平面の衝突判定（三角形の衝突判定を作る用）
/// </summary>
/// <param name="segment">線分</param>
/// <param name="normal">平面の法線</param>
/// <param name="distance">平面と原点距離</param>
/// <returns></returns>
bool IsCollision(const Segment& segment, const Vector3& normal, const float& distance);

/// <summary>
/// 平面と線分の衝突点を求める
/// </summary>
/// <param name="segment">線分</param>
/// <param name="normal">平面の法線</param>
/// <param name="distance">平面と原点距離</param>
/// <returns></returns>
Vector3 MakeCollisionPoint(const Segment& segment, const Vector3& normal, const float& distance);

/// <summary>
/// 三角形の衝突判定
/// </summary>
/// <param name="segment">線分</param>
/// <param name="triangle">三角形</param>
/// <returns></returns>
bool IsCollision(const Segment& segment, const Triangle& triangle);

///xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx//

///																		///
///								描画関数
///																		///

///xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx//


/// <summary>
/// 3Dで線を引く関数
/// </summary>
/// <param name="segment"></param>
/// <param name="viewProjectionMatrix"></param>
/// <param name="viewportMatrix"></param>
/// <param name="color"></param>
void DrawLine(const Segment& segment, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color);


/// <summary>
/// グリッド線を引く関数
/// </summary>
/// <param name="viewProjectionMatrix">ビュー行列*プロジェクション行列</param>
/// <param name="viewportMatrix">ビューポート行列</param>
void DrawGrid(const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix);

/// <summary>
/// スフィアを作る関数
/// </summary>
/// <param name="sphere">スフィア</param>
/// <param name="viewProjectionMatrix">ビュープロジェクション</param>
/// <param name="viewportMatrix">ビューポート</param>
/// <param name="color"></param>
void DrawSphere(const Sphere& sphere, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix,
	uint32_t color);

/// <summary>
/// 平面を作る関数
/// </summary>
/// <param name="plane"></param>
/// <param name="viewProjectionMatrix"></param>
/// <param name="viewportMatrix"></param>
/// <param name="color"></param>
void DrawPlane(const Plane& plane, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color);


/// <summary>
/// 三角形を描画
/// </summary>
/// <param name="triangle"></param>
/// <param name="viewProjectionMatrix"></param>
/// <param name="viewportMatrix"></param>
/// <param name="color"></param>
void DrawTriangle(const Triangle& triangle, const Matrix4x4& viewProjectionMatrix,
	const Matrix4x4& viewportMatrix, uint32_t color);

