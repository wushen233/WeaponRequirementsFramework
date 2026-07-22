#pragma once
#include <SimpleIni.h>
#include <nlohmann/json.hpp>

namespace WRF
{
	struct StrengthMapping {
		int priority{ 0 };
		bool isModifier{ false };
		bool isMultiplier{ false };
		float value{ 0.0f };
		std::vector<RE::TESForm*> weapons;
		std::vector<RE::BGSKeyword*> keywords;
		bool keywordMatchAnd{ false };
		float minWeight{ -1.0f };  // 重量范围下限（-1 = 不启用）
		float maxWeight{ -1.0f };  // 重量范围上限（-1 = 无上限）
	};

	struct AmmoWeightMapping {
		int priority{ 0 };
		float minWeight{ 0.0f };
		float maxWeight{ -1.0f };  // -1 = 无上限
		int value{ 0 };
	};

	struct SkillMapping {
		int priority{ 0 };
		float reqValue{ 0.0f };
		std::string icon;
		std::vector<RE::TESObjectWEAP*> weapons;
		std::vector<RE::BGSKeyword*> keywords;
		bool keywordMatchAnd{ false };
		std::vector<RE::ActorValueInfo*> skillAVs;
		std::vector<RE::BGSPerk*> perks;
	};

		class Config
	{
	public:
		static Config* GetSingleton();

		enum class StrengthCalcMode : std::uint32_t
		{
			kJsonRules = 0,      // 原有的 JSON 规则驱动（按武器/关键词匹配）
			kWeightBased = 1     // 基于重量范围的 JSON 规则驱动
		};

		enum class AmmoCalcMode : std::uint32_t
		{
			kFixedConfig = 0,    // 原有的固定配置驱动（按弹药类型匹配 JSON）
			kWeightBased = 1     // 基于弹药自身重量计算需求
		};

		enum class SkillCalcMode : std::uint32_t
		{
			kPerkBased = 0,         // 原版 Perk 检测（突击队、双枪侠等）
			kActorValueBased = 1    // 社区 0-100 角色值技能检测
		};

		void LoadAllSettings();
		void LoadForms();

		// --- 数据存储 ---
		std::unordered_map<std::uint32_t, int> CustomAmmoReqs;
		std::unordered_map<std::uint32_t, std::uint32_t> MSF_AmmoMapping;
		std::vector<SkillMapping> CustomSkillMappings;
		std::vector<StrengthMapping> CustomStrengthMappings;
		std::vector<AmmoWeightMapping> CustomAmmoWeightMappings;
		std::vector<std::string> IconPool;

		// --- MCM 配置变量 ---
		bool bModEnabled{ true };
		bool bDebugMode{ false };
		bool bShowUI{ true };
		int iUIDamageDisplayMode{ 0 };

		// ----- 力量计算模式选择 -----
		StrengthCalcMode iStrengthCalcMode{ StrengthCalcMode::kJsonRules };
		AmmoCalcMode iAmmoCalcMode{ AmmoCalcMode::kFixedConfig };
		SkillCalcMode iSkillCalcMode{ SkillCalcMode::kPerkBased };

		bool bGatedSprint{ true };
		bool bGatedRun{ false };
		int iGatedThreshold{ 2 };
		bool bPASoftGateEnabled{ true };
		int iPenaltyMode{ 0 };
		int iReq_AmmoBase{ 0 };
		bool bEnableAmmoReq{ true };
		bool bAmmoReqAlwaysActive{ false };
		bool bEnableCustomAmmoReq{ true };
		bool bAmmoReqCompensation{ true };
		bool bHeavyGunnerReduction{ true };
		float fSkillDamageBase{ 0.50f };
		float fSkillDamageMax{ 1.00f };
		float fSwayDynCoef{ 0.25f };
		float fGunDamageDynCoef{ 0.00f };
		float fThrownDamageDynCoef{ 0.00f };
		float fSpreadDynCoef{ 0.20f };
		float fMeleeStrPenaltyMult{ 0.15f };
		float fMeleeSkillPenaltyMult{ 0.10f };
		float fVATSAccuracyDynCoef{ 0.10f };
		float fDamageMinLimit{ 0.1f };

		// --- 游戏表单 (GameData) ---
		RE::TESGlobal* WRF_Message_PenaltyMode{ nullptr };
		RE::TESGlobal* WRF_InsufficientStrength{ nullptr };
		RE::TESGlobal* WRF_InsufficientSkill{ nullptr };
		RE::TESGlobal* WRF_PowerArmorWeaponState{ nullptr };
		RE::TESGlobal* WRF_MCM_StrengthDebuff_GatedSprint{ nullptr };
		RE::TESGlobal* WRF_MCM_StrengthDebuff_GatedRun{ nullptr };
		RE::TESGlobal* WRF_EquippedWeaponType{ nullptr };
		RE::BGSPerk* WRF_MainScript_Perk{ nullptr };
		RE::BGSPerk* PerkHeavyGunner1{ nullptr };
		RE::BGSPerk* PerkHeavyGunner2{ nullptr };
		RE::BGSPerk* PerkHeavyGunner3{ nullptr };
		RE::BGSPerk* PerkHeavyGunner4{ nullptr };
		RE::BGSPerk* PerkHeavyGunner5{ nullptr };
		RE::ActorValueInfo* WRF_WeaponRequirementValue{ nullptr };
		RE::ActorValueInfo* WRF_StrengthDeficit{ nullptr };
		RE::ActorValueInfo* WRF_Mult_ScopeStability{ nullptr };
		RE::ActorValueInfo* WRF_Mult_ConeOfFire{ nullptr };
		RE::ActorValueInfo* WRF_Mult_VATSAccuracy{ nullptr };
		RE::BGSSoundDescriptorForm* WRF_WeaponJamSound{ nullptr };

		RE::BGSKeyword* WeaponTypeHeavyGun{ nullptr };
		RE::BGSKeyword* WeaponTypePistol{ nullptr };
		RE::BGSKeyword* WeaponTypeCryolator{ nullptr };
		RE::BGSKeyword* WeaponTypeFlamer{ nullptr };
		RE::BGSKeyword* WeaponTypeJunkJet{ nullptr };
		RE::BGSKeyword* WeaponTypeMissileLauncher{ nullptr };
		RE::BGSKeyword* WeaponTypeMinigun{ nullptr };
		RE::BGSKeyword* WeaponTypeGatlingLaser{ nullptr };
		RE::BGSKeyword* WeaponTypeFatMan{ nullptr };
		RE::BGSKeyword* WeaponTypeBroadsider{ nullptr };
		RE::BGSKeyword* ObjectTypeAmmo{ nullptr };
		RE::BGSKeyword* isPowerArmorFrame{ nullptr };
		RE::BGSKeyword* FurnitureTypePowerArmor{ nullptr };
		RE::BGSKeyword* WRF_PASupport{ nullptr };
		RE::BGSKeyword* WeaponTypePowerArmor{ nullptr };

		//RE::BGSPerk* WRF_MeleeSpeedPerk{ nullptr };

		RE::BSFixedString strControl_Melee{ "Melee" };
		RE::BSFixedString strControl_PrimaryAttack{ "PrimaryAttack" };

	private:
		Config() = default;
		void LoadINI();
		void LoadMSFCompatibility();
		void LoadCustomAmmo();
		void LoadCustomSkills();
		void LoadCustomStrengths();
	};
}