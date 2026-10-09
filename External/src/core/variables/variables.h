#pragma once
#include "../../../ext/imgui/imgui.h"

namespace variables {
inline bool menuOpen = false;
inline int selectedTab = 0;
inline bool waitingForKey = false;
inline int* keyToRebind = nullptr;
inline bool teamCheck = false;

namespace Aimbot {
inline bool enabled = false;
inline bool showFOV = false;
inline float fovRadius = 100.0f;
inline float smoothing = 5.0f;
inline int aimTarget = 0;
inline int aimMethod = 1;
inline int aimbotKey = 2;
inline ImVec4 fovColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
inline bool visibleCheck = false;
inline bool useDeadzone = false;
inline float deadzone = 5.0f;
inline bool triggerbot = false;
inline int triggerKey = 6;
inline float triggerFov = 10.0f;
inline int triggerDelay = 100;
}

namespace ESP {
inline bool enabled = false;
inline bool boxes = false;
inline bool names = false;
inline bool distance = false;
inline bool healthBar = false;
inline bool skeleton = false;
inline float skeletonThickness = 2.0f;
inline bool skeletonOutline = true;
inline bool deadCheck = true;
inline bool localPlayer = false;
inline bool tool = false;
inline bool workspaceScan = true;
inline ImVec4 toolColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
inline ImVec4 boxColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
inline int boxStyle = 0;
inline int boxMode = 0;
inline bool boxFilled = false;
inline bool boxFillGradient = false;
inline ImVec4 boxFillColor = ImVec4(1.0f, 1.0f, 1.0f, 0.25f);
inline ImVec4 boxFillColor2 = ImVec4(0.2f, 0.4f, 1.0f, 0.25f);
inline ImVec4 nameColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
inline ImVec4 distanceColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
inline ImVec4 healthColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
inline ImVec4 skeletonColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
}

namespace Wallshot {
inline bool enabled = false;
inline float wallAlpha = 0.5f;
}

namespace Visual {
inline bool fullbright = false;
inline float fullbrightBrightness = 5.0f;
}

namespace Aspect {
inline bool enabled = false;
inline float scaleX = 1.0f;
inline float scaleY = 1.0f;
inline float scaleZ = 1.0f;
}

namespace World {
inline bool timeEnabled = false;
inline float clockTime = 14.0f;
inline bool skyEnabled = false;
inline float sunSize = 11.0f;
inline float moonSize = 11.0f;
inline float starCount = 3000.0f;
}

namespace Local {
inline bool speedEnabled = false;
inline float walkSpeed = 16.0f;
inline bool jumpEnabled = false;
inline float jumpPower = 50.0f;
}

namespace Misc {
inline bool streamProof = false;
inline bool watermark = false;
inline bool keybinds = true;
inline bool vsync = false;
inline int fpsLimit = 0;
inline int priority = 1;
inline float menuFontSize = 1.0f;
inline float animSpeed = 1.0f;
inline int theme = 0;
inline float espFontSize = 13.0f;
}

namespace Movement {
inline bool fov = false;
inline int fovKey = 0;
inline int fovKeyMode = 0;
inline float fovValue = 70.0f;
inline bool fly = false;
inline int flyKey = 0;
inline int flyKeyMode = 0;
inline int flyMethod = 0;
inline float flySpeed = 150.0f;
inline float flyVerticalBoost = 1.5f;
inline bool flyBypass = true;
inline bool noclip = false;
inline int noclipKey = 0;
inline int noclipKeyMode = 0;
inline int noclipMode = 0;
inline bool bunnyHop = false;
inline float bunnyHopSpeed = 32.0f;
inline bool hipHeight = false;
inline float hipHeightValue = 2.0f;
}

namespace Client {
inline const char* name = "stellarsense.fun";
inline const char* plan = "Lifetime";
inline const char* expires = "Never";
}

namespace Theme {
inline ImVec4 background = ImVec4(0.0392f, 0.0392f, 0.0471f, 1.0f);
inline ImVec4 panels = ImVec4(0.0745f, 0.0745f, 0.0863f, 1.0f);
inline ImVec4 controls = ImVec4(0.1176f, 0.1176f, 0.1333f, 1.0f);
inline ImVec4 accent = ImVec4(0.0392f, 0.5176f, 1.0f, 1.0f);
inline ImVec4 text = ImVec4(0.8200f, 0.8200f, 0.8400f, 1.0f);
inline ImVec4 textBright = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
}
}
