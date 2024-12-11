#pragma once

#include "imgui.h"
#include "implot.h"

#include "dataTypes.h"

namespace Colors {
ImU32 ChangeBrightness(ImVec4 startColor, float changedBrightness) {
    ImVec4 changedColor = startColor;
    if (changedBrightness > 1 || changedBrightness < -1) {
        return ImGui::GetColorU32(startColor);
    }

    changedColor.x = changedColor.x * (1 + changedBrightness);
    changedColor.y = changedColor.y * (1 + changedBrightness);
    changedColor.z = changedColor.z * (1 + changedBrightness);

    if (changedColor.x > 1) {
        changedColor.x = 1;
    }
    if (changedColor.y > 1) {
        changedColor.y = 1;
    }
    if (changedColor.z > 1) {
        changedColor.z = 1;
    }

    return ImGui::GetColorU32(changedColor);
}
}  // namespace Colors