#pragma once
#include <atomic>

namespace Wallshot {
extern std::atomic<int> modifiedCount;
void Loop();
}
