// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Layout {
struct FramebufferLayout;
}

namespace Vulkan {

class Device;
class MemoryAllocator;
class Scheduler;
struct PresentPushConstants;

// The playtest notice drawn over the picture (playtest builds only, see vk_blit_screen.cpp). The text
// is rasterized once with FreeType from a system font; without FreeType or a font it stays empty.
class Watermark final {
public:
    explicit Watermark(const Device& device, MemoryAllocator& memory_allocator, Scheduler& scheduler);
    ~Watermark();

    [[nodiscard]] bool IsReady() const {
        return ready;
    }

    [[nodiscard]] VkDescriptorSet GetDescriptorSet() const {
        return descriptor_sets[0];
    }

    // Centred in the screen rectangle, about 60% of its width.
    void ConfigureDraw(PresentPushConstants& out, const Layout::FramebufferLayout& layout) const;

private:
    bool ready = false;
    u32 width = 0;
    u32 height = 0;
    vk::Image image;
    vk::ImageView image_view;
    vk::Sampler sampler;
    vk::DescriptorSetLayout descriptor_set_layout;
    vk::DescriptorPool descriptor_pool;
    vk::DescriptorSets descriptor_sets;
};

} // namespace Vulkan
