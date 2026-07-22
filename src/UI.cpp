#include "pch.h"
#include "UI.h"
#include "Config.h"
#include "Mechanics.h"
#include "IIF_API.h"

#include <string>
#include <cmath>

namespace WRF::UI
{
    using namespace RE;

    // ==========================================
    // 🛠️ 工具函数：判断武器是否正被玩家装备
    // ==========================================
    bool IsEq(PlayerCharacter* p, TESObjectWEAP* w, TBO_InstanceData* i) {
        if (!p || !p->currentProcess || !p->currentProcess->middleHigh) return false;
        for (auto& it : p->currentProcess->middleHigh->equippedItems) {
            if (it.item.object == w && it.item.instanceData.get() == i) return true;
        }
        return false;
    }

    // ==========================================
    // 👑 框架通讯回调：响应 IIF 广播并注入需求与伤害
    // ==========================================
    void OnIIFMessage(IIF_API::UpdateMessage* msg) {
        if (!msg || !msg->itemForm || !msg->gfxArray) return;

        auto cfg = Config::GetSingleton();
        if (!cfg->bModEnabled || !cfg->bShowUI) return;

        auto player = PlayerCharacter::GetSingleton();
        if (!player) return;

        auto weapon = static_cast<RE::TESForm*>(msg->itemForm)->As<RE::TESObjectWEAP>();
        auto ammo = static_cast<RE::TESForm*>(msg->itemForm)->As<RE::TESAmmo>();
        auto instance = static_cast<RE::TBO_InstanceData*>(msg->instanceData);
        auto item = static_cast<RE::BGSInventoryItem*>(msg->inventoryItem);

        // ------------------------------------------
        // 🔫 武器卡片处理逻辑
        // ------------------------------------------
        if (weapon) {
            // --- 1. 力量与技能需求卡片渲染 ---
            float strReq = Mechanics::CalculateRequirement(player, weapon, instance);
            float strDeficit = Mechanics::GetStrengthDeficit(player, weapon, instance);
            auto skillReq = Mechanics::GetSkillRequirement(player, weapon, instance);

            bool hasStr = (strReq > 0.0f);			int strInt = static_cast<int>(strReq);
            std::string strValStr = std::to_string(strInt);
            bool strFail = (strDeficit > 0.0f);

            bool hasSkill = skillReq.hasReq;
            bool skillFail = hasSkill ? (skillReq.deficit > 0.0f) : false;
            std::string skillIconStr = hasSkill ? skillReq.icon : "";
            std::string skillValStr = hasSkill ? std::to_string(skillReq.reqValue) : "";

            if (hasStr || hasSkill) {
                IIF_API::CardRequest card{};

                card.text = "$StrengthReq";
                card.value = hasStr ? strValStr.c_str() : "0";
                card.label = "$REQUIRES";
                card.highlightLabel = false;
                card.displayType = 2;
                card.hasBackground = true;
                card.hideDifference = true;

                if (hasSkill) {
                    card.icon1IsText = false;
                    card.icon1 = skillIconStr.c_str();
                    card.val1 = skillValStr.c_str();
                    card.val1Bad = skillFail;
                    card.val1Good = !skillFail;
                    card.align1 = "left";
                }
                else {
                    card.icon1IsText = true;
                    card.icon1 = ""; card.val1 = "";
                    card.val1Bad = false; card.val1Good = false;
                }

                if (hasStr) {
                    card.icon2IsText = false;
                    card.icon2 = "[Unarmed]";
                    card.val2 = strValStr.c_str();
                    card.val2Bad = strFail;
                    card.val2Good = !strFail;
                    card.align2 = "right";
                }
                else {
                    card.icon2IsText = true;
                    card.icon2 = ""; card.val2 = "";
                    card.val2Bad = false; card.val2Good = false;
                }
                msg->addCard(msg->context, &card);
            }

            // --- 2. 👑 IIF 原生面板降伤 ---
            bool shDmg = (cfg->iUIDamageDisplayMode == 1) ? (item && item >= player->inventoryList->data.begin() && item < player->inventoryList->data.end()) : ((cfg->iUIDamageDisplayMode == 2) ? IsEq(player, weapon, instance) : true);

            if (shDmg && msg->modifyCard) {
                auto m = Mechanics::CalculateDamageMults(player, weapon, instance);

                // 🚀 核心修复：力量已不再影响伤害，所有武器的伤害衰减统一看技能 (skillMult)
                float specMult = m.skillMult;

                if (specMult != 1.0f) {
                    msg->modifyCard(msg->context, "$dmg", static_cast<double>(specMult));
                }
            }
        }
        // ------------------------------------------
        // 📦 弹药卡片处理逻辑
        // ------------------------------------------
        else if (ammo && cfg->bEnableAmmoReq) {
            int ammoReq = Mechanics::GetAmmoRequirement(ammo);
            if (ammoReq >= 0) {
                IIF_API::CardRequest card{};

                card.text = "$StrengthReq";
                card.label = "$REQUIRES";
                card.highlightLabel = false;

                std::string valStr = std::to_string(ammoReq);
                card.value = valStr.c_str();

                card.displayType = 2;
                card.hasBackground = true;
                card.hideDifference = true;

                card.icon1IsText = true;
                card.icon1 = "";
                card.val1 = "";
                card.val1Bad = false;
                card.val1Good = false;

                card.icon2IsText = false;
                card.icon2 = "[Unarmed]";
                card.val2 = valStr.c_str();
                card.val2Bad = false;
                card.val2Good = true;
                card.align2 = "right";

                msg->addCard(msg->context, &card);
            }
        }
    }
}
