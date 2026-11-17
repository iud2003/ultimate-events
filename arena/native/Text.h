#pragma once
#include "State.h"
#include <vector>
// DirectWrite + WIC: CPU text rasterization, no GUI application or GL on workers.
std::vector<unsigned char> rasterText(const Json& settings,int width,int height,double elapsed);
