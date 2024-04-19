#pragma once
#include <cmath> //C++

#include "cassert"

/// <summary>
/// 3次元ベクトル
/// </summary>
struct Vector3 final {
	float x;
	float y;
	float z;
};

/// <summary>
/// 4x4行列
/// </summary>
struct Matrix4x4 final {
	float m[4][4];
};

struct Vector3Transform {

	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;

};

/*-----------------------------------------------------------------------*/
//
//								4x4
//
/*-----------------------------------------------------------------------*/


// 1. 行列の加法
Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);

// 2. 行列の減法
Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);

// 3. 行列の積
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

// 4. 逆行列
Matrix4x4 Inverse(const Matrix4x4& m);

// 5. 転置行列
Matrix4x4 Transpose(const Matrix4x4& m);

// 6. 単位行列の生成
Matrix4x4 MakeIdentity4x4();

// 1.平行移動行列
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

// 2.拡大縮小行列
Matrix4x4 MakeScaleMatrix(const Vector3& scale);

// 3.座標変換
Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);

// 1 X軸回転行列
Matrix4x4 MakeRotateXMatrix(float radian);

// 2 Y軸回転行列
Matrix4x4 MakeRotateYMatrix(float radian);

// 3 Z軸回転行列
Matrix4x4 MakeRotateZMatrix(float radian);

// アフィン変換行列
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);


// 1.透視投射行列
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);
// 2.正射影行列
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

//　3.ビューポート行列
Matrix4x4 MakeViewportMatrix(float left, float top, float width,
	float height, float minDepth, float maxDepth);

//クロス積
Vector3 Cross(const Vector3& v1, const Vector3& v2);


/*-----------------------------------------------------------------------*/
//
//								3次元ベクトル
//
/*-----------------------------------------------------------------------*/

// ３次元ベクトルの値を表示する
void VectorScreenPrintf(int x, int y, const Vector3& vector, const char* label);
// 加算
Vector3 Add(const Vector3& v1, const Vector3& v2);
// 減算
Vector3 Subtract(const Vector3& v1, const Vector3& v2);
// スカラー倍
Vector3 Multiply(float scalar, const Vector3& v);
// 内積
float Dot(const Vector3& v1, const Vector3& v2);
// 長さ
float Length(const Vector3& v);
// 正規化
Vector3 Normalize(const Vector3& v);
