#include "wallshot.h"
#include "../../variables/variables.h"
#include "../../globals/globals.h"
#include "../../cache/cache.h"
#include "../../../memory/memory.h"
#include "../../../sdk/offsets.h"
#include "../../../sdk/sdk.h"
#include <thread>
#include <chrono>
#include <atomic>

namespace Wallshot {
std::atomic<int> modifiedCount{0};

void Loop() {}
}
