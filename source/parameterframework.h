//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <algorithm>
#include <array>
#include <base/source/fobject.h>
#include <base/source/fstring.h>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <optional>
#include <pluginterfaces/base/fstrdefs.h>
#include <pluginterfaces/base/ftypes.h>
#include <pluginterfaces/base/ustring.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <pluginterfaces/vst/vsttypes.h>
#include <public.sdk/source/vst/utility/stringconvert.h>
#include <public.sdk/source/vst/vstparameters.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace PF
{
#pragma region Custom Parameter

    class ExpParameter : public Steinberg::Vst::Parameter
    {
    private:
        Steinberg::Vst::ParamValue min;
        Steinberg::Vst::ParamValue max;

    public:
        ExpParameter(
            const Steinberg::Vst::TChar* title,
            Steinberg::Vst::ParamID tag,
            const Steinberg::Vst::TChar* units,
            Steinberg::Vst::ParamValue minPlain,
            Steinberg::Vst::ParamValue maxPlain,
            Steinberg::int32 flags,
            Steinberg::Vst::UnitID unitID
        )
            : Parameter(title, tag, units, 0.0, 0, flags, unitID),
            min(minPlain), max(maxPlain)
        {
            setPrecision(0);
        }

        void toString(Steinberg::Vst::ParamValue valueNormalized, Steinberg::Vst::String128 string) const override
        {
            Steinberg::UString128 wrapper;
            wrapper.printFloat(toPlain(valueNormalized), precision);
            wrapper.copyTo(string, 128);
        }

        bool fromString(const Steinberg::Vst::TChar* string, Steinberg::Vst::ParamValue& valueNormalized) const override
        {
            Steinberg::UString wrapper((Steinberg::Vst::TChar*)string, Steinberg::strlen16(string));
            Steinberg::Vst::ParamValue plainValue;
            if (wrapper.scanFloat(plainValue))
            {
                valueNormalized = toNormalized(plainValue);
                return true;
            }
            return false;
        }

        Steinberg::Vst::ParamValue toNormalized(Steinberg::Vst::ParamValue plainValue) const override
        {
            assert(max != min && "ExpParameter: max and min cannot be equal");
            if (max == min) return 0.0; // 安全回避

            Steinberg::Vst::ParamValue norm = (plainValue - min) / (max - min);
            norm = std::clamp(norm, 0.0, 1.0);
            return std::pow(norm, 1.0 / 3.0);
        }

        Steinberg::Vst::ParamValue toPlain(Steinberg::Vst::ParamValue valueNormalized) const override
        {
            assert(max != min && "ExpParameter: max and min cannot be equal");
            if (max == min) return 0.0; // 安全回避

            valueNormalized = std::clamp(valueNormalized, 0.0, 1.0);
            return ((max - min) * std::pow(valueNormalized, 3.0)) + min;
        }

        OBJ_METHODS(ExpParameter, Parameter)
    };

#pragma endregion

#pragma region Enums

    enum class VALUE
    {
        Bool,
        Int,
        Real,
    };

    enum class SCALE
    {
        Linear,
        Exponential,
    };

    enum FLAG
    {
        SYS_BYPASS = Steinberg::Vst::ParameterInfo::kIsBypass | Steinberg::Vst::ParameterInfo::kCanAutomate,
        AUTOMATE = Steinberg::Vst::ParameterInfo::kCanAutomate,
        HIDDEN = Steinberg::Vst::ParameterInfo::kIsHidden,
    };

#pragma endregion

#pragma region Parameter Helper

    struct ValueRange
    {
        double minValue = 0.0;
        double maxValue = 1.0;
        bool hasOption = false;
    };

    struct IRangeResolver
    {
        virtual ~IRangeResolver() = default;
        virtual bool resolve(Steinberg::uint32 rangeKind, ValueRange& out) const = 0;
    };

    struct IOptionProvider
    {
        virtual ~IOptionProvider() = default;
        virtual std::vector<std::string> getOptionNames(Steinberg::int32 rangeKind) const = 0;
    };

    struct ParamDef
    {
        Steinberg::Vst::ParamID tag;
        Steinberg::String name;
        Steinberg::String units;
        VALUE valueType;
        SCALE scaleType = SCALE::Linear;

        std::optional<Steinberg::uint32> rangeKind;

        FLAG flags;
        
        Steinberg::Vst::UnitID unitID;

        double minValue = 0.0;
        double maxValue = 1.0;

        double defaultValue = 0.0;
        Steinberg::int32 precision = 0;
 
        Steinberg::int32 defaultIndex = 0;
    };

    class ParamHelper
    {
    public:
        static ParamHelper& get()
        {
            static ParamHelper instance;
            return instance;
        }

        void setKindResolver(const IRangeResolver* r) { rangeResolver = r; }

        void setOptionProvider(const IOptionProvider* p) { optionProvider = p; }

        std::unique_ptr<Steinberg::Vst::Parameter> createParameter(const ParamDef& def)
        {
            if (!rangeResolver || !optionProvider) return nullptr;

            VALUE vtype = def.valueType;

            double defaultValue = def.defaultValue;
            Steinberg::int32 precision = def.precision;

            double min = def.minValue;
            double max = def.maxValue;
            bool hasOption = false;

            std::vector<std::string> options;

            ValueRange rs;
            if (rangeResolver && def.rangeKind.has_value() && rangeResolver->resolve(*def.rangeKind, rs))
            {
                min = rs.minValue;
                max = rs.maxValue;
                hasOption = rs.hasOption;
            }

            if (defaultValue < min || defaultValue > max)
            {
                defaultValue = min;
            }

            std::unique_ptr<Steinberg::Vst::Parameter> param;

            if (hasOption && def.valueType != VALUE::Bool)
            {   // Bool は enumerated を想定しない
                auto p =
                    std::make_unique<Steinberg::Vst::StringListParameter>(
                        def.name,
                        def.tag,
                        def.units,
                        def.flags,
                        def.unitID
                    );

                auto options = optionProvider->getOptionNames(*def.rangeKind);

                if (options.empty())
                {   // 最低限ダミーを1つ
                    options.push_back("-");
                }

                for (auto& s : options)
                {
                    Steinberg::Vst::String128 u16str;
#pragma warning(disable : 4996)
                    VST3::StringConvert::convert(s, u16str);    // 非推奨となっているがVST SDKの対応待ち
                    p->appendString(u16str);
                }               

                Steinberg::int32 index = std::clamp(def.defaultIndex, 0, static_cast<Steinberg::int32>(options.size() - 1));

                Steinberg::Vst::ParamValue norm =
                    options.size() > 1
                    ? static_cast<Steinberg::Vst::ParamValue>(index) / static_cast<Steinberg::Vst::ParamValue>(options.size() - 1)
                    : 0.0;

                p->setNormalized(norm);

                param = std::move(p);
            }
            else
            {
                switch (vtype)
                {
                case VALUE::Bool:
                {
                    auto p =
                        std::make_unique<Steinberg::Vst::Parameter>
                        (
                            def.name,
                            def.tag,
                            def.units,
                            defaultValue,
                            1,
                            def.flags,
                            def.unitID
                        );

                    p->setPrecision(0);

                    param = std::move(p);

                    break;
                }
                case VALUE::Int:
                {
                    auto p =
                        std::make_unique<Steinberg::Vst::RangeParameter>(
                            def.name,
                            def.tag,
                            def.units,
                            min,
                            max,
                            defaultValue,
                            static_cast<int>(max - min),
                            def.flags,
                            def.unitID
                        );

                    param = std::move(p);
                    break;
                }
                case VALUE::Real:
                {
                    if (def.scaleType == SCALE::Linear)
                    {
                        auto p =
                            std::make_unique<Steinberg::Vst::RangeParameter>(
                                def.name,
                                def.tag,
                                def.units,
                                min,
                                max,
                                0,
                                0,
                                def.flags,
                                def.unitID
                            );
                        p->setPrecision(precision);
                        p->setNormalized(p->toNormalized(defaultValue));
                        param = std::move(p);
                    }
                    else
                    {
                        auto p =
                            std::make_unique<ExpParameter>(
                                def.name,
                                def.tag,
                                def.units,
                                min,
                                max,
                                def.flags,
                                def.unitID
                            );
                        p->setPrecision(precision);
                        p->setNormalized(p->toNormalized(defaultValue));
                        param = std::move(p);
                    }

                    break;
                }
                default:
                    break;
                }
            }

            return param;
        }
        
        bool resolveRange(const ParamDef& def, ValueRange& out) const
        {
            if (!rangeResolver || !def.rangeKind) return false;
            return rangeResolver->resolve(*def.rangeKind, out);
        }

    private:
        // Resolver
        const IRangeResolver* rangeResolver = nullptr;
        const IOptionProvider* optionProvider = nullptr;
    };

#pragma endregion

#pragma region Value Storage
 
    class ProcessorParamStorage
    {
    public:
        ProcessorParamStorage() = default;

        template <size_t N>
        void initialize(const std::array<ParamDef, N>& paramTable)
        {
            storage.clear();
            paramInstances.clear();

            for (const auto& def : paramTable)
            {
                // 正規化処理はパラメータメソッドを利用
                std::unique_ptr<Steinberg::Vst::Parameter> p = ParamHelper::get().createParameter(def);
                if (!p) continue;
                paramInstances.emplace(def.tag, std::move(p));

                // 正規化値初期化
                ParamEntry entry{};
                entry.current = entry.previous = getNormalized(def.tag, def.defaultValue);
                entry.changed = false;

                storage.emplace(def.tag, entry);
            }
        }

        double get(Steinberg::Vst::ParamID id) const
        {
            auto it = storage.find(id);
            if (it == storage.end()) return 0.0;

            auto* p = findParameter(id);
            if (!p) return 0.0;

            return p->toPlain(it->second.current);
        }

        double getPrevious(Steinberg::Vst::ParamID id) const
        {
            auto it = storage.find(id);
            if (it == storage.end()) return 0.0;

            auto* p = findParameter(id);
            if (!p) return 0.0;

            return p->toPlain(it->second.previous);
        }

        void set(Steinberg::Vst::ParamID id, double val)
        {
            Steinberg::Vst::ParamValue normalized = getNormalized(id, val);
            setNormalized(id, normalized);
        }

        void setNormalized(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue val)
        {
            auto it = storage.find(id);
            if (it == storage.end()) return;

            val = std::clamp(val, 0.0, 1.0);

            if (std::abs(it->second.current - val) > 1e-6)
            {
                it->second.previous = it->second.current;
                it->second.current = val;
                it->second.changed = true;
            }
        }

        Steinberg::Vst::ParamValue getNormalized(Steinberg::Vst::ParamID id, double plain) const
        {
            auto* p = findParameter(id);
            if (!p) return 0.0;
            return p->toNormalized(plain);
        }

        bool isChanged(Steinberg::Vst::ParamID id) const
        {
            auto it = storage.find(id);
            return (it != storage.end()) ? it->second.changed : false;
        }

        void clearChangedFlags(Steinberg::Vst::ParamID id)
        {
            auto it = storage.find(id);
            if (it == storage.end()) return;
            it->second.changed = false;
        }

    private:

        struct ParamEntry
        {
            Steinberg::Vst::ParamValue current = 0.0;
            Steinberg::Vst::ParamValue previous = 0.0;
            bool changed = false;
        };

        std::unordered_map<Steinberg::Vst::ParamID, ParamEntry> storage;
        std::unordered_map<Steinberg::Vst::ParamID, std::unique_ptr<Steinberg::Vst::Parameter>> paramInstances;

        Steinberg::Vst::Parameter* findParameter(Steinberg::Vst::ParamID id) const
        {
            auto it = paramInstances.find(id);
            return it != paramInstances.end() ? it->second.get() : nullptr;
        }
    };

#pragma endregion
}
