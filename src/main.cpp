#include "pch.h"
#include "Config.h"
#include "Mechanics.h"
#include "UI.h"
#include "IIF_API.h"
#include <fstream>
#include <nlohmann/json.hpp>

#define PLUGIN_NAME "WeaponRequirementsFramework"
#define PLUGIN_VERSION_MAJOR 1
#define PLUGIN_VERSION_MINOR 2
#define PLUGIN_VERSION_PATCH 0

namespace WRF {
    bool IsModEnabled(std::monostate) { return Config::GetSingleton()->bModEnabled; }
    float GetEquippedTotalRequirement(std::monostate, RE::Actor* a_actor) {
        RE::TESObjectWEAP* w = nullptr; RE::TBO_InstanceData* i = nullptr;
        return Mechanics::GetEquippedMainWeapon(a_actor, w, i) ? Mechanics::CalculateRequirement(a_actor, w, i, true) : 0.0f;
    }
    float GetEquippedBaseRequirement(std::monostate, RE::Actor* a_actor) {
        RE::TESObjectWEAP* w = nullptr; RE::TBO_InstanceData* i = nullptr;
        return Mechanics::GetEquippedMainWeapon(a_actor, w, i) ? Mechanics::CalculateRequirement(a_actor, w, i, false) : 0.0f;
    }
    std::int32_t GetAmmoRequirementValue(std::monostate, RE::TESForm* a_ammo) { return Mechanics::GetAmmoRequirement(a_ammo); }
    float GetActorStrengthDeficit(std::monostate, RE::Actor* a_actor) { return Mechanics::GetStrengthDeficit(a_actor); }
    bool IsEquippedHeavyWeapon(std::monostate, RE::Actor* a_actor) {
        RE::TESObjectWEAP* w = nullptr; RE::TBO_InstanceData* i = nullptr;
        return Mechanics::GetEquippedMainWeapon(a_actor, w, i) ? Mechanics::IsHeavyWeapon(w, i) : false;
    }
    std::int32_t GetEquippedPAWeaponState(std::monostate, RE::Actor* a_actor) {
        RE::TESObjectWEAP* w = nullptr; RE::TBO_InstanceData* i = nullptr;
        return Mechanics::GetEquippedMainWeapon(a_actor, w, i) ? Mechanics::GetPAWeaponState(w, i) : 0;
    }

    bool RegisterPapyrus(RE::BSScript::IVirtualMachine* vm) {
        const char* cls = "WRF_Native";
        vm->BindNativeMethod(cls, "IsModEnabled", IsModEnabled);
        vm->BindNativeMethod(cls, "GetEquippedTotalRequirement", GetEquippedTotalRequirement);
        vm->BindNativeMethod(cls, "GetEquippedBaseRequirement", GetEquippedBaseRequirement);
        vm->BindNativeMethod(cls, "GetAmmoRequirementValue", GetAmmoRequirementValue);
        vm->BindNativeMethod(cls, "GetActorStrengthDeficit", GetActorStrengthDeficit);
        vm->BindNativeMethod(cls, "IsEquippedHeavyWeapon", IsEquippedHeavyWeapon);
        vm->BindNativeMethod(cls, "GetEquippedPAWeaponState", GetEquippedPAWeaponState);
        return true;
    }

    // Reload settings only on PauseMenu close (MCM is accessed through it)
    class PauseMenuWatcher : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
    public:
        static PauseMenuWatcher* GetSingleton() { static PauseMenuWatcher s; return &s; }
        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& e, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
            if (!e.opening && e.menuName == "PauseMenu") {
                Config::GetSingleton()->LoadAllSettings();
                if (auto t = F4SE::GetTaskInterface())
                    t->AddTask([]() { Mechanics::RefreshStatus(RE::PlayerCharacter::GetSingleton()); });
            }
            return RE::BSEventNotifyControl::kContinue;
        }
    };
}

static bool g_hooksInstalled = false;

namespace {
    bool g_iifProviderRegistered = false;

    struct ItemUICardLayout
    {
        int priority{ 1 };
        std::string anchorTarget{ "CND" };
        std::string anchorMode{ "before" };
        std::string fallbackAnchorTarget{ "$ammo" };
        std::string fallbackAnchorMode{ "after" };
    };

    ItemUICardLayout LoadItemUICardLayout()
    {
        ItemUICardLayout layout;
        constexpr const char* path = "Data\\F4SE\\Plugins\\Weapon Requirements Framework\\ItemUICards.json";
        try {
            std::ifstream file(path);
            if (!file.is_open()) return layout;

            auto root = nlohmann::json::parse(file, nullptr, true, true);
            if (!root.is_object()) return layout;

            const nlohmann::json* card = nullptr;
            if (root.contains("ItemUICards") && root["ItemUICards"].is_object()) {
                auto& cards = root["ItemUICards"];
                if (cards.contains("$StrengthReq") && cards["$StrengthReq"].is_object()) card = &cards["$StrengthReq"];
            }
            if (!card && root.contains("$StrengthReq") && root["$StrengthReq"].is_object()) {
                card = &root["$StrengthReq"];
            }
            if (!card) return layout;

            layout.priority = card->value("priority", layout.priority);
            layout.anchorTarget = card->value("anchorTarget", layout.anchorTarget);
            layout.anchorMode = card->value("anchorMode", layout.anchorMode);
            layout.fallbackAnchorTarget = card->value("fallbackAnchorTarget", layout.fallbackAnchorTarget);
            layout.fallbackAnchorMode = card->value("fallbackAnchorMode", layout.fallbackAnchorMode);
        }
        catch (const std::exception& e) {
            REX::WARN("[WRF-IIF] Failed to load ItemUICards.json: {}", e.what());
        }
        catch (...) {
            REX::WARN("[WRF-IIF] Failed to load ItemUICards.json.");
        }
        return layout;
    }

    void RegisterIIFProvider()
    {
        if (g_iifProviderRegistered) return;

        auto messaging = F4SE::GetMessagingInterface();
        if (!messaging) return;

        REX::INFO("[WRF-IIF] Provider registration begin.");

        static ItemUICardLayout layout = LoadItemUICardLayout();
        IIF_API::CPPCardRegistration reg;
        reg.id = "$StrengthReq";
        reg.defaultPriority = layout.priority;
        reg.defaultAnchorTarget = layout.anchorTarget.c_str();
        reg.defaultAnchorMode = layout.anchorMode.c_str();
        reg.fallbackAnchorTarget = layout.fallbackAnchorTarget.c_str();
        reg.fallbackAnchorMode = layout.fallbackAnchorMode.c_str();

        IIF_API::ProviderConfig cfg;
        cfg.cards = { reg };
        cfg.damageModifier = WRF::Mechanics::OnCombatDamageCalculate;

        IIF_API::RegisterCardProvider(messaging, WRF::UI::OnIIFMessage, cfg);
        g_iifProviderRegistered = true;

        REX::INFO("[WRF-IIF] Provider registration end.");
    }
}

void OnF4SEMessage(F4SE::MessagingInterface::Message* a_msg) {
    if (!a_msg) return;

    if (a_msg->type == F4SE::MessagingInterface::kPostLoad) {
        RegisterIIFProvider();
    }
    else if (a_msg->type == F4SE::MessagingInterface::kGameLoaded) {
        WRF::Config::GetSingleton()->LoadAllSettings();
        WRF::Config::GetSingleton()->LoadForms();
        if (auto ui = RE::UI::GetSingleton()) {
            ui->GetEventSource<RE::MenuOpenCloseEvent>()->RegisterSink(WRF::PauseMenuWatcher::GetSingleton());
        }
    }
    else if (a_msg->type == F4SE::MessagingInterface::kPostLoadGame || a_msg->type == F4SE::MessagingInterface::kNewGame) {
        WRF::Config::GetSingleton()->LoadAllSettings();
        if (!g_hooksInstalled) {
            WRF::Mechanics::InstallHooks();
            g_hooksInstalled = true;
        }
        WRF::Mechanics::QueuePostLoadRefresh();
    }
}

namespace OGSupport {
    static F4SE::Impl::F4SEInterface RestoreLoadInterface;
    [[nodiscard]] inline static const char* F4SEAPI F4SEGetSaveFolderName() noexcept { return "Fallout4"; }
    void Init(const F4SE::LoadInterface* a_f4se) {
        if (a_f4se->RuntimeVersion() <= F4SE::RUNTIME_1_10_163) {
            memcpy(&RestoreLoadInterface, a_f4se, 48);
            (((F4SE::Impl::F4SEInterface*)(&RestoreLoadInterface))->GetSaveFolderName) = F4SEGetSaveFolderName;
            F4SE::Init((const F4SE::LoadInterface*)(&RestoreLoadInterface));
        }
        else F4SE::Init(a_f4se);
    }
}

extern "C" __declspec(dllexport) bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info) {
    if (!a_f4se || !a_info) return false;
    a_info->infoVersion = F4SE::PluginInfo::kVersion;
    a_info->name = PLUGIN_NAME;
    a_info->version = PLUGIN_VERSION_MAJOR;
    if (a_f4se->IsEditor()) return false;
    return true;
}

extern "C" __declspec(dllexport) bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se) {
    static std::once_flag once;
    std::call_once(once, [&]() {
        OGSupport::Init(a_f4se);
        REL::GetTrampoline().create(128);

        if (auto msg = F4SE::GetMessagingInterface()) {
            msg->RegisterListener(OnF4SEMessage);
            REX::INFO("[WRF] F4SE message listener registered.");
        }

        if (auto pap = F4SE::GetPapyrusInterface()) {
            pap->Register(WRF::RegisterPapyrus);
        }
        });
    return true;
}
