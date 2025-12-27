#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

#include "plugdefine.h"

namespace MinMax
{
	// ============================================================================================
	// 指数的な変化をする自作パラメータークラス
	// ============================================================================================
	class ExpParameter
		: public Parameter
	{
	private:
		ParamValue min; // パラメーターの表示値の最小
		ParamValue max; // パラメーターの表示値の最大

	public:
		// コンストラクタ(継承元メンバー変数などの初期化を行うためほぼ必須)
		ExpParameter(
			const TChar* title,
			ParamID tag,
			const TChar* units = 0,
			ParamValue minPlain = 0.0,
			ParamValue maxPlain = 1.0,
			int32 flags = ParameterInfo::kCanAutomate,
			UnitID unitID = kRootUnitId
		);

		// 正規化された値(0.0f～1.0fの値)を表示値の文字列にする関数(必須)
		virtual void toString(ParamValue valueNormalized, String128 string) const;

		// 表示されている文字列から正規化された値(0.0f～1.0fの値)を取得する関数(必須)
		virtual bool fromString(const TChar* string, ParamValue& valueNormalized) const;

		// 正規化された値(0.0f～1.0fの値)から表示されている値にする関数(必須)
		virtual ParamValue toPlain(ParamValue valueNormalized) const;

		// 表示されている値から正規化された値(0.0f～1.0fの値)にする関数(必須)
		virtual ParamValue toNormalized(ParamValue plainValue) const;

		// Parameterクラスの各種設定を自作パラメータークラス置き換えるマクロ(必須)
		OBJ_METHODS(ExpParameter, Parameter)
	};
}
