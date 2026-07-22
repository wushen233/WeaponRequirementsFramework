#include "pch.h"
#include "Config.h"
#include <ConfigReader.h>
#include <fstream>

namespace WRF
{
	Config* Config::GetSingleton() {
		static Config singleton;
		return &singleton;
	}

	std::string Trim(const std::string& str) {
		std::string s = str;
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch) && ch != 0xEF && ch != 0xBB && ch != 0xBF; }));
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
		return s;
	}

	template <class T>
	T* ResolveIdentifier(const std::string& a_identifier) {
		auto dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler || a_identifier.empty()) return nullptr;
		if (size_t splitPos = a_identifier.find('|'); splitPos != std::string::npos) {
			std::string modName = Trim(a_identifier.substr(0, splitPos));
			try {
				std::uint32_t localID = std::stoul(Trim(a_identifier.substr(splitPos + 1)), nullptr, 16);
				auto form = dataHandler->LookupForm(static_cast<RE::TESFormID>(localID), modName);
				return form ? form->As<T>() : nullptr;
			}
			catch (...) { return nullptr; }
		}
		auto form = RE::TESForm::GetFormByEditorID(RE::BSFixedString(a_identifier));
		return form ? form->As<T>() : nullptr;
	}

	void Config::LoadAllSettings() {
		LoadINI();
		LoadMSFCompatibility();
		LoadCustomAmmo();
		LoadCustomSkills();
		LoadCustomStrengths();
	}

	void Config::LoadINI() {
		std::filesystem::path defaultPath = "Data\\MCM\\Config\\Weapon Requirements Framework\\settings.ini";
		std::filesystem::path userPath = "Data\\MCM\\Settings\\Weapon Requirements Framework.ini";
		CSimpleIniA defaultIni, userIni;
		defaultIni.SetUnicode(); defaultIni.SetSpaces(false);
		defaultIni.LoadFile(defaultPath.string().c_str());
		userIni.SetUnicode(); userIni.SetSpaces(false);
		userIni.LoadFile(userPath.string().c_str());

		auto getBool = [&](const char* sec, const char* key, bool def) {
			if (userIni.GetValue(sec, key)) return userIni.GetBoolValue(sec, key);
			if (defaultIni.GetValue(sec, key)) return defaultIni.GetBoolValue(sec, key);
			return def;
			};
		auto getInt = [&](const char* sec, const char* key, int def) {
			if (userIni.GetValue(sec, key)) return (int)userIni.GetLongValue(sec, key);
			if (defaultIni.GetValue(sec, key)) return (int)defaultIni.GetLongValue(sec, key);
			return def;
			};
		auto getFloat = [&](const char* sec, const char* key, float def) {
			if (userIni.GetValue(sec, key)) return (float)userIni.GetDoubleValue(sec, key);
			if (defaultIni.GetValue(sec, key)) return (float)defaultIni.GetDoubleValue(sec, key);
			return def;
			};

		bModEnabled = getBool("Main", "bModEnabled", true);
		bDebugMode = getBool("Main", "bDebugMode", false);
		bShowUI = getBool("UI", "bShowUI", true);
		iUIDamageDisplayMode = getInt("UI", "iUIDamageDisplayMode", 0);
		// ----- 力量计算模式 -----
		int modeVal = getInt("Main", "iStrengthCalcMode", 0);
		iStrengthCalcMode = (modeVal == 1) ? StrengthCalcMode::kWeightBased : StrengthCalcMode::kJsonRules;

		// ----- 弹药需求计算模式 -----
		int ammoModeVal = getInt("Modifiers", "iAmmoCalcMode", 0);
		iAmmoCalcMode = (ammoModeVal == 1) ? AmmoCalcMode::kWeightBased : AmmoCalcMode::kFixedConfig;

		// ----- 技能检测模式 -----
		int skillModeVal = getInt("Main", "iSkillCalcMode", 0);
		iSkillCalcMode = (skillModeVal == 1) ? SkillCalcMode::kActorValueBased : SkillCalcMode::kPerkBased;

		bGatedSprint = getBool("Movement", "bGatedSprint", true);
		bGatedRun = getBool("Movement", "bGatedRun", false);
		iGatedThreshold = getInt("Movement", "iGatedThreshold", 4);
		bPASoftGateEnabled = getBool("PowerArmor", "bPASoftGateEnabled", true);
		iPenaltyMode = getInt("PowerArmor", "iPenaltyMode", 0);
		iReq_AmmoBase = getInt("Requirements", "iReq_AmmoBase", 0);
		bEnableAmmoReq = getBool("Modifiers", "bEnableAmmoReq", true);
		bAmmoReqAlwaysActive = getBool("Modifiers", "bAmmoReqAlwaysActive", false);
		bEnableCustomAmmoReq = getBool("Modifiers", "bEnableCustomAmmoReq", true);
		bAmmoReqCompensation = getBool("Modifiers", "bAmmoReqCompensation", true);
		bHeavyGunnerReduction = getBool("Reductions", "bHeavyGunnerReduction", true);
		fSkillDamageBase = getFloat("Penalties", "fSkillDamageBase", 0.50f);
		fSkillDamageMax = getFloat("Penalties", "fSkillDamageMax", 1.00f);
		fSwayDynCoef = getFloat("Penalties", "fSwayDynCoef", 0.25f);
		fGunDamageDynCoef = getFloat("Penalties", "fGunDamageDynCoef", 0.00f);
		fThrownDamageDynCoef = getFloat("Penalties", "fThrownDamageDynCoef", 0.00f);
		fSpreadDynCoef = getFloat("Penalties", "fSpreadDynCoef", 0.20f);
		fMeleeStrPenaltyMult = getFloat("Penalties", "fMeleeStrPenaltyMult", 0.15f);
		fMeleeSkillPenaltyMult = getFloat("Penalties", "fMeleeSkillPenaltyMult", 0.10f);
		fVATSAccuracyDynCoef = getFloat("Penalties", "fVATSAccuracyDynCoef", 0.10f);
		fDamageMinLimit = getFloat("Penalties", "fDamageMinLimit", 0.1f);
	}

	void Config::LoadMSFCompatibility() {
		MSF_AmmoMapping.clear();
		std::string msfPath = "Data/MSF/";
		if (!std::filesystem::exists(msfPath)) return;
		for (const auto& entry : std::filesystem::recursive_directory_iterator(msfPath)) {
			if (entry.path().extension() == ".json") {
				std::ifstream file(entry.path());
				if (!file.is_open()) continue;
				std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
				size_t basePos = content.find("\"baseAmmo\"");
				if (basePos == std::string::npos) continue;
				auto getVal = [&](size_t pos) {
					size_t col = content.find(':', pos);
					if (col == std::string::npos) return ""s;
					size_t s = content.find('"', col), e = content.find('"', s + 1);
					return (e != std::string::npos) ? content.substr(s + 1, e - s - 1) : ""s;
					};
				auto parseID = [&](const std::string& str) -> std::uint32_t {
					if (auto f = ResolveIdentifier<RE::TESAmmo>(str)) return f->GetFormID();
					return 0;
					};
				std::uint32_t baseID = parseID(getVal(basePos));
				if (!baseID) continue;
				size_t searchPos = content.find("\"ammoTypes\"");
				while ((searchPos = content.find("\"ammo\"", searchPos + 1)) != std::string::npos) {
					std::uint32_t subID = parseID(getVal(searchPos));
					if (subID && subID != baseID) MSF_AmmoMapping[subID] = baseID;
				}
			}
		}
	}

	void Config::LoadCustomAmmo() {
		CustomAmmoReqs.clear();
		CustomAmmoWeightMappings.clear();
		ConfigReader::ForEachJsonInDirectory(
			"Data\\F4SE\\Plugins\\Weapon Requirements Framework\\Ammo",
			[&](const nlohmann::json& j, const auto&) {
				if (!j.value("Enabled", true) || !j.contains("Rules")) return;
				int filePri = j.value("Priority", 0);
				for (const auto& item : j["Rules"]) {
					int val = item.value("Value", 0);
					if (!item.contains("Conditions")) continue;
					auto& conds = item["Conditions"];

					// 重量范围模式：匹配 MinWeight/MaxWeight
					if (conds.contains("MinWeight")) {
						AmmoWeightMapping mapping;
						mapping.value = val;
						mapping.minWeight = conds["MinWeight"].get<float>();
						if (conds.contains("MaxWeight")) mapping.maxWeight = conds["MaxWeight"].get<float>();
						mapping.priority = filePri + item.value("Priority", 0);
						CustomAmmoWeightMappings.push_back(mapping);
					}

					// 固定配置模式：匹配 Ammo 表单数组
					if (conds.contains("Ammo")) {
						for (auto& ammoVal : conds["Ammo"]) {
							if (auto ammoForm = ResolveIdentifier<RE::TESAmmo>(ammoVal.get<std::string>())) CustomAmmoReqs[ammoForm->GetFormID()] = val;
						}
					}
				}
			},
			true);
		// 重量范围规则按优先级降序排序
		std::sort(CustomAmmoWeightMappings.begin(), CustomAmmoWeightMappings.end(),
			[](const auto& a, const auto& b) { return a.priority > b.priority; });
		REX::INFO("Loaded Ammo Mappings: {} rules, {} weight ranges", CustomAmmoReqs.size(), CustomAmmoWeightMappings.size());
	}

	void Config::LoadCustomSkills() {
		CustomSkillMappings.clear(); IconPool.clear();
		ConfigReader::ForEachJsonInDirectory(
			"Data\\F4SE\\Plugins\\Weapon Requirements Framework\\Skills",
			[&](const nlohmann::json& j, const auto&) {
				if (!j.value("Enabled", true) || !j.contains("Rules")) return;
				int filePri = j.value("Priority", 0);
				for (const auto& item : j["Rules"]) {
					SkillMapping mapping;
					mapping.reqValue = item.value("ReqValue", 0.0f);
					mapping.icon = item.value("Icon", "");
					if (!mapping.icon.empty() && std::find(IconPool.begin(), IconPool.end(), mapping.icon) == IconPool.end()) IconPool.push_back(mapping.icon);
					if (item.contains("Skills")) {
						auto& skills = item["Skills"];
						if (skills.contains("Perks")) for (auto& pkVal : skills["Perks"]) if (auto pkForm = ResolveIdentifier<RE::BGSPerk>(pkVal.get<std::string>())) mapping.perks.push_back(pkForm);

						if (skills.contains("ActorValues")) {
							for (auto& avVal : skills["ActorValues"]) {
								if (auto avForm = ResolveIdentifier<RE::ActorValueInfo>(avVal.get<std::string>())) mapping.skillAVs.push_back(avForm);
							}
						}
						else if (skills.contains("SkillAVs")) {
							for (auto& skVal : skills["SkillAVs"]) {
								if (auto skForm = ResolveIdentifier<RE::ActorValueInfo>(skVal.get<std::string>())) mapping.skillAVs.push_back(skForm);
							}
						}
					}
					bool hasWpn = false;
					if (item.contains("Conditions")) {
						auto& conds = item["Conditions"];
						std::string mType = conds.value("MatchType", "OR");
						std::transform(mType.begin(), mType.end(), mType.begin(), ::toupper);
						mapping.keywordMatchAnd = (mType == "AND");
						if (conds.contains("Weapons")) for (auto& wpVal : conds["Weapons"]) if (auto wpForm = ResolveIdentifier<RE::TESObjectWEAP>(wpVal.get<std::string>())) { mapping.weapons.push_back(wpForm); hasWpn = true; }
						if (conds.contains("Keywords")) for (auto& kwVal : conds["Keywords"]) if (auto kwForm = ResolveIdentifier<RE::BGSKeyword>(kwVal.get<std::string>())) mapping.keywords.push_back(kwForm);
					}
					mapping.priority = hasWpn ? 9999 : (filePri + item.value("Priority", 0));

					// 根据当前技能检测模式过滤：只保留匹配该模式的规则
					bool hasValidSkills = false;
					if (iSkillCalcMode == SkillCalcMode::kPerkBased) {
						hasValidSkills = !mapping.perks.empty();
					} else {
						hasValidSkills = !mapping.skillAVs.empty();
					}
					if ((!mapping.weapons.empty() || !mapping.keywords.empty()) && hasValidSkills)
						CustomSkillMappings.push_back(mapping);
				}
			},
			true);
		std::sort(CustomSkillMappings.begin(), CustomSkillMappings.end(), [](const auto& a, const auto& b) { return a.priority > b.priority; });
		if (IconPool.empty()) IconPool.push_back("[Weapon]");
		REX::INFO("Loaded Skill Mappings: {} rules. Icons: {}", CustomSkillMappings.size(), IconPool.size());
	}

	void Config::LoadCustomStrengths() {
		CustomStrengthMappings.clear();
		ConfigReader::ForEachJsonInDirectory(
			"Data\\F4SE\\Plugins\\Weapon Requirements Framework\\Strength",
			[&](const nlohmann::json& j, const auto&) {
				if (!j.value("Enabled", true) || !j.contains("Rules")) return;
				int filePri = j.value("Priority", 0);
				for (const auto& item : j["Rules"]) {
					StrengthMapping mapping;
					mapping.value = item.value("Value", 0.0f);
					if (item.contains("Options")) {
						mapping.isModifier = item["Options"].value("IsModifier", false);
						mapping.isMultiplier = item["Options"].value("IsMultiplier", false);
					}
					bool hasWpn = false;
					if (item.contains("Conditions")) {
						auto& conds = item["Conditions"];
						std::string mType = conds.value("MatchType", "OR");
						std::transform(mType.begin(), mType.end(), mType.begin(), ::toupper);
						mapping.keywordMatchAnd = (mType == "AND");
						if (conds.contains("Weapons")) for (auto& wpVal : conds["Weapons"]) if (auto wpForm = ResolveIdentifier<RE::TESObjectWEAP>(wpVal.get<std::string>())) { mapping.weapons.push_back(wpForm); hasWpn = true; }
						if (conds.contains("Keywords")) for (auto& kwVal : conds["Keywords"]) if (auto kwForm = ResolveIdentifier<RE::BGSKeyword>(kwVal.get<std::string>())) mapping.keywords.push_back(kwForm);
						// 重量范围条件
						if (conds.contains("MinWeight")) {
							mapping.minWeight = conds["MinWeight"].get<float>();
						}
						if (conds.contains("MaxWeight")) mapping.maxWeight = conds["MaxWeight"].get<float>();
					}
					mapping.priority = hasWpn ? 9999 : (filePri + item.value("Priority", 0));
					CustomStrengthMappings.push_back(mapping);
				}
			},
			true);
		std::sort(CustomStrengthMappings.begin(), CustomStrengthMappings.end(), [](const auto& a, const auto& b) { return a.priority > b.priority; });
		REX::INFO("Loaded Strength Mappings: {}", CustomStrengthMappings.size());
	}

	template <typename T>
	void LoadFormInternal(T*& ptr, const std::string& modName, std::uint32_t localID, const char*) {
		ptr = RE::TESDataHandler::GetSingleton()->LookupForm<T>(localID, modName);
	}
	template <typename T>
	void LoadVanillaInternal(T*& ptr, std::uint32_t id, const char*) {
		auto form = RE::TESForm::GetFormByID(id);
		ptr = form ? form->As<T>() : nullptr;
	}

	void Config::LoadForms() {
		const std::string modName = "Weapon Requirements Framework.esp";
#define L_MOD(var, id) LoadFormInternal(var, modName, id, #var)
#define L_VAN(var, id) LoadVanillaInternal(var, id, #var)

		L_MOD(WRF_Message_PenaltyMode, 0x4FC); L_MOD(WRF_InsufficientStrength, 0x762); L_MOD(WRF_InsufficientSkill, 0xFAF);
		L_MOD(WRF_PowerArmorWeaponState, 0x763); L_MOD(WRF_MCM_StrengthDebuff_GatedSprint, 0xE3A); L_MOD(WRF_MCM_StrengthDebuff_GatedRun, 0xE39);
		L_MOD(WRF_EquippedWeaponType, 0xFB0); L_MOD(WRF_MainScript_Perk, 0x6A8);
		L_VAN(PerkHeavyGunner1, 0x4A0D6); L_VAN(PerkHeavyGunner2, 0x4A0D7); L_VAN(PerkHeavyGunner3, 0x4A0D8); L_VAN(PerkHeavyGunner4, 0x65E2A); L_VAN(PerkHeavyGunner5, 0x65E2B);
		L_MOD(WRF_WeaponRequirementValue, 0xD99); L_MOD(WRF_StrengthDeficit, 0xE23);
		L_MOD(WRF_Mult_ScopeStability, 0x6A5); L_MOD(WRF_Mult_ConeOfFire, 0x6A6); L_MOD(WRF_Mult_VATSAccuracy, 0x6A3);
		L_MOD(WRF_WeaponJamSound, 0xDAC); L_MOD(WRF_PASupport, 0x1F7); L_MOD(WeaponTypePowerArmor, 0x257);

		L_VAN(WeaponTypePistol, 0x4A0A0); L_VAN(WeaponTypeHeavyGun, 0x4A0A3); L_VAN(WeaponTypeBroadsider, 0x225766);
		L_VAN(WeaponTypeCryolator, 0x22575F); L_VAN(WeaponTypeFlamer, 0x225760); L_VAN(WeaponTypeJunkJet, 0x225763);
		L_VAN(WeaponTypeMissileLauncher, 0x22575B); L_VAN(WeaponTypeMinigun, 0x22575D); L_VAN(WeaponTypeGatlingLaser, 0x22575E);
		L_VAN(WeaponTypeFatMan, 0x22575C); L_VAN(ObjectTypeAmmo, 0xF4AE8); L_VAN(isPowerArmorFrame, 0x15503F); L_VAN(FurnitureTypePowerArmor, 0x3430B);

		//L_MOD(WRF_MeleeSpeedPerk, 0x3430B);

		REX::INFO("Forms Loaded.");
	}
}
