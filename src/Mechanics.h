#pragma once
#include <string>

// 👑 前置声明
namespace RE { class Actor; class TESObjectWEAP; class TBO_InstanceData; class TESForm; }

namespace WRF::Mechanics
{
    struct SkillReqInfo {
        bool hasReq{ false };
        bool isPerk{ false };
        std::string name;
        int reqValue{ 0 };
        float deficit{ 0.0f };
        std::string icon;
        int maxRank{ 0 };
    };

    struct DamageMults {
        float strMult{ 1.0f };
        float skillMult{ 1.0f };
    };

    // --- 核心计算 ---
    SkillReqInfo GetSkillRequirement(RE::Actor* a_actor, RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance);
    float CalculateRequirement(RE::Actor* a_actor, RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance, bool a_includeAmmo = true);
    float GetStrengthDeficit(RE::Actor* a_actor, RE::TESObjectWEAP* a_weapon = nullptr, RE::TBO_InstanceData* a_instance = nullptr);
    float GetWeaponWeight(RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance);
    DamageMults CalculateDamageMults(RE::Actor* a_actor, RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance);
    int GetAmmoRequirement(RE::TESForm* a_item);
    int GetWeaponCategory(RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance);
    int GetPAWeaponState(RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance);
    bool IsHeavyWeapon(RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance);

    // --- 工具函数 ---
    bool GetEquippedMainWeapon(RE::Actor* a_actor, RE::TESObjectWEAP*& a_weapon, RE::TBO_InstanceData*& a_instance);
    bool GetEquippedThrownWeapon(RE::Actor* a_actor, RE::TESObjectWEAP*& a_weapon, RE::TBO_InstanceData*& a_instance);
    bool IsPlayerInPowerArmor(RE::Actor* a_actor);
    std::string GetFallbackIcon(RE::TESForm* a_form);
    void DumpWeaponDebugInfo(RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance);

    // --- 状态控制与钩子 ---
    void InstallHooks();
    void QueuePostLoadRefresh();
    void RefreshStatus(RE::Actor* a_actor);

    // 👑 必须补上的战斗总线回调函数声明
    void OnCombatDamageCalculate(RE::Actor* attacker, RE::TESObjectWEAP* weapon, float* damagePtr);
}