#pragma once
#include "../../../src/sdk/sdk.h"
#include "../globals/globals.h"
#include "../variables/variables.h"
#include <vector>
#include <string>
#include <utility>
#include <chrono>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace PlayerCache {
struct CachedPlayer {
    std::uintptr_t playerAddr = 0;
    std::uintptr_t characterAddr = 0;
    std::uintptr_t humanoidAddr = 0;
    std::uintptr_t rootPartAddr = 0;
    std::uintptr_t headAddr = 0;
    std::uintptr_t teamAddr = 0;
    std::string name;
    std::string tool = "None";
    RBX::Vec3 position{};
    float health = 0.0f;
    float maxHealth = 0.0f;
    float distance = 0.0f;
    bool isValid = false;
    bool isR6 = false;
};

struct LimbAddrs {
    bool r6 = false;
    std::uintptr_t head = 0;
    std::uintptr_t hrp = 0;
    std::uintptr_t torso = 0;
    std::uintptr_t upperTorso = 0;
    std::uintptr_t lowerTorso = 0;
    std::uintptr_t lUpperArm = 0;
    std::uintptr_t lLowerArm = 0;
    std::uintptr_t lHand = 0;
    std::uintptr_t rUpperArm = 0;
    std::uintptr_t rLowerArm = 0;
    std::uintptr_t rHand = 0;
    std::uintptr_t lUpperLeg = 0;
    std::uintptr_t lLowerLeg = 0;
    std::uintptr_t lFoot = 0;
    std::uintptr_t rUpperLeg = 0;
    std::uintptr_t rLowerLeg = 0;
    std::uintptr_t rFoot = 0;
    std::uintptr_t lArm = 0;
    std::uintptr_t rArm = 0;
    std::uintptr_t lLeg = 0;
    std::uintptr_t rLeg = 0;
    std::uintptr_t humanoid = 0;
    bool hasRealHumanoid = false;
};

inline std::unordered_map<std::uintptr_t, LimbAddrs> limbCache;

inline const LimbAddrs& GetLimbs(std::uintptr_t characterAddr) {
    auto it = limbCache.find(characterAddr);
    if (it != limbCache.end())
        return it->second;
    if (limbCache.size() >= 256)
        limbCache.clear();
    LimbAddrs l{};
    RBX::RbxInstance ch{characterAddr};
    auto head = ch.FindChild("Head");
    auto hrp = ch.FindChild("HumanoidRootPart");
    l.head = head.Addr;
    l.hrp = hrp.Addr;
    auto torso = ch.FindChild("Torso");
    l.r6 = torso.Addr != 0;
    l.torso = torso.Addr;
    if (l.r6) {
        l.lArm = ch.FindChild("Left Arm").Addr;
        l.rArm = ch.FindChild("Right Arm").Addr;
        l.lLeg = ch.FindChild("Left Leg").Addr;
        l.rLeg = ch.FindChild("Right Leg").Addr;
    } else {
        l.upperTorso = ch.FindChild("UpperTorso").Addr;
        l.lowerTorso = ch.FindChild("LowerTorso").Addr;
        l.lUpperArm = ch.FindChild("LeftUpperArm").Addr;
        l.lLowerArm = ch.FindChild("LeftLowerArm").Addr;
        l.lHand = ch.FindChild("LeftHand").Addr;
        l.rUpperArm = ch.FindChild("RightUpperArm").Addr;
        l.rLowerArm = ch.FindChild("RightLowerArm").Addr;
        l.rHand = ch.FindChild("RightHand").Addr;
        l.lUpperLeg = ch.FindChild("LeftUpperLeg").Addr;
        l.lLowerLeg = ch.FindChild("LeftLowerLeg").Addr;
        l.lFoot = ch.FindChild("LeftFoot").Addr;
        l.rUpperLeg = ch.FindChild("RightUpperLeg").Addr;
        l.rLowerLeg = ch.FindChild("RightLowerLeg").Addr;
        l.rFoot = ch.FindChild("RightFoot").Addr;
    }
    const auto humNode = ch.FindChildByClass("Humanoid");
    if (humNode.Addr) {
        l.humanoid = humNode.Addr;
        l.hasRealHumanoid = true;
    } else {
        l.humanoid = ch.FindChildByClass("AnimationController").Addr;
        l.hasRealHumanoid = false;
    }
    auto res = limbCache.emplace(characterAddr, l);
    return res.first->second;
}

inline void PruneLimbs(const std::unordered_set<std::uintptr_t>& alive) {
    for (auto it = limbCache.begin(); it != limbCache.end();) {
        if (alive.find(it->first) == alive.end())
            it = limbCache.erase(it);
        else
            ++it;
    }
}

inline std::vector<CachedPlayer> players;
inline RBX::Vec3 localPlayerPos{};
inline std::uintptr_t localPlayerTeam = 0;
inline std::uintptr_t localRootPrim = 0;

inline void updateplayers() {
    if (!Globals::players.Addr || !Globals::localPlayer.Addr) {
        players.clear();
        localRootPrim = 0;
        return;
    }
    if (localRootPrim) {
        localPlayerPos = memory->read<RBX::Vec3>(localRootPrim + Offsets::Primitive::Position);
        for (auto& c : players) {
            if (!c.isValid)
                continue;
            if (c.maxHealth > 0) {
                c.health = memory->read<float>(c.humanoidAddr + Offsets::Humanoid::Health);
                if (variables::ESP::deadCheck && c.health <= 0)
                    c.isValid = false;
            }
        }
    }
    using Clock = std::chrono::steady_clock;
    static auto lastTopo = Clock::now() - std::chrono::seconds(10);
    if (Clock::now() - lastTopo < std::chrono::milliseconds(100))
        return;
    lastTopo = Clock::now();

    auto localChar = Globals::localPlayer.GetModelRef();
    if (!localChar.Addr) {
        players.clear();
        localRootPrim = 0;
        return;
    }
    auto localRoot = localChar.FindChild("HumanoidRootPart");
    if (!localRoot.Addr) {
        players.clear();
        localRootPrim = 0;
        return;
    }
    localRootPrim = localRoot.GetPrimitivePtr();
    localPlayerPos = localRoot.GetPos();
    localPlayerTeam = memory->read<std::uintptr_t>(Globals::localPlayer.Addr + Offsets::Player::Team);

    auto list = Globals::players.GetChildList();
    std::unordered_set<std::uintptr_t> alive;
    alive.reserve(list.size() + 1);
    alive.insert(localChar.Addr);

    for (auto& plr : players)
        plr.isValid = false;

    for (auto& plr : list) {
        if (plr.Addr == Globals::localPlayer.Addr)
            continue;
        const auto character = plr.GetModelRef();
        if (!character.Addr)
            continue;
        alive.insert(character.Addr);
        CachedPlayer* slot = nullptr;
        for (auto& c : players) {
            if (c.playerAddr == plr.Addr) {
                slot = &c;
                break;
            }
        }
        if (slot && slot->characterAddr == character.Addr && slot->isValid) {
            if (slot->maxHealth > 0) {
                slot->health = memory->read<float>(slot->humanoidAddr + Offsets::Humanoid::Health);
                if (variables::ESP::deadCheck && slot->health <= 0) {
                    slot->isValid = false;
                    continue;
                }
            }
            continue;
        }
        const auto& limbs = GetLimbs(character.Addr);
        if (!limbs.hrp || !limbs.head)
            continue;
        float hp = 0.0f;
        float maxHp = 0.0f;
        if (limbs.hasRealHumanoid && limbs.humanoid) {
            hp = memory->read<float>(limbs.humanoid + Offsets::Humanoid::Health);
            maxHp = memory->read<float>(limbs.humanoid + Offsets::Humanoid::MaxHealth);
            if (variables::ESP::deadCheck && maxHp > 0 && hp <= 0)
                continue;
        }
        const auto team = memory->read<std::uintptr_t>(plr.Addr + Offsets::Player::Team);
        if (variables::teamCheck && team && team == localPlayerTeam)
            continue;
        CachedPlayer c{};
        c.playerAddr = plr.Addr;
        c.characterAddr = character.Addr;
        c.humanoidAddr = limbs.humanoid;
        c.rootPartAddr = limbs.hrp;
        c.headAddr = limbs.head;
        c.teamAddr = team;
        c.name = plr.GetName();
        c.health = hp;
        c.maxHealth = maxHp;
        c.isR6 = limbs.r6;
        c.isValid = true;
        for (auto& child : RBX::RbxInstance(character.Addr).GetChildList()) {
            if (child.GetClass() == "Tool") {
                c.tool = child.GetName();
                break;
            }
        }
        if (slot)
            *slot = std::move(c);
        else
            players.push_back(std::move(c));
    }

    players.erase(std::remove_if(players.begin(), players.end(), [](const CachedPlayer& c) { return !c.isValid; }), players.end());
    PruneLimbs(alive);
}
}
