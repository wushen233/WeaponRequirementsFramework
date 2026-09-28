#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace RE
{
    class TESBoundObject;
    class ExtraDataList;
    class TBO_InstanceData;
    class TESObjectWEAP;
}

namespace WRF::Classification
{
    struct MatchedRule
    {
        std::string ruleID;
        std::string categoryID;
        std::string exclusiveGroup;
        int priority{ 0 };
        bool instanceMatch{ false };
    };

    struct Result
    {
        std::vector<std::string> categories;
        std::unordered_map<std::string, std::string> selectedByGroup;
        std::vector<MatchedRule> diagnostics;

        [[nodiscard]] bool Has(std::string_view a_category) const;
        [[nodiscard]] std::optional<std::string> GetSelected(std::string_view a_group) const;
    };

    void Load();
    Result Evaluate(RE::TESBoundObject* a_object, RE::ExtraDataList* a_extraList);
    Result Evaluate(RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance);
    bool HasCategory(RE::TESBoundObject* a_object, RE::ExtraDataList* a_extraList, std::string_view a_category);
    bool HasCategory(RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance, std::string_view a_category);
    bool HasAnyCategory(RE::TESBoundObject* a_object, RE::ExtraDataList* a_extraList, const std::vector<std::string>& a_categories);
    bool HasAnyCategory(RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance, const std::vector<std::string>& a_categories);
    std::optional<std::string> GetCategoryInGroup(RE::TESBoundObject* a_object, RE::ExtraDataList* a_extraList, std::string_view a_group);
}
