#include "pch.h"
#include "Mechanics.h"
#include "Config.h"
#include <unordered_set>
#include <atomic>
#include <chrono>
#include <cmath>

namespace WRF::Mechanics
{
	static std::atomic<std::chrono::steady_clock::time_point> s_attackLockoutUntil;

	// =========================================================
	// 基础工具与判定
	// =========================================================
	const char* GetFormName(RE::TESForm* a_form) {
		if (!a_form) return "NULL";
		const char* name = a_form->GetFormEditorID();
		if (name && name[0]) return name;
		auto fullName = a_form->As<RE::TESFullName>();
		if (fullName && fullName->GetFullNameLength() > 0) return fullName->GetFullName();
		return "<No Editor ID>";
	}

	std::string GetCleanName(RE::TESForm* form) {
		std::string finalName = "战斗技能";
		if (form) {
			auto fullName = form->As<RE::TESFullName>();
			if (fullName && fullName->GetFullNameLength() > 0) finalName = fullName->GetFullName();
			else finalName = form->GetFormEditorID();
		}
		if (finalName.find("PRKFEx") != std::string::npos || finalName.find("Demo") != std::string::npos || finalName.find("AV_") != std::string::npos) finalName = "战斗技能";
		return finalName;
	}

	std::string GetFallbackIcon(RE::TESForm* a_form) {
		auto cfg = Config::GetSingleton();
		if (cfg->IconPool.empty()) return "[Weapon]";
		return cfg->IconPool[a_form ? (a_form->GetFormID() % cfg->IconPool.size()) : 0];
	}

	bool HasKeyword(RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance, RE::BGSKeyword* a_kwd) {
		if (!a_kwd) return false;
		if (auto baseKw = static_cast<RE::BGSKeywordForm*>(a_weapon); baseKw && baseKw->HasKeyword(a_kwd)) return true;
		if (a_instance) {
			auto instData = static_cast<RE::TESObjectWEAP::InstanceData*>(a_instance);
			if (instData && instData->keywords && instData->keywords->HasKeyword(a_kwd)) return true;
		}
		return false;
	}

	bool GetEquippedMainWeapon(RE::Actor* a_actor, RE::TESObjectWEAP*& a_weapon, RE::TBO_InstanceData*& a_instance) {
		a_weapon = nullptr; a_instance = nullptr;
		if (!a_actor || !a_actor->currentProcess || !a_actor->currentProcess->middleHigh) return false;
		for (auto& item : a_actor->currentProcess->middleHigh->equippedItems) {
			if (item.item.object && item.item.object->As<RE::TESObjectWEAP>()) {
				auto w = item.item.object->As<RE::TESObjectWEAP>();
				int cat = GetWeaponCategory(w, item.item.instanceData.get());
				if (cat >= 1 && cat <= 3) { a_weapon = w; a_instance = item.item.instanceData.get(); return true; }
			}
		}
		return false;
	}

	bool GetEquippedThrownWeapon(RE::Actor* a_actor, RE::TESObjectWEAP*& a_weapon, RE::TBO_InstanceData*& a_instance) {
		a_weapon = nullptr; a_instance = nullptr;
		if (!a_actor || !a_actor->currentProcess || !a_actor->currentProcess->middleHigh) return false;
		for (auto& item : a_actor->currentProcess->middleHigh->equippedItems) {
			if (item.item.object && item.item.object->As<RE::TESObjectWEAP>()) {
				auto w = item.item.object->As<RE::TESObjectWEAP>();
				if (GetWeaponCategory(w, item.item.instanceData.get()) == 4) { a_weapon = w; a_instance = item.item.instanceData.get(); return true; }
			}
		}
		return false;
	}

	bool IsPlayerInPowerArmor(RE::Actor* a_actor) {
		if (!a_actor) return false;
		if (RE::PowerArmor::ActorInPowerArmor(*a_actor)) return true;
		auto cfg = Config::GetSingleton();
		return (cfg->isPowerArmorFrame && a_actor->HasKeyword(cfg->isPowerArmorFrame)) ||
			(cfg->FurnitureTypePowerArmor && a_actor->HasKeyword(cfg->FurnitureTypePowerArmor));
	}

	int GetWeaponCategory(RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance) {
		if (!a_weapon) return 0;
		auto weapData = a_instance ? static_cast<RE::TESObjectWEAP::InstanceData*>(a_instance) : &a_weapon->weaponData;
		auto wType = static_cast<std::uint32_t>(weapData->type.get());
		if (wType == 10 || wType == 11) return 4; // Grenade/Mine
		static auto kwdEx = RE::TESForm::GetFormByID(0x092A87);
		static auto kwdTh = RE::TESForm::GetFormByID(0x0B2CA9);
		if ((kwdEx && HasKeyword(a_weapon, a_instance, kwdEx->As<RE::BGSKeyword>())) || (kwdTh && HasKeyword(a_weapon, a_instance, kwdTh->As<RE::BGSKeyword>()))) return 4;
		if (wType == 0) return 1;
		if (wType >= 1 && wType <= 6) return 2;
		if (wType >= 7 && wType <= 13) return 3;
		return 0;
	}

	int GetPAWeaponState(RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance) {
		auto cfg = Config::GetSingleton();
		if (!a_weapon) return 0;
		if (HasKeyword(a_weapon, a_instance, cfg->WeaponTypePowerArmor)) return 3;
		if (HasKeyword(a_weapon, a_instance, cfg->WRF_PASupport)) return 2;
		if (HasKeyword(a_weapon, a_instance, cfg->WeaponTypePistol)) return 1;
		return 0;
	}

	bool IsHeavyWeapon(RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance) {
		auto cfg = Config::GetSingleton();
		return a_weapon && (HasKeyword(a_weapon, a_instance, cfg->WeaponTypeHeavyGun) || HasKeyword(a_weapon, a_instance, cfg->WeaponTypeMinigun) ||
			HasKeyword(a_weapon, a_instance, cfg->WeaponTypeGatlingLaser) || HasKeyword(a_weapon, a_instance, cfg->WeaponTypeFatMan) ||
			HasKeyword(a_weapon, a_instance, cfg->WeaponTypeMissileLauncher) || HasKeyword(a_weapon, a_instance, cfg->WeaponTypeFlamer) ||
			HasKeyword(a_weapon, a_instance, cfg->WeaponTypeJunkJet) || HasKeyword(a_weapon, a_instance, cfg->WeaponTypeCryolator) ||
			HasKeyword(a_weapon, a_instance, cfg->WeaponTypeBroadsider));
	}

	int GetAmmoRequirement(RE::TESForm* a_item) {
		if (!a_item || !a_item->As<RE::TESAmmo>()) return -1;
		auto cfg = Config::GetSingleton();
		auto ammo = a_item->As<RE::TESAmmo>();

		// 模式 B：基于弹药自身重量计算需求（JSON 规则驱动）
		if (cfg->iAmmoCalcMode == Config::AmmoCalcMode::kWeightBased) {
			float weight = ammo->weight;
			float roundedWeight = std::round(weight);
			int req = 0;
			bool matched = false;
			for (const auto& rule : cfg->CustomAmmoWeightMappings) {
				bool match = roundedWeight >= rule.minWeight;
				if (match && rule.maxWeight >= 0.0f) {
					match = roundedWeight <= rule.maxWeight;
				}
				if (match) {
					req = rule.value;
					matched = true;
					break;
				}
			}
			if (cfg->bDebugMode) {
				if (matched) {
					REX::INFO("[WRF DEBUG] 弹药重量模式: weight={:.3f} -> 取整={:.0f}, [MATCH] req={}", weight, roundedWeight, req);
				} else {
					REX::INFO("[WRF DEBUG] 弹药重量模式: weight={:.3f} -> 取整={:.0f}, [NO MATCH]", weight, roundedWeight);
				}
			}
			return req;
		}

		// 模式 A：原有的固定配置驱动
		std::uint32_t id = a_item->formID;
		if (cfg->bEnableCustomAmmoReq) {
			if (cfg->CustomAmmoReqs.contains(id)) return cfg->CustomAmmoReqs[id];
			if (cfg->MSF_AmmoMapping.contains(id) && cfg->CustomAmmoReqs.contains(cfg->MSF_AmmoMapping[id])) return cfg->CustomAmmoReqs[cfg->MSF_AmmoMapping[id]];
		}
		if (cfg->ObjectTypeAmmo && ammo->HasKeyword(cfg->ObjectTypeAmmo)) return cfg->iReq_AmmoBase;
		return 0;
	}

	SkillReqInfo GetSkillRequirement(RE::Actor* a_actor, RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance) {
		if (!a_actor || !a_weapon) return { false, false, "", 0, 0.0f, "", 0 };
		auto cfg = Config::GetSingleton();
		bool debug = cfg->bDebugMode;

		if (debug) {
			REX::INFO("[WRF DEBUG] GetSkillRequirement: weapon={} ({:08X}), mode={}, mappings={}",
				GetFormName(a_weapon), a_weapon->formID,
				(cfg->iSkillCalcMode == Config::SkillCalcMode::kActorValueBased) ? "AV" : "Perk",
				cfg->CustomSkillMappings.size());
		}

		for (const auto& mapping : cfg->CustomSkillMappings) {
			bool isMatch = std::find(mapping.weapons.begin(), mapping.weapons.end(), a_weapon) != mapping.weapons.end();
			if (!isMatch && !mapping.keywords.empty()) {
				if (mapping.keywordMatchAnd) {
					isMatch = true;
					for (auto kwd : mapping.keywords) if (!HasKeyword(a_weapon, a_instance, kwd)) { isMatch = false; break; }
				}
				else {
					for (auto kwd : mapping.keywords) if (HasKeyword(a_weapon, a_instance, kwd)) { isMatch = true; break; }
				}
			}
			if (isMatch) {
				if (!mapping.skillAVs.empty()) {
					float maxSkill = 0.0f; RE::ActorValueInfo* bestAV = mapping.skillAVs[0];
					for (auto av : mapping.skillAVs) { float val = a_actor->GetActorValue(*av); if (val > maxSkill) { maxSkill = val; bestAV = av; } }
						return { true, false, GetCleanName(bestAV), static_cast<int>(mapping.reqValue), mapping.reqValue - maxSkill, mapping.icon, 100 };
				}
				else if (!mapping.perks.empty()) {
					float maxRank = 0.0f; RE::BGSPerk* bestPerk = mapping.perks[0];
					for (auto pk : mapping.perks) { float r = a_actor->GetPerkRank(pk); if (r > maxRank) { maxRank = r; bestPerk = pk; } }
					int mRank = bestPerk && bestPerk->data.numRanks > 0 ? bestPerk->data.numRanks : 5;
					return { true, true, GetCleanName(bestPerk), static_cast<int>(mapping.reqValue), mapping.reqValue - maxRank, mapping.icon, mRank };
				}
		}
	}
	// Perk 模式下：无规则匹配则跳过武器原生 skill 托底，返回无需求
	if (cfg->iSkillCalcMode == Config::SkillCalcMode::kPerkBased) {
		if (debug) REX::INFO("[WRF DEBUG] GetSkillRequirement: no match (Perk mode, silent)");
		return { false, false, "", 0, 0.0f, GetFallbackIcon(a_weapon), 0 };
	}
	auto wData = a_instance ? static_cast<RE::TESObjectWEAP::InstanceData*>(a_instance) : &a_weapon->weaponData;
	if (wData && wData->skill) {
		if (debug) REX::INFO("[WRF DEBUG] GetSkillRequirement: using native skill {}", GetFormName(wData->skill));
		return { true, false, GetCleanName(wData->skill), 50, 50.0f - a_actor->GetActorValue(*wData->skill), GetFallbackIcon(a_weapon), 100 };
	}
	if (debug) REX::INFO("[WRF DEBUG] GetSkillRequirement: no match, no native skill -> no requirement");
	return { false, false, "", 0, 0.0f, GetFallbackIcon(a_weapon), 0 };
}

	float GetWeaponWeight(RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance)
	{
		if (!a_weapon) {
			if (Config::GetSingleton()->bDebugMode) REX::INFO("[WRF DEBUG] GetWeaponWeight: a_weapon 为 nullptr");
			return 0.0f;
		}
		float w;
		if (a_instance) {
			auto inst = static_cast<RE::TESObjectWEAP::InstanceData*>(a_instance);
			w = inst->weight;
			if (Config::GetSingleton()->bDebugMode) REX::INFO("[WRF DEBUG] GetWeaponWeight: 使用实例重量 inst->weight = {:.2f}", w);
		}
		else {
			w = a_weapon->weaponData.weight;
			if (Config::GetSingleton()->bDebugMode) REX::INFO("[WRF DEBUG] GetWeaponWeight: 使用基础重量 weaponData.weight = {:.2f}", w);
		}
		return w;
	}

	float CalculateRequirement(RE::Actor* a_actor, RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance, bool a_includeAmmo) {
		if (!a_weapon) return 0.0f;
		if (GetWeaponCategory(a_weapon, a_instance) == 4) return -1.0f;
		auto cfg = Config::GetSingleton();
		bool debug = cfg->bDebugMode;

		if (debug) {
			const char* wName = GetFormName(a_weapon);
			REX::INFO("[WRF DEBUG] ===== CalculateRequirement 开始 =====");
			REX::INFO("[WRF DEBUG] 武器: {} ({:08X})", wName, a_weapon->formID);
			REX::INFO("[WRF DEBUG] 当前模式: {}", (cfg->iStrengthCalcMode == Config::StrengthCalcMode::kWeightBased) ? "重量范围(WeightBased)" : "武器/关键词(JsonRules)");
		}

		float req;
		if (cfg->iStrengthCalcMode == Config::StrengthCalcMode::kWeightBased) {
			// 模式 B：基于重量范围的 JSON 规则
			float weight = GetWeaponWeight(a_weapon, a_instance);
			float roundedWeight = std::round(weight);
			if (debug) REX::INFO("[WRF DEBUG] [重量模式] 武器重量: {:.2f} -> 取整: {:.0f}", weight, roundedWeight);

			req = 0.0f;
			bool matched = false;
			for (const auto& rule : cfg->CustomStrengthMappings) {
				// 跳过有武器/关键词条件的规则（只在关键词模式下使用）
				if (rule.minWeight >= 0.0f && rule.weapons.empty() && rule.keywords.empty()) {
					bool match = roundedWeight >= rule.minWeight;
					if (match && rule.maxWeight >= 0.0f) {
						match = roundedWeight <= rule.maxWeight;
					}
					if (debug) {
						REX::INFO("[WRF DEBUG] [重量模式] 检查规则: minW={:.1f}, maxW={:.1f}, value={:.1f} -> {}",
							rule.minWeight, rule.maxWeight, rule.value, match ? "[MATCH]" : "[SKIP]");
					}
					if (match) {
						req = rule.value;
						matched = true;
						break;
					}
				}
			}
			if (debug) REX::INFO("[WRF DEBUG] [重量模式] 匹配结果: {} (req={:.1f})", matched ? "[MATCH]" : "[NO MATCH, DEFAULT 0]", req);
		}
		else {
			// 模式 A：原有的 JSON 规则驱动（按武器/关键词匹配）
			float baseReq = -1.0f, addMod = 0.0f, multMod = 1.0f;
			if (debug) REX::INFO("[WRF DEBUG] [关键词模式] 开始匹配规则，共 {} 条规则", cfg->CustomStrengthMappings.size());

			for (const auto& rule : cfg->CustomStrengthMappings) {
				// 跳过仅有重量范围的规则
				if (rule.minWeight >= 0.0f && rule.weapons.empty() && rule.keywords.empty()) {
					if (debug) REX::INFO("[WRF DEBUG] [关键词模式] 规则跳过(仅重量范围规则)");
					continue;
				}
				bool isMatch = std::find(rule.weapons.begin(), rule.weapons.end(), a_weapon) != rule.weapons.end();
				if (!isMatch && !rule.keywords.empty()) {
					if (rule.keywordMatchAnd) {
						isMatch = true;
						for (auto kwd : rule.keywords) if (!HasKeyword(a_weapon, a_instance, kwd)) { isMatch = false; break; }
					}
					else {
						for (auto kwd : rule.keywords) if (HasKeyword(a_weapon, a_instance, kwd)) { isMatch = true; break; }
					}
				}
				if (debug) {
					REX::INFO("[WRF DEBUG] [关键词模式] 规则: val={:.1f}, isMod={}, isMult={}, weapons={}, keywords={} -> {}",
						rule.value, rule.isModifier, rule.isMultiplier, rule.weapons.size(), rule.keywords.size(),
						isMatch ? "[MATCH]" : "[SKIP]");
				}
				if (isMatch) {
					if (rule.isModifier) { if (rule.isMultiplier) multMod *= rule.value; else addMod += rule.value; }
					else if (baseReq < 0.0f) { baseReq = rule.value; if (debug) REX::INFO("[WRF DEBUG] [关键词模式]   -> 设置 baseReq = {:.1f}", baseReq); }
				}
			}

			if (baseReq < 0.0f) baseReq = 0.0f;
			req = (baseReq + addMod) * multMod;
			if (debug) REX::INFO("[WRF DEBUG] [关键词模式] 计算: ({:.1f} + {:.1f}) * {:.1f} = {:.1f}", baseReq, addMod, multMod, req);
		}

		if (cfg->bEnableAmmoReq && a_includeAmmo) {
			RE::TESAmmo* ammo = a_instance ? static_cast<RE::TESObjectWEAP::InstanceData*>(a_instance)->ammo : a_weapon->weaponData.ammo;
			if (ammo) {
				int aReq = GetAmmoRequirement(ammo);
				if (debug) REX::INFO("[WRF DEBUG] 弹药需求: ammo={:08X}, aReq={}", ammo->formID, aReq);
				if (aReq > 0) {
					if (cfg->bAmmoReqCompensation) {
						req = std::max(0.0f, req - aReq);
						if (debug) REX::INFO("[WRF DEBUG] 弹药补偿: req 减少 {} -> {:.1f}", aReq, req);
					}
					bool hasAmmo = false;
					if (a_actor) {
						std::uint32_t ammoCount = 0;
						a_actor->GetItemCount(ammoCount, ammo, false);
						if (ammoCount > 0) hasAmmo = true;
					}
					if (cfg->bAmmoReqAlwaysActive || hasAmmo) {
						req += aReq;
						if (debug) REX::INFO("[WRF DEBUG] 弹药加成: req 增加 {} -> {:.1f} (hasAmmo={}, alwaysActive={})", aReq, req, hasAmmo, cfg->bAmmoReqAlwaysActive);
					}
				}
			}
		}

		if (cfg->bHeavyGunnerReduction && IsHeavyWeapon(a_weapon, a_instance) && a_actor) {
			int rank = 0;
			if (cfg->PerkHeavyGunner5 && a_actor->GetPerkRank(cfg->PerkHeavyGunner5) > 0) rank = 5;
			else if (cfg->PerkHeavyGunner4 && a_actor->GetPerkRank(cfg->PerkHeavyGunner4) > 0) rank = 4;
			else if (cfg->PerkHeavyGunner3 && a_actor->GetPerkRank(cfg->PerkHeavyGunner3) > 0) rank = 3;
			else if (cfg->PerkHeavyGunner2 && a_actor->GetPerkRank(cfg->PerkHeavyGunner2) > 0) rank = 2;
			else if (cfg->PerkHeavyGunner1 && a_actor->GetPerkRank(cfg->PerkHeavyGunner1) > 0) rank = 1;
			if (rank >= 2) {
				req -= (rank - 1);
				if (debug) REX::INFO("[WRF DEBUG] 重枪手减免: rank={}, req 减少 {} -> {:.1f}", rank, (rank - 1), req);
			}
		}

		float finalReq = std::max(0.0f, req);
		finalReq = std::round(finalReq);
		if (debug) REX::INFO("[WRF DEBUG] ===== 最终力量需求: {:.0f} =====", finalReq);
		return finalReq;
	}

	float GetStrengthDeficit(RE::Actor* a_actor, RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance) {
		if (!a_actor || !Config::GetSingleton()->bModEnabled) return 0.0f;
		if (!a_weapon && !GetEquippedMainWeapon(a_actor, a_weapon, a_instance)) return 0.0f;
		static auto avStr = RE::TESForm::GetFormByID(0x2C2)->As<RE::ActorValueInfo>();
		return std::max(0.0f, CalculateRequirement(a_actor, a_weapon, a_instance) - (avStr ? a_actor->GetActorValue(*avStr) : 5.0f));
	}

	DamageMults CalculateDamageMults(RE::Actor* a_actor, RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance) {
		DamageMults m;
		auto cfg = Config::GetSingleton();
		if (!a_actor || !a_weapon || !cfg->bModEnabled) {
			m.strMult = 1.0f; // 默认倍率
			m.skillMult = 1.0f;
			return m;
		}

		// 1. 初始化为 1.0 (中性，不参与惩罚)
		m.strMult = 1.0f;
		m.skillMult = 1.0f;

		int cat = GetWeaponCategory(a_weapon, a_instance);
		auto skillReq = GetSkillRequirement(a_actor, a_weapon, a_instance);

		// 2. 只有技能才会影响伤害 (Skill -> Damage)
		if (cat != 1) { // 假设 1 是徒手，或者你按需调整分类
			float pSkill = (skillReq.hasReq && skillReq.reqValue > 0) ? std::clamp(100.0f * (1.0f - (skillReq.deficit / static_cast<float>(skillReq.reqValue))), 0.0f, 100.0f) : 100.0f;
			m.skillMult = cfg->fSkillDamageBase + (pSkill / 100.0f) * (cfg->fSkillDamageMax - cfg->fSkillDamageBase);
		}

		// 3. 计算惩罚值 (normDef 已经是归一化后的技能缺口)
		float normDef = skillReq.isPerk ? skillReq.deficit : (skillReq.deficit / 20.0f);

		// 4. 彻底移除力量对伤害的 strMult *= ... 逻辑
		// 现在的 strMult 保持 1.0，不会对伤害产生衰减

		// 应用技能伤害衰减
		if (skillReq.deficit > 0) {
			if (cat == 1 || cat == 2) { // 近战
				m.skillMult *= std::max(1.0f - (normDef * cfg->fMeleeSkillPenaltyMult), 0.01f);
			}
			else if (cat == 3) { // 远程
				m.skillMult *= std::max(1.0f - (normDef * cfg->fGunDamageDynCoef), 0.01f);
			}
			else if (cat == 4) { // 投掷
				m.skillMult *= std::max(1.0f - (normDef * cfg->fThrownDamageDynCoef), 0.01f);
			}
		}

		// 确保伤害不会低于设定的最低下限
		m.skillMult = std::max(m.skillMult, cfg->fDamageMinLimit);

		return m;
	}

	static void UpdateMainPerkInternal(RE::Actor* a_actor) {
		auto cfg = Config::GetSingleton();
		if (!cfg->WRF_MainScript_Perk) return;
		bool hasP = a_actor->GetPerkRank(cfg->WRF_MainScript_Perk) > 0;

		if (cfg->bModEnabled) {
			if (!hasP) a_actor->AddPerk(cfg->WRF_MainScript_Perk, 1);
		}
		else {
			if (hasP) a_actor->RemovePerk(cfg->WRF_MainScript_Perk);
		}
	}

	static RE::BSTSmartPointer<RE::BSInputEnableLayer> g_restrictionLayer;
	static bool s_isCurrentlyGated = false, s_isRunGated = false, s_recentSprintSnapshot = false;
	static std::atomic<int> g_tripFrames{ 0 };
	static std::unordered_set<std::string> s_heldKeys;

	bool IsPlayerMoving() { auto pc = RE::PlayerControls::GetSingleton(); return pc && (pc->data.moveInputVec.x != 0 || pc->data.moveInputVec.y != 0); }
	bool IsPlayerTryingToSprint() { auto p = RE::PlayerCharacter::GetSingleton(); return s_heldKeys.count("Sprint") || (p && p->sprintToggled); }
	void TryShowSprintMsg() { RE::SendHUDMessage::ShowHUDMessage("$SprintBlockedMessage", nullptr, true, false); }


	void RefreshStatus(RE::Actor* a_actor) {
		if (!a_actor) return;
		auto cfg = Config::GetSingleton();

		if (!cfg->bModEnabled) {
			if (g_restrictionLayer) g_restrictionLayer.reset(); s_isCurrentlyGated = s_isRunGated = false;
			if (cfg->WRF_Mult_ScopeStability) a_actor->SetActorValue(*cfg->WRF_Mult_ScopeStability, 1.0f);
			if (cfg->WRF_Mult_ConeOfFire) a_actor->SetActorValue(*cfg->WRF_Mult_ConeOfFire, 1.0f);
			if (cfg->WRF_Mult_VATSAccuracy) a_actor->SetActorValue(*cfg->WRF_Mult_VATSAccuracy, 1.0f);
			if (cfg->WRF_StrengthDeficit) a_actor->SetActorValue(*cfg->WRF_StrengthDeficit, 0.0f);
			if (cfg->WRF_WeaponRequirementValue) a_actor->SetActorValue(*cfg->WRF_WeaponRequirementValue, 0.0f);
			if (cfg->WRF_InsufficientStrength) cfg->WRF_InsufficientStrength->value = 0.0f;
			if (cfg->WRF_InsufficientSkill) cfg->WRF_InsufficientSkill->value = 0.0f;
			if (cfg->WRF_PowerArmorWeaponState) cfg->WRF_PowerArmorWeaponState->value = 0.0f;
			UpdateMainPerkInternal(a_actor); return;
		}

		RE::TESObjectWEAP* w = nullptr;
		RE::TBO_InstanceData* i = nullptr;
		bool hasW = GetEquippedMainWeapon(a_actor, w, i);
		float strDef = GetStrengthDeficit(a_actor);
		bool inPA = IsPlayerInPowerArmor(a_actor);
		bool isDrawn = (a_actor->weaponState == RE::WEAPON_STATE::kDrawn || a_actor->weaponState == RE::WEAPON_STATE::kDrawing);

		// ========================================================
		// 动力甲限制与移动限制
		// ========================================================
		int paState = hasW ? GetPAWeaponState(w, i) : 0;
		if (hasW && paState == 0) {
			float req = CalculateRequirement(a_actor, w, i);
			static auto avStr = RE::TESForm::GetFormByID(0x2C2)->As<RE::ActorValueInfo>();
			if (req > (avStr ? a_actor->GetBaseActorValue(*avStr) : 5.0f)) paState = -1;
		}

		static std::uint32_t s_lastW = 0; static bool s_wasPA = false, s_wasDrawn = false;
		std::uint32_t curW = hasW ? w->GetFormID() : 0;
		if (hasW && inPA && ((curW != s_lastW) || (inPA && !s_wasPA && isDrawn) || (isDrawn && !s_wasDrawn))) {
			if (paState == -1) RE::SendHUDMessage::ShowHUDMessage("$PowerArmorMSG", nullptr, false, true);
			else if (paState == 2) RE::SendHUDMessage::ShowHUDMessage("$WRF_PASupportMSG", nullptr, false, true);
			else if (paState == 3) RE::SendHUDMessage::ShowHUDMessage("$WeaponTypePowerArmorMSG", nullptr, false, true);
		}
		if (cfg->WRF_PowerArmorWeaponState) cfg->WRF_PowerArmorWeaponState->value = static_cast<float>(paState);
		s_lastW = curW; s_wasPA = inPA; s_wasDrawn = isDrawn;

		bool restSprint = false, restRun = false;
		if (!inPA && isDrawn && strDef > 0 && hasW && cfg->bGatedSprint) {
			if (IsHeavyWeapon(w, i) || cfg->iGatedThreshold == 0 || strDef >= cfg->iGatedThreshold) {
				restSprint = true; if (cfg->bGatedRun) restRun = true;
			}
		}

		static bool s_lastR = false;
		s_isCurrentlyGated = (restSprint || restRun); s_isRunGated = restRun;
		bool trySpr = IsPlayerTryingToSprint() || s_recentSprintSnapshot;
		if (restSprint) s_recentSprintSnapshot = false;
		if (restSprint && (restSprint != s_lastR) && trySpr && IsPlayerMoving()) TryShowSprintMsg();

		if (auto input = RE::BSInputEnableManager::GetSingleton()) {
			if (s_isCurrentlyGated) {
				if (!g_restrictionLayer) input->AllocateNewLayer(g_restrictionLayer, "WRF_Gating");
				if (g_restrictionLayer) {
					input->EnableOtherEvent(g_restrictionLayer->layerID, RE::OtherInputEvents::OTHER_EVENT_FLAG::kSprinting, !restSprint, RE::UserEvents::SENDER_ID::kScript);
					input->EnableOtherEvent(g_restrictionLayer->layerID, RE::OtherInputEvents::OTHER_EVENT_FLAG::kRunning, !restRun, RE::UserEvents::SENDER_ID::kScript);
					if (!restRun && restSprint && restSprint != s_lastR) g_tripFrames = 3;
				}
			}
			else if (g_restrictionLayer) g_restrictionLayer.reset();
		}
		s_lastR = restSprint;

		float swayM = 1.0f, spreadM = 1.0f, vatsM = 1.0f; bool skillDef = false;
		int cat = GetWeaponCategory(w, i);
		if (cfg->WRF_EquippedWeaponType) cfg->WRF_EquippedWeaponType->value = static_cast<float>(cat);

		if (hasW) {
			auto sReq = GetSkillRequirement(a_actor, w, i);
			if (sReq.hasReq && sReq.deficit > 0) skillDef = true;
			if (cat == 3 || cat == 4) {
				if (strDef > 0 && !inPA) { swayM = 1.0f + (strDef * cfg->fSwayDynCoef); spreadM = 1.0f + (strDef * cfg->fSpreadDynCoef); }
				if (skillDef) vatsM = 1.0f - ((sReq.isPerk ? sReq.deficit : (sReq.deficit / 20.0f)) * cfg->fVATSAccuracyDynCoef);
			}
		}

		if (cfg->WRF_Mult_ScopeStability) a_actor->SetActorValue(*cfg->WRF_Mult_ScopeStability, std::max(0.05f, swayM));
		if (cfg->WRF_Mult_ConeOfFire) a_actor->SetActorValue(*cfg->WRF_Mult_ConeOfFire, std::max(0.05f, spreadM));
		if (cfg->WRF_Mult_VATSAccuracy) a_actor->SetActorValue(*cfg->WRF_Mult_VATSAccuracy, std::clamp(vatsM, 0.05f, 5.0f));
		if (cfg->WRF_InsufficientStrength) cfg->WRF_InsufficientStrength->value = (strDef > 0) ? 1.0f : 0.0f;
		if (cfg->WRF_InsufficientSkill) cfg->WRF_InsufficientSkill->value = skillDef ? 1.0f : 0.0f;
		if (cfg->WRF_WeaponRequirementValue) a_actor->SetActorValue(*cfg->WRF_WeaponRequirementValue, hasW ? CalculateRequirement(a_actor, w, i) : 0.0f);
		if (cfg->WRF_StrengthDeficit) a_actor->SetActorValue(*cfg->WRF_StrengthDeficit, strDef);

		UpdateMainPerkInternal(a_actor);
	}

	void QueuePostLoadRefresh() {
		std::thread([]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(1500));
			if (auto task = F4SE::GetTaskInterface()) task->AddTask([]() { RefreshStatus(RE::PlayerCharacter::GetSingleton()); });
			}).detach();
	}

	void DumpWeaponDebugInfo(RE::TESObjectWEAP* a_weapon, RE::TBO_InstanceData* a_instance) {
		if (!a_weapon || !Config::GetSingleton()->bDebugMode) return;
		REX::INFO("==================================================");
		REX::INFO("[WRF 武器雷达] {}", GetFormName(a_weapon));
		REX::INFO("  -> FormID: {:08X}, Cat: {}", a_weapon->formID, GetWeaponCategory(a_weapon, a_instance));
		auto bk = static_cast<RE::BGSKeywordForm*>(a_weapon);
		if (bk && bk->keywords) for (uint32_t i = 0; i < bk->numKeywords; ++i) if (bk->keywords[i]) REX::INFO("  [基础] {}", GetFormName(bk->keywords[i]));
		if (a_instance) {
			auto ik = static_cast<RE::TESObjectWEAP::InstanceData*>(a_instance);
			if (ik && ik->keywords && ik->keywords->keywords)
				for (uint32_t i = 0; i < ik->keywords->numKeywords; ++i) if (ik->keywords->keywords[i]) REX::INFO("  [配件] {}", GetFormName(ik->keywords->keywords[i]));
		}
		REX::INFO("==================================================");
	}

	void OnCombatDamageCalculate(RE::Actor* attacker, RE::TESObjectWEAP* weapon, float* damagePtr) {
		if (!Config::GetSingleton()->bModEnabled || !attacker || !weapon || !damagePtr) return;

		bool isThrown = weapon->IsThrownWeapon();
		RE::TESObjectWEAP* eqWeap = nullptr;
		RE::TBO_InstanceData* eqInst = nullptr;

		if (!isThrown && GetEquippedMainWeapon(attacker, eqWeap, eqInst)) {
			auto mults = CalculateDamageMults(attacker, eqWeap, eqInst);

			// 🚀 核心修复：无论是近战还是枪械，实战伤害都只受技能惩罚影响
			*damagePtr = (*damagePtr) * mults.skillMult;
		}
		else if (isThrown && GetEquippedThrownWeapon(attacker, eqWeap, eqInst)) {
			auto mults = CalculateDamageMults(attacker, eqWeap, eqInst);
			*damagePtr = (*damagePtr) * mults.skillMult;
		}
	}

	class CoreHooks {
	public:
		static void HookInput(RE::BSInputEventReceiver* a_this, const RE::InputEvent* a_head) {
			auto cfg = Config::GetSingleton(); auto player = RE::PlayerCharacter::GetSingleton();
			if (cfg->bModEnabled && player) {
				static bool s_wasMov = false; bool curMov = IsPlayerMoving();
				if (curMov && !s_wasMov) {
					if (s_isCurrentlyGated && IsPlayerTryingToSprint()) TryShowSprintMsg();
					if (auto t = F4SE::GetTaskInterface()) t->AddTask([]() { RefreshStatus(RE::PlayerCharacter::GetSingleton()); });
				}
				s_wasMov = curMov;

				for (auto e = a_head; e; e = e->next) {
					if (e->eventType == RE::INPUT_EVENT_TYPE::kButton) {
						auto bEvent = static_cast<const RE::ButtonEvent*>(e);
						if (!bEvent->strUserEvent.empty()) {
							std::string key = bEvent->strUserEvent.c_str();

							// 核心逻辑：拦截攻击输入
							if ((key == "PrimaryAttack" || key == "RightAttack") && bEvent->value != 0) {
								// 判断当前时间是否在封锁期内
								if (std::chrono::steady_clock::now() < s_attackLockoutUntil.load()) {
									const_cast<RE::ButtonEvent*>(bEvent)->value = 0.0f; // 拦截输入
									if (cfg->bDebugMode) REX::INFO("[WRF Debug] 攻击输入被拦截 (力量不足)"); // 可选：打印拦截日志
								}
							}

							bool hold = (bEvent->value != 0); bool first = false;
							if (hold) { if (!s_heldKeys.count(key)) { first = true; s_heldKeys.insert(key); } }
							else s_heldKeys.erase(key);
							bool drawn = player->GetWeaponMagicDrawn();

							if ((key == "PrimaryAttack" || key == "RightAttack") && drawn && cfg->bPASoftGateEnabled && IsPlayerInPowerArmor(player)) {
								RE::TESObjectWEAP* w = nullptr; RE::TBO_InstanceData* i = nullptr;
								if (GetEquippedMainWeapon(player, w, i) && GetPAWeaponState(w, i) == 1) {
									if (cfg->iPenaltyMode == 0) {
										if (first) RE::SendHUDMessage::ShowHUDMessage("$PowerArmorWeaponRestrictionsMSG", "WRF_WeaponJamSound", false, true);
										const_cast<RE::ButtonEvent*>(bEvent)->value = 0.0f;
									}
									else if (cfg->iPenaltyMode == 1) {
										const_cast<RE::ButtonEvent*>(bEvent)->strUserEvent = cfg->strControl_Melee;
									}
								}
							}
							if (key == "Sprint" && first && s_isCurrentlyGated && curMov) TryShowSprintMsg();
							if (first && (key == "ReadyWeapon" || ((key == "PrimaryAttack" || key == "RightAttack") && !drawn))) {
								std::thread([]() { std::this_thread::sleep_for(std::chrono::milliseconds(150)); if (auto t = F4SE::GetTaskInterface()) t->AddTask([]() { RefreshStatus(RE::PlayerCharacter::GetSingleton()); }); }).detach();
							}
						}
					}
				}
			}
			_PerformInputProcessing(a_this, a_head);
			if (cfg->bModEnabled) {
				if (auto pc = RE::PlayerControls::GetSingleton()) {
					if (s_isRunGated) {
						float mag = std::sqrt(pc->data.moveInputVec.x * pc->data.moveInputVec.x + pc->data.moveInputVec.y * pc->data.moveInputVec.y);
						if (mag > 0.4f) { pc->data.moveInputVec.x *= (0.4f / mag); pc->data.moveInputVec.y *= (0.4f / mag); }
					}
					else if (g_tripFrames > 0) { pc->data.moveInputVec.x = pc->data.moveInputVec.y = 0; g_tripFrames--; }
				}
			}
		}

		static RE::BSEventNotifyControl HookProcessAnim(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_this, const RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_src)
		{
			// 🎯 核心修复：捕获拔/收武器事件，记录冲刺状态快照
			if (a_event.tag == "weaponDraw" || a_event.tag == "weaponSheathe" || a_event.tag == "heavyWeaponDraw") {
				s_recentSprintSnapshot = IsPlayerTryingToSprint();
				std::thread([]() { std::this_thread::sleep_for(std::chrono::milliseconds(50)); if (auto t = F4SE::GetTaskInterface()) t->AddTask([]() { RefreshStatus(RE::PlayerCharacter::GetSingleton()); }); }).detach();
			}

			auto player = RE::PlayerCharacter::GetSingleton();
			auto cfg = Config::GetSingleton();

			if (player && a_event.tag == "BeginMeleeAttack") {
				float deficit = GetStrengthDeficit(player);

				// 调试：打印当前的缺口值，确认逻辑是否被触发
				if (cfg->bDebugMode) REX::INFO("[WRF Debug] 攻击开始, 当前力量缺口: {:.2f}, 使用的惩罚系数: {:.2f}", deficit, cfg->fMeleeStrPenaltyMult);

				if (deficit > 0.0f) {
					// 使用 MCM 中的 fMeleeStrPenaltyMult 作为惩罚系数
					float penaltyTime = deficit * cfg->fMeleeStrPenaltyMult;

					// 只有当惩罚时间达到一定阈值（比如 0.1s）才锁定，避免轻微惩罚造成的手感断层
					if (penaltyTime > 0.1f) {
						s_attackLockoutUntil = std::chrono::steady_clock::now() + std::chrono::milliseconds(static_cast<int>(penaltyTime * 1000));
						if (cfg->bDebugMode) REX::INFO("[WRF Debug] 施加攻击粘滞: {:.2f}s", penaltyTime);
					}
				}
			}

			return _ProcessAnim(a_this, a_event, a_src);
		}

		static inline void(*_PerformInputProcessing)(RE::BSInputEventReceiver*, const RE::InputEvent*) = nullptr;
		static inline decltype(&HookProcessAnim) _ProcessAnim = nullptr;
	};

	class MenuEquipEvents : public RE::BSTEventSink<RE::MenuOpenCloseEvent>, public RE::BSTEventSink<RE::ActorEquipManagerEvent::Event> {
	public:
		RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent&, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
			// [MOVED TO MCM] LoadAllSettings removed - reloads only on game load now
			return RE::BSEventNotifyControl::kContinue;
		}
		RE::BSEventNotifyControl ProcessEvent(const RE::ActorEquipManagerEvent::Event& a_event, RE::BSTEventSource<RE::ActorEquipManagerEvent::Event>*) override {
			if (a_event.actorAffected && a_event.actorAffected->IsPlayerRef()) {
				s_recentSprintSnapshot = IsPlayerTryingToSprint();
				if (a_event.changeType == RE::ActorEquipManagerEvent::Type::kEquip && a_event.itemAffected) {
					if (auto w = a_event.itemAffected->object->As<RE::TESObjectWEAP>()) DumpWeaponDebugInfo(w, a_event.itemAffected->instanceData.get());
				}
				if (auto t = F4SE::GetTaskInterface()) t->AddUITask([]() { RefreshStatus(RE::PlayerCharacter::GetSingleton()); });
			}
			return RE::BSEventNotifyControl::kContinue;
		}
	} static g_events;

	void InstallHooks() {
		DWORD oldProtect;

		if (auto pc = RE::PlayerControls::GetSingleton()) {
			auto vtable = *(uintptr_t**)static_cast<RE::BSInputEventReceiver*>(pc);
			CoreHooks::_PerformInputProcessing = reinterpret_cast<decltype(CoreHooks::_PerformInputProcessing)>(vtable[0]);

			VirtualProtect(&vtable[0], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
			vtable[0] = reinterpret_cast<uintptr_t>(CoreHooks::HookInput);
			VirtualProtect(&vtable[0], sizeof(void*), oldProtect, &oldProtect);
		}
		if (auto p = RE::PlayerCharacter::GetSingleton()) {
			auto vtable = *(uintptr_t**)static_cast<RE::BSTEventSink<RE::BSAnimationGraphEvent>*>(p);
			CoreHooks::_ProcessAnim = reinterpret_cast<decltype(CoreHooks::_ProcessAnim)>(vtable[1]);

			VirtualProtect(&vtable[1], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
			vtable[1] = reinterpret_cast<uintptr_t>(CoreHooks::HookProcessAnim);
			VirtualProtect(&vtable[1], sizeof(void*), oldProtect, &oldProtect);
		}

		auto eqMgr = RE::ActorEquipManager::GetSingleton();
		if (eqMgr) eqMgr->RegisterSink(&g_events);
		if (auto ui = RE::UI::GetSingleton()) ui->GetEventSource<RE::MenuOpenCloseEvent>()->RegisterSink(&g_events);
	}
}
