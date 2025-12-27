#pragma once

#include <optional>
#include <memory>
#include <vector>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <base/source/fstring.h>
#include <public.sdk/source/vst/utility/stringconvert.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <pluginterfaces/base/ustring.h>
#include <base/source/fobject.h>

namespace ParameterFramework
{
    using namespace Steinberg;
    using namespace Steinberg::Vst;

    using OptionList = std::vector<std::string>;

#pragma region Custom Parameter

    class ExpParameter : public Parameter
    {
    private:
        ParamValue min;
        ParamValue max;

    public:
        ExpParameter(const TChar* title, ParamID tag, const TChar* units,
            ParamValue minPlain, ParamValue maxPlain,
            int32 flags, UnitID unitID)
            : Parameter(title, tag, units, 0.0, 0, flags, unitID),
            min(minPlain), max(maxPlain)
        {
            setPrecision(0);
        }

        void toString(ParamValue valueNormalized, String128 string) const
        {
            UString128 wrapper;
            wrapper.printFloat(toPlain(valueNormalized), precision);
            wrapper.copyTo(string, 128);
        }

        bool fromString(const TChar* string, ParamValue& valueNormalized) const
        {
            UString wrapper((TChar*)string, strlen16(string));
            ParamValue plainValue;
            if (wrapper.scanFloat(plainValue))
            {
                valueNormalized = toNormalized(plainValue);
                return true;
            }
            return false;
        }

        ParamValue toPlain(ParamValue valueNormalized) const
        {
            valueNormalized = std::clamp(valueNormalized, 0.0, 1.0);
            return ((max - min) * std::pow(valueNormalized, 3.0)) + min;
        }

        ParamValue toNormalized(ParamValue plainValue) const
        {
            ParamValue norm = (plainValue - min) / (max - min);
            norm = std::clamp(norm, 0.0, 1.0);
            return std::pow(norm, 1.0 / 3.0);
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
        SYS_BYPASS = ParameterInfo::kIsBypass | ParameterInfo::kCanAutomate,
        AUTOMATE = ParameterInfo::kCanAutomate,
        HIDDEN = ParameterInfo::kCanAutomate,
    };

#pragma endregion

#pragma region Container

    struct ValueRange
    {
        double minValue = 0.0;
        double maxValue = 1.0;

        // true の場合、この Int は index として扱われる
        bool isEnumerated = false;
    };

    struct IRangeResolver
    {
        virtual ~IRangeResolver() = default;
        virtual bool resolve(uint32 rangeKind, ValueRange& out) const = 0;
    };

    struct IOptionProvider
    {
        virtual ~IOptionProvider() = default;

        // 必ず1つ以上返すこと
        // 未定義の場合はダミー要素を1つ返す
        virtual OptionList getOptionNames(int32 rangeKind) const = 0;
    };

    struct ParamDef
    {
        ParamID tag;
        String name;
        String units;
        VALUE valueType;
        SCALE scaleType = SCALE::Linear;

        std::optional<uint32> rangeKind;

        FLAG flags;
        
        UnitID unitID;

        // minValue, maxValue, defaultValueはすべてplain値
        // rangeKindの値が存在する場合は上書きされる
        double minValue = 0.0;
        double maxValue = 1.0;

        //
        double defaultValue = 0.0;
        int32 precision = 0;
 
        int32 defaultIndex = 0;
    };

    class PFContainer
    {
    public:
        static PFContainer& get()
        {
            static PFContainer instance;
            return instance;
        }

        void setKindResolver(const IRangeResolver* r) { rangeResolver = r; }

        void setOptionProvider(const IOptionProvider* p) { optionProvider = p; }

        void addDef(ParamDef def)
        {
            defs.push_back(def);
        }

        const std::vector<ParamDef>& getDefs() const { return defs; }

        std::unique_ptr<Parameter> createParameter(const ParamDef& def)
        {
            if (!rangeResolver || !optionProvider) return nullptr;

            VALUE vtype = def.valueType;

            double defaultValue = def.defaultValue;
            int32 precision = def.precision;

            double min = def.minValue;
            double max = def.maxValue;
            bool isEnumerated = false;

            OptionList options;

            ValueRange rs;
            if (rangeResolver && def.rangeKind.has_value() && rangeResolver->resolve(*def.rangeKind, rs))
            {
                min = rs.minValue;
                max = rs.maxValue;
                isEnumerated = rs.isEnumerated;
            }

            if (defaultValue < min || defaultValue > max)
            {
                defaultValue = min;
            }

            std::unique_ptr<Parameter> param;

            if (isEnumerated && def.valueType != VALUE::Bool)
            {   // Bool は enumerated を想定しない
                auto p =
                    std::make_unique<StringListParameter>(
                        def.name,
                        def.tag,
                        def.units,
                        def.flags,
                        def.unitID
                    );

                auto options = optionProvider->getOptionNames(*def.rangeKind);
                if (options.empty())
                {
                    // 最低限ダミーを1つ
                    options.push_back("-");
                }

                for (auto& s : options)
                {
                    String128 u16str;
                    VST3::StringConvert::convert(s, u16str);
                    p->appendString(u16str);
                }               

                int32 index = std::clamp(def.defaultIndex, 0, static_cast<int32>(options.size() - 1));

                ParamValue norm =
                    options.size() > 1
                    ? static_cast<ParamValue>(index) / static_cast<ParamValue>(options.size() - 1)
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
                        std::make_unique<Parameter>
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
                        std::make_unique<RangeParameter>(
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
                            std::make_unique<RangeParameter>(
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

    private:

        // Resolver
        const IRangeResolver* rangeResolver = nullptr;
        const IOptionProvider* optionProvider = nullptr;

        //
        std::vector<ParamDef> defs{};
    };

#pragma endregion

#pragma region Value Storage

    struct ParamEntry
    {
        ParamValue normalized = 0.0;
        bool changed = false;
    };

    class ProcessorParamStorage
    {
    public:

        ProcessorParamStorage() = default;

        // PFContainer の定義で初期化
        void initialize()
        {
            const auto& defs = PFContainer::get().getDefs();
            storage.clear();
            for (const auto& def : defs)
            {
                ParamEntry entry{};
                entry.normalized = def.scaleType == SCALE::Exponential
                    ? ExpParameter(def.name, def.tag, def.units,
                        def.minValue, def.maxValue,
                        def.flags, def.unitID)
                    .toNormalized(def.defaultValue)
                    : (def.defaultValue - def.minValue) / (def.maxValue - def.minValue);

                entry.changed = false;
                storage.emplace(def.tag, entry);
            }
        }

        const std::unordered_map<ParamID, ParamEntry>& getAll() const { return storage; }

        // 正規化値の取得/設定
        ParamValue getNormalized(ParamID id) const
        {
            auto it = storage.find(id);
            return (it != storage.end()) ? it->second.normalized : 0.0;
        }

        void setNormalized(ParamID id, ParamValue val)
        {
            auto it = storage.find(id);
            if (it != storage.end())
            {
                val = std::clamp(val, 0.0, 1.0);
                if (std::abs(it->second.normalized - val) > 1e-6)
                {
                    it->second.normalized = val;
                    it->second.changed = true;
                }
            }
        }

        // プレーン値の取得/設定
        double getPlain(ParamID id) const
        {
            auto it = storage.find(id);
            if (it == storage.end()) return 0.0;

            const auto& def = findDef(id);
            if (!def) return 0.0;

            return def->scaleType == SCALE::Exponential
                ? ExpParameter(def->name, def->tag, def->units,
                    def->minValue, def->maxValue,
                    def->flags, def->unitID)
                .toPlain(it->second.normalized)
                : it->second.normalized * (def->maxValue - def->minValue) + def->minValue;
        }

        void setPlain(ParamID id, double val)
        {
            const auto* def = findDef(id);
            if (!def) return;

            double normalized = def->scaleType == SCALE::Exponential
                ? ExpParameter(def->name, def->tag, def->units,
                    def->minValue, def->maxValue,
                    def->flags, def->unitID)
                .toNormalized(val)
                : (val - def->minValue) / (def->maxValue - def->minValue);

            setNormalized(id, normalized);
        }

        bool isChanged(ParamID id) const
        {
            auto it = storage.find(id);
            return (it != storage.end()) ? it->second.changed : false;
        }

        void clearChangedFlags()
        {
            for (auto& [id, entry] : storage)
                entry.changed = false;
        }

    private:

        std::unordered_map<ParamID, ParamEntry> storage;

        const ParamDef* findDef(ParamID id) const
        {
            const auto& defs = PFContainer::get().getDefs();

            auto it = std::find_if(defs.begin(), defs.end(), [id](const ParamDef& d) { return d.tag == id; });

            return (it != defs.end()) ? &(*it) : nullptr;
        }
    };

#pragma endregion
}
