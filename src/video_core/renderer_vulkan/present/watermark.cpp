// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <filesystem>
#include <string_view>
#include <vector>

#ifdef HAS_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

#include "common/logging.h"
#include "core/frontend/framebuffer_layout.h"
#include "video_core/renderer_vulkan/present/present_push_constants.h"
#include "video_core/renderer_vulkan/present/util.h"
#include "video_core/renderer_vulkan/present/watermark.h"
#include "video_core/vulkan_common/vulkan_device.h"

namespace Vulkan {

namespace {

#ifdef OUNCE2DN_PLAYTEST
// Playtest releases always carry this notice (see vk_blit_screen.cpp).
constexpr std::array<std::string_view, 2> Lines{
    "Droplet Online playtest",
    "build " OUNCE2DN_BUILD_DATE " - not for distribution",
};
#else
constexpr std::array<std::string_view, 2> Lines{
    "Experimental Nintendo Switch 2 Emulator Prototype",
    "Not intended for public release.",
};
#endif

constexpr std::array<const char*, 8> FontPaths{
    "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    "/usr/share/fonts/dejavu/DejaVuSans-Bold.ttf",
    "/usr/share/fonts/noto/NotoSans-Bold.ttf",
    "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
    "/usr/share/fonts/google-noto/NotoSans-Bold.ttf",
    "C:/Windows/Fonts/segoeuib.ttf",
    "C:/Windows/Fonts/arialbd.ttf",
};

constexpr u32 PixelSize = 64;   // glyph height the text is rasterized at, then scaled on screen
constexpr u32 Padding = 8;      // room for the outline
constexpr int OutlineRadius = 3;
constexpr float Opacity = 0.4f; // of the whole notice: white text in a dark outline

struct Bitmap {
    u32 width = 0;
    u32 height = 0;
    std::vector<u8> rgba; // premultiplied
};

#ifdef HAS_FREETYPE
// White text with a dark outline, so it reads on both bright and dark scenes.
Bitmap RenderText() {
    FT_Library library{};
    if (FT_Init_FreeType(&library) != 0) {
        return {};
    }
    FT_Face face{};
    for (const char* path : FontPaths) {
        if (std::filesystem::exists(path) && FT_New_Face(library, path, 0, &face) == 0) {
            break;
        }
        face = nullptr;
    }
    if (face == nullptr) {
        LOG_WARNING(Render_Vulkan, "Watermark: no font found, not shown");
        FT_Done_FreeType(library);
        return {};
    }
    FT_Set_Pixel_Sizes(face, 0, PixelSize);
    const int ascender = static_cast<int>(face->size->metrics.ascender >> 6);
    const int line_height = static_cast<int>(face->size->metrics.height >> 6);

    std::array<int, Lines.size()> line_widths{};
    for (size_t i = 0; i < Lines.size(); ++i) {
        for (const char c : Lines[i]) {
            if (FT_Load_Char(face, static_cast<FT_ULong>(c), FT_LOAD_DEFAULT) == 0) {
                line_widths[i] += static_cast<int>(face->glyph->advance.x >> 6);
            }
        }
    }
    const int text_width = *std::max_element(line_widths.begin(), line_widths.end());
    const u32 width = static_cast<u32>(text_width) + 2 * Padding;
    const u32 height = static_cast<u32>(line_height) * static_cast<u32>(Lines.size()) + 2 * Padding;

    std::vector<u8> coverage(size_t{width} * height);
    for (size_t i = 0; i < Lines.size(); ++i) {
        int pen_x = static_cast<int>(Padding) + (text_width - line_widths[i]) / 2;
        const int baseline = static_cast<int>(Padding) + ascender + static_cast<int>(i) * line_height;
        for (const char c : Lines[i]) {
            if (FT_Load_Char(face, static_cast<FT_ULong>(c), FT_LOAD_RENDER) != 0) {
                continue;
            }
            const FT_Bitmap& glyph = face->glyph->bitmap;
            const int left = pen_x + face->glyph->bitmap_left;
            const int top = baseline - face->glyph->bitmap_top;
            for (u32 y = 0; y < glyph.rows; ++y) {
                for (u32 x = 0; x < glyph.width; ++x) {
                    const int px = left + static_cast<int>(x);
                    const int py = top + static_cast<int>(y);
                    if (px < 0 || py < 0 || px >= static_cast<int>(width) ||
                        py >= static_cast<int>(height)) {
                        continue;
                    }
                    u8& out = coverage[static_cast<size_t>(py) * width + static_cast<size_t>(px)];
                    out = (std::max)(out, glyph.buffer[y * static_cast<u32>(glyph.pitch) + x]);
                }
            }
            pen_x += static_cast<int>(face->glyph->advance.x >> 6);
        }
    }
    FT_Done_Face(face);
    FT_Done_FreeType(library);

    // The outline: the coverage grown by OutlineRadius (a max filter, rows then columns).
    std::vector<u8> rows(coverage.size());
    std::vector<u8> outline(coverage.size());
    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            u8 value = 0;
            for (int d = -OutlineRadius; d <= OutlineRadius; ++d) {
                const int px = std::clamp(static_cast<int>(x) + d, 0, static_cast<int>(width) - 1);
                value = (std::max)(value, coverage[size_t{y} * width + static_cast<size_t>(px)]);
            }
            rows[size_t{y} * width + x] = value;
        }
    }
    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            u8 value = 0;
            for (int d = -OutlineRadius; d <= OutlineRadius; ++d) {
                const int py = std::clamp(static_cast<int>(y) + d, 0, static_cast<int>(height) - 1);
                value = (std::max)(value, rows[static_cast<size_t>(py) * width + x]);
            }
            outline[size_t{y} * width + x] = value;
        }
    }

    Bitmap bitmap{.width = width, .height = height, .rgba = std::vector<u8>(size_t{width} * height * 4)};
    for (size_t i = 0; i < coverage.size(); ++i) {
        const float text = coverage[i] / 255.0f;
        const float edge = outline[i] / 255.0f;
        // White text over a black outline, premultiplied, then the notice's opacity.
        const float alpha = (text + edge * (1.0f - text)) * Opacity;
        const u8 white = static_cast<u8>(text * Opacity * 255.0f + 0.5f);
        bitmap.rgba[i * 4 + 0] = white;
        bitmap.rgba[i * 4 + 1] = white;
        bitmap.rgba[i * 4 + 2] = white;
        bitmap.rgba[i * 4 + 3] = static_cast<u8>(alpha * 255.0f + 0.5f);
    }
    return bitmap;
}
#else
Bitmap RenderText() {
    LOG_WARNING(Render_Vulkan, "Watermark: built without FreeType, not shown");
    return {};
}
#endif

} // Anonymous namespace

Watermark::Watermark(const Device& device, MemoryAllocator& memory_allocator, Scheduler& scheduler) {
    const Bitmap bitmap = RenderText();
    if (bitmap.width == 0 || bitmap.height == 0) {
        return;
    }
    width = bitmap.width;
    height = bitmap.height;
    const VkExtent2D extent{.width = width, .height = height};
    image = CreateWrappedImage(memory_allocator, extent, VK_FORMAT_R8G8B8A8_UNORM);
    UploadImage(device, memory_allocator, scheduler, image, extent, VK_FORMAT_R8G8B8A8_UNORM,
                bitmap.rgba);
    image_view = CreateWrappedImageView(device, image, VK_FORMAT_R8G8B8A8_UNORM);
    sampler = CreateBilinearSampler(device);

    // The same layout as the window adapt pass's, so the set binds with its pipeline layout.
    descriptor_set_layout =
        CreateWrappedDescriptorSetLayout(device, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER});
    descriptor_pool = CreateWrappedDescriptorPool(device, 1, 1);
    const std::vector<VkDescriptorSetLayout> layouts{*descriptor_set_layout};
    descriptor_sets = CreateWrappedDescriptorSets(descriptor_pool, layouts);

    std::vector<VkDescriptorImageInfo> images;
    images.reserve(1);
    const VkWriteDescriptorSet write = CreateWriteDescriptorSet(images, *sampler, *image_view,
                                                                descriptor_sets[0], 0);
    device.GetLogical().UpdateDescriptorSets(std::array{write}, {});
    ready = true;
    LOG_INFO(Render_Vulkan, "Watermark: {}x{} texture", width, height);
}

Watermark::~Watermark() = default;

void Watermark::ConfigureDraw(PresentPushConstants& out, const Layout::FramebufferLayout& layout) const {
    out.modelview_matrix = MakeOrthographicMatrix(f32(layout.width), f32(layout.height));
    const auto& screen = layout.screen;
    const f32 w = f32(screen.GetWidth()) * 0.6f;
    const f32 h = w * f32(height) / f32(width);
    const f32 x = f32(screen.left) + (f32(screen.GetWidth()) - w) / 2.0f;
    const f32 y = f32(screen.top) + (f32(screen.GetHeight()) - h) / 2.0f;
    out.vertices[0] = ScreenRectVertex(x, y, 0.0f, 0.0f);
    out.vertices[1] = ScreenRectVertex(x + w, y, 1.0f, 0.0f);
    out.vertices[2] = ScreenRectVertex(x, y + h, 0.0f, 1.0f);
    out.vertices[3] = ScreenRectVertex(x + w, y + h, 1.0f, 1.0f);
}

} // namespace Vulkan
