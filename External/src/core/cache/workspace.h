#pragma once
#include "../../sdk/sdk.h"
#include <vector>
#include <cstdint>

namespace WorkspaceCache {
struct CachedPart {
    std::uintptr_t prim = 0;
    RBX::Vec3 pos{};
    RBX::Vec3 size{};
    float rot[9]{};
};

bool IsVisible(const RBX::Vec3& camPos, const RBX::Vec3& targetPos);
void Update();
void Loop();
}
