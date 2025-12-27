#pragma once

#include <pluginterfaces/base/ustring.h>
#include <algorithm>   // std::clamp
#include <cmath>       // pow

#include "ExpParameter.h"
#include "plugdefine.h"

namespace MinMax
{
	// ============================================================================================
	// コンストラクタ(継承元メンバー変数などの初期化を行うためほぼ必須)
	// ============================================================================================
	ExpParameter::ExpParameter(
		const TChar* title,
		ParamID tag,
		const TChar* units,
		ParamValue minPlain,
		ParamValue maxPlain,
		int32 flags,
		UnitID unitID
	)
		// 一部の引数は継承元クラスにそのまま渡し、初期化する
		// stepCount = 0 : 連続値パラメータ
		: Parameter(title, tag, units, 0.0, 0, flags, unitID)
	{
		// 最小値、最大値を設定
		min = minPlain;
		max = maxPlain;
	}

	// ============================================================================================
	// 正規化された値(0.0f～1.0fの値)を表示値の文字列にする関数
	// ============================================================================================
	void ExpParameter::toString(ParamValue valueNormalized, String128 string) const
	{
		// 正規化された値を文字列にするにはUString128クラスを使う
		// UString128クラスは内部に文字列用バッファ(長さ128)を持ったクラス
		// 詳細は割愛するが、文字列をいろいろ変換するための関数が用意されている
		UString128 wrapper;

		// printFloat()関数はfloatの値を文字列にし、wrapperの内部文字列用バッファに保存する
		// 正規化された値なので、toPlain()関数を使い表示値に変換してから保存する。
		// precisionは少数第何位まで表示するかを示す変数(継承元クラスで定義済み)
		wrapper.printFloat(toPlain(valueNormalized), precision);

		// 内部文字列用バッファからstringにコピーする
		wrapper.copyTo(string, 128);
	}

	// ============================================================================================
	// 表示されている文字列から正規化された値(0.0f～1.0fの値)を取得する関数
	// ============================================================================================
	bool ExpParameter::fromString(const TChar* string, ParamValue& valueNormalized) const
	{
		// 表示値の文字列から正規化値を取得にするにはUString128クラスを使う
		// まず、wrapperの内部文字列用バッファにstringを設定する
		UString wrapper((TChar*)string, strlen16(string));

		// 表示値の文字列から表示値を取得する
		ParamValue plainValue;
		if (wrapper.scanFloat(plainValue))
		{
			// 表示値取得に成功したら、toNormalized()関数で正規化し、valueNormalizedに代入
			// ★ 修正：誤って valueNormalized を再変換していたバグを修正
			valueNormalized = toNormalized(plainValue);

			// 変換に成功したらtrueを返す
			return true;
		}

		// 変換に失敗したらfalseを返す
		return false;
	}

	// ============================================================================================
	// 正規化された値(0.0f～1.0fの値)から表示されている値にする関数
	// ============================================================================================
	ParamValue ExpParameter::toPlain(ParamValue valueNormalized) const
	{
		// 正規化値を表示値に変換して返す
		// toNormalized()関数とtoPlain()関数で可逆であることが必要
		// 指数関数としてはとりあえずvalueNormalizedを3乗する関数にしておく(適当)
		// あわせて、最小値～最大値の範囲に収めるよう計算する。

		// ★ 修正：Hostから範囲外の値が来る可能性があるためclamp
		valueNormalized = std::clamp(valueNormalized, 0.0, 1.0);

		return ((max - min) * std::pow(valueNormalized, 3.0)) + min;
	}

	// ============================================================================================
	// 表示されている値から正規化された値(0.0f～1.0fの値)にする関数
	// ============================================================================================
	ParamValue ExpParameter::toNormalized(ParamValue plainValue) const
	{
		// 表示値を正規化値に変換して返す
		// toNormalized()関数とtoPlain()関数で可逆であることが必要
		// toPlain()関数では3乗しているので、plainValueの3乗根を計算する

		// ★ 修正：正規化前に範囲チェックを行う
		ParamValue norm = (plainValue - min) / (max - min);
		norm = std::clamp(norm, 0.0, 1.0);

		return std::pow(norm, 1.0 / 3.0);
	}
}
