#include "pch.h"
#include "ClassificationManager.h"
#include "FormResolver.h"
#include "ConfigReader.h"

#include <algorithm>
#include <mutex>
#include <nlohmann/json.hpp>

namespace WRF::Classification
{
    using json = nlohmann::json;

    struct Rule
    {
        std::string id;
        std::string group;
        int priority{ 0 };
        std::vector<RE::TESObjectWEAP*> baseForms;
        std::vector<RE::BGSKeyword*> baseKeywords;
        std::vector<RE::BGSKeyword*> baseAllKeywords;
        std::vector<RE::BGSKeyword*> instanceKeywords;
        std::vector<RE::BGSKeyword*> instanceAllKeywords;
        std::vector<RE::BGSKeyword*> noneKeywords;
    };

    std::mutex g_mutex;
    std::vector<Rule> g_rules;

    static RE::BGSKeyword* ResolveKeyword(const json& node)
    {
        if (!node.is_string()) return nullptr;
        const auto value = node.get<std::string>();
        if (value.empty()) return nullptr;
        return ResolveIdentifier<RE::BGSKeyword>(value);
    }

    static void ReadKeywords(const json& node, std::vector<RE::BGSKeyword*>& output)
    {
        if (!node.is_array()) return;
        for (const auto& item : node) if (auto* keyword = ResolveKeyword(item)) output.push_back(keyword);
    }

    static void ReadRule(const json& node, const std::string& fallbackID, std::vector<Rule>& output)
    {
        if (!node.is_object()) return;
        Rule rule;
        rule.id = node.value("id", node.value("category", fallbackID));
        if (rule.id.empty()) return;
        rule.group = node.value("exclusiveGroup", "");
        rule.priority = node.value("priority", 0);
        const auto match = node.contains("forms") || node.contains("keywords") || node.contains("allKeywords") ||
            node.contains("instanceKeywords") || node.contains("instanceAllKeywords") || node.contains("noneKeywords")
            ? node : node.value("match", node.value("conditions", json::object()));
        ReadKeywords(match.value("keywords", json::array()), rule.baseKeywords);
        ReadKeywords(match.value("allKeywords", json::array()), rule.baseAllKeywords);
        ReadKeywords(match.value("instanceKeywords", json::array()), rule.instanceKeywords);
        ReadKeywords(match.value("instanceAllKeywords", json::array()), rule.instanceAllKeywords);
        ReadKeywords(match.value("noneKeywords", json::array()), rule.noneKeywords);
        if (match.contains("forms") && match["forms"].is_array())
            for (const auto& form : match["forms"])
                if (form.is_string())
                    if (auto* weapon = ResolveIdentifier<RE::TESObjectWEAP>(form.get<std::string>())) rule.baseForms.push_back(weapon);
        if (!rule.baseKeywords.empty() || !rule.baseAllKeywords.empty() ||
            !rule.instanceKeywords.empty() || !rule.instanceAllKeywords.empty() || !rule.baseForms.empty())
            output.push_back(std::move(rule));
    }

    static std::string DefaultExclusiveGroup(std::string_view category)
    {
        if (category.starts_with("Weapon.Handling.")) return "Weapon.Handling";
        if (category.starts_with("Weapon.Configuration.")) return "Weapon.Configuration";
        return {};
    }

    static json MakeCategoryRule(const json& source, const std::string& category)
    {
        if (!source.is_object()) return json::object();
        json node = source;
        node["id"] = category;
        if (!node.contains("exclusiveGroup") || !node["exclusiveGroup"].is_string()) {
            const auto group = DefaultExclusiveGroup(category);
            if (group.empty()) node.erase("exclusiveGroup");
            else node["exclusiveGroup"] = group;
        }
        return node;
    }

    void Load()
    {
        std::scoped_lock lock(g_mutex);
        g_rules.clear();
        const std::filesystem::path directory("Data\\F4SE\\Plugins\\Weapon Requirements Framework\\Classifications");
        std::error_code ec;
        if (!std::filesystem::exists(directory)) std::filesystem::create_directories(directory, ec);
        ConfigReader::ForEachJsonInDirectory(directory, [](const json& root, const auto&) {
            if (root.value("module", "") != "weaponClassifications") return;
            if (root.contains("categories") && root["categories"].is_object()) {
                for (const auto& [category, value] : root["categories"].items()) {
                    if (value.is_object()) {
                        ReadRule(MakeCategoryRule(value, category), category, g_rules);
                    } else if (value.is_array()) {
                        for (const auto& item : value) {
                            if (item.is_object()) ReadRule(MakeCategoryRule(item, category), category, g_rules);
                        }
                    }
                }
            }
            if (root.contains("rules") && root["rules"].is_array())
                for (const auto& item : root["rules"]) ReadRule(item, "", g_rules);
        }, true);
        REX::INFO("[WRF Classification] Loaded {} category rules.", g_rules.size());
    }

    static bool HasAny(RE::BGSKeywordForm* form, const std::vector<RE::BGSKeyword*>& keywords)
    {
        if (!form) return false;
        for (auto* keyword : keywords) if (keyword && form->HasKeyword(keyword)) return true;
        return false;
    }

    static bool HasAll(RE::BGSKeywordForm* form, const std::vector<RE::BGSKeyword*>& keywords)
    {
        if (keywords.empty()) return true;
        if (!form) return false;
        for (auto* keyword : keywords) if (!keyword || !form->HasKeyword(keyword)) return false;
        return true;
    }

    static bool MatchesSelectors(
        RE::BGSKeywordForm* form,
        bool hasFormSelectors,
        bool formMatch,
        const std::vector<RE::BGSKeyword*>& anyKeywords,
        const std::vector<RE::BGSKeyword*>& allKeywords)
    {
        const bool hasAnySelector = hasFormSelectors || !anyKeywords.empty();
        const bool hasAllSelector = !allKeywords.empty();
        if (!hasAnySelector && !hasAllSelector) return false;

        const bool anyMatch = !hasAnySelector || formMatch || HasAny(form, anyKeywords);
        return anyMatch && HasAll(form, allKeywords);
    }

    static Result EvaluateRules(RE::TESObjectWEAP* weapon, RE::BGSKeywordForm* base, RE::BGSKeywordForm* instance)
    {
        Result result;
        if (!weapon) return result;

        std::unordered_map<std::string, MatchedRule> selected;
        for (const auto& rule : g_rules) {
            if (HasAny(base, rule.noneKeywords) || HasAny(instance, rule.noneKeywords)) continue;

            const bool formMatch = std::find(rule.baseForms.begin(), rule.baseForms.end(), weapon) != rule.baseForms.end();
            const bool baseMatch = MatchesSelectors(base, !rule.baseForms.empty(), formMatch, rule.baseKeywords, rule.baseAllKeywords);
            const bool instanceMatch = MatchesSelectors(instance, false, false, rule.instanceKeywords, rule.instanceAllKeywords);
            if (!baseMatch && !instanceMatch) continue;

            MatchedRule hit{ rule.id, rule.id, rule.group, rule.priority, instanceMatch };
            if (rule.group.empty()) result.diagnostics.push_back(hit);
			else if (const auto it = selected.find(rule.group); it == selected.end() || hit.priority > it->second.priority ||
				(hit.priority == it->second.priority && hit.instanceMatch != it->second.instanceMatch && hit.instanceMatch) ||
				(hit.priority == it->second.priority && hit.instanceMatch == it->second.instanceMatch && hit.categoryID < it->second.categoryID)) selected[rule.group] = hit;
        }

        for (const auto& hit : result.diagnostics) result.categories.push_back(hit.categoryID);
        for (const auto& [group, hit] : selected) {
            result.categories.push_back(hit.categoryID);
            result.selectedByGroup[group] = hit.categoryID;
            result.diagnostics.push_back(hit);
        }
        std::sort(result.categories.begin(), result.categories.end());
        result.categories.erase(std::unique(result.categories.begin(), result.categories.end()), result.categories.end());
        return result;
    }

    Result Evaluate(RE::TESBoundObject* object, RE::ExtraDataList* extra)
    {
        std::scoped_lock lock(g_mutex);
        auto* weapon = object ? object->As<RE::TESObjectWEAP>() : nullptr;
        (void)extra;
        return EvaluateRules(weapon, weapon ? weapon->As<RE::BGSKeywordForm>() : nullptr, nullptr);
    }

    Result Evaluate(RE::TESObjectWEAP* weapon, RE::TBO_InstanceData* instanceData)
    {
        std::scoped_lock lock(g_mutex);
        auto* instance = instanceData ? static_cast<RE::TESObjectWEAP::InstanceData*>(instanceData)->keywords : nullptr;
        return EvaluateRules(weapon, weapon ? weapon->As<RE::BGSKeywordForm>() : nullptr, instance);
    }

    bool Result::Has(std::string_view category) const { return std::find(categories.begin(), categories.end(), category) != categories.end(); }
    std::optional<std::string> Result::GetSelected(std::string_view group) const {
        const auto it = selectedByGroup.find(std::string(group));
        return it == selectedByGroup.end() ? std::nullopt : std::optional<std::string>{ it->second };
    }
    bool HasAnyCategory(const Result& result, const std::vector<std::string>& categories) {
        return std::any_of(categories.begin(), categories.end(), [&](const auto& category) { return result.Has(category); });
    }
    bool HasCategory(RE::TESBoundObject* object, RE::ExtraDataList* extra, std::string_view category) { return Evaluate(object, extra).Has(category); }
    bool HasCategory(RE::TESObjectWEAP* weapon, RE::TBO_InstanceData* instance, std::string_view category) { return Evaluate(weapon, instance).Has(category); }
    bool HasAnyCategory(RE::TESBoundObject* object, RE::ExtraDataList* extra, const std::vector<std::string>& categories) {
        return HasAnyCategory(Evaluate(object, extra), categories);
    }
    bool HasAnyCategory(RE::TESObjectWEAP* weapon, RE::TBO_InstanceData* instance, const std::vector<std::string>& categories) {
        return HasAnyCategory(Evaluate(weapon, instance), categories);
    }
    std::optional<std::string> GetCategoryInGroup(RE::TESBoundObject* object, RE::ExtraDataList* extra, std::string_view group) { return Evaluate(object, extra).GetSelected(group); }
}
