// Included by runtime_ui.cpp inside its UI namespace, after
// runtime_settings_ui.inl. Presentation only: every setting goes through the
// same renderer configuration and enhancement calls as before.
//
// GRAPHICS in the look of tools/launcher-html (pages/graphics.js,
// styles/graphics.css): Game display with Renderer & performance folded under
// it, then HUD & effects beside Vehicles & scenery, then the footer that
// restores the defaults. The study has fewer settings than the runtime; the
// extra ones sit in the card they belong to, the rarely used ones behind
// disclosures. Accurate keeps only what it always offered.

struct GraphicsPageState {
    ImGuiContext* context = nullptr;
    int last_frame = -100;
    bool renderer_open = false;
    bool distances_open = false;
    // Restoring the defaults can be undone until the next graphics change or
    // until the page is left.
    bool can_undo = false;
    std::string status;
    bool hud_save_failed = false;
};
GraphicsPageState g_graphics_page;

SettingsRangeValue GraphicsTimes(int value) { return {std::to_string(value), "x"}; }

// A colour swatch field for the performance overlay's text.
bool GraphicsColourField(const char* id, std::string_view label, ImVec4& colour, float width) {
    SettingsLabel(label, width);
    ImGui::PushID(id);
    const PaddockPress press = PaddockBeginPress("##swatch", {width, 52.0F});
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const PaddockRadii radii = PaddockRound(8.0F);
    PaddockFill(draw, press.min, press.max, radii,
                PaddockMix(PaddockRgb(0x061D2C), PaddockRgb(0x0A2A3D), press.hover));
    PaddockStroke(draw, press.min, press.max, radii,
                  PaddockMix(PaddockRgb(0x2F6A86), PaddockRgb(0x5A97B1), press.hover), 1.0F);
    const ImVec2 swatch{press.min.x + 12.0F, press.min.y + 12.0F};
    PaddockFill(draw, swatch, {swatch.x + 28.0F, swatch.y + 28.0F}, PaddockRound(6.0F),
                ImGui::ColorConvertFloat4ToU32(colour));
    PaddockStroke(draw, swatch, {swatch.x + 28.0F, swatch.y + 28.0F}, PaddockRound(6.0F),
                  PaddockRgb(0xFFFFFF, 60U), 1.0F);
    char hex[16]{};
    std::snprintf(hex, sizeof(hex), "#%02X%02X%02X", static_cast<int>(colour.x * 255.0F + 0.5F),
                  static_cast<int>(colour.y * 255.0F + 0.5F), static_cast<int>(colour.z * 255.0F + 0.5F));
    const PaddockType type = PaddockReading(16.0F, true, 1.25F);
    PaddockDrawRun(draw, type, {swatch.x + 28.0F + 12.0F, std::round(press.min.y + (52.0F - type.line) * 0.5F)},
                   PaddockRgb(0xFFFFFF), hex, hex + std::strlen(hex));
    PaddockEndPress(press, 8.0F, 0.98F);
    if (press.pressed) ImGui::OpenPopup("##colour");
    bool changed = false;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {12.0F, 12.0F});
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 12.0F);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, PaddockRgb(0x071C29));
    ImGui::PushStyleColor(ImGuiCol_Border, PaddockRgb(0x3C6478));
    if (ImGui::BeginPopup("##colour")) {
        changed = ImGui::ColorPicker4("##picker", &colour.x,
                                      ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf |
                                          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoSidePreview);
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::PopID();
    return changed;
}

void DrawGraphicsPage(float available_width, bool live) {
    namespace enh = dkr::runtime::enhancements;
    GraphicsPageState& state = g_graphics_page;
    const int frame = ImGui::GetFrameCount();
    const bool entered = state.context != ImGui::GetCurrentContext() || state.last_frame != frame - 1;
    if (entered) {
        state.context = ImGui::GetCurrentContext();
        state.can_undo = false;
        state.status.clear();
    }
    state.last_frame = frame;

    GraphicsConfig config = ultramodern::renderer::get_graphics_config();
    bool changed = false;
    const bool owned_camera_settings_deferred =
        dkr::runtime::netplay::experimental::owned_runtime_active();
    int profile = static_cast<int>(enh::presentation_profile());
    int window = static_cast<int>(config.wm_option);
    int resolution = static_cast<int>(config.res_option);
    int aspect = static_cast<int>(config.ar_option);
    int aa = static_cast<int>(config.msaa_option);
    int hpfb = static_cast<int>(config.hpfb_option);
    int downsample = std::clamp(config.ds_option, 1, 4);

    // Both menu surfaces use the same full-width responsive cards.
    const float width = std::max(available_width, 1.0F);
    // Container queries on the content panel.
    const bool stacked = available_width <= 850.0F;
    const bool single = available_width <= 540.0F;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0F, 0.0F});

    DrawPageHeading("GRAPHICS");
    PaddockGap(6.0F);
    PaddockText(PaddockReading(16.0F, false, 1.5F), PaddockRgb(kSetMuted),
                "Adjust the game display and visual detail.", width);
    PaddockGap(6.0F);
    PaddockText(SetHelpType(), PaddockRgb(kSetMuted),
                live ? "Most changes apply to the running game; settings that need a new launch say so."
                     : "Changes made before launch are applied when the adventure begins.",
                width);
    PaddockGap(18.0F);

    // ------------------------------------------------------------ game display
    bool modern = enh::modern_options_visible(enh::presentation_profile());
    SettingsCard(width, [&](float inner) {
        SettingsCardHead("Performance presets (optional)", "Nothing changes until you press Apply.", inner);
        static int preset = 1;
        SettingsDropdown("performance-preset", "Preset", &preset,
#if defined(__ANDROID__)
                         {"Battery", "Balanced", "Quality", "Low load - 30 FPS", "Low load - 60 FPS"},
#else
                         {"Battery", "Balanced", "Quality"},
#endif
                         inner);
        PaddockGap(8.0F);
        SettingsHelp("Battery: 480p / 30 FPS. Balanced: 480p / 60 FPS. Both use standard precision and 4x "
                     "filtering. Quality: window resolution / 60 FPS, high precision and 16x filtering. "
                     "All disable MSAA and supersampling; HUD and aspect stay unchanged. Targets are not guarantees.", inner);
#if defined(__ANDROID__)
        SettingsHelp("Low load presets use native 240p game rendering with standard precision. The launcher "
                     "and touch controls stay sharp. These are optional quality reductions, not simulation-speed changes.", inner);
#endif
        PaddockGap(12.0F);
        if (SettingsRaceButton("APPLY PERFORMANCE PRESET", false)) {
            if (!enh::modern_presentation_enabled()) {
                enh::set_presentation_profile(enh::PresentationProfile::Modern);
                ApplyProfileGraphics(config, enh::PresentationProfile::Modern);
            }
            dkr::runtime::mobile_graphics::apply_preset(config,
                static_cast<dkr::runtime::mobile_graphics::Preset>(preset));
            enh::set_anisotropy_level(preset == 2 ? 16 : 4);
            RememberModernGraphics(config);
            profile = static_cast<int>(enh::PresentationProfile::Modern);
            resolution = static_cast<int>(config.res_option);
            aspect = static_cast<int>(config.ar_option);
            aa = static_cast<int>(config.msaa_option);
            hpfb = static_cast<int>(config.hpfb_option);
            downsample = config.ds_option;
            modern = true;
            changed = true;
        }
        PaddockGap(8.0F);
        SettingsHelp("Filtering applies on the next game launch. Existing settings are preserved unless you apply a preset.", inner);
    });
    PaddockGap(18.0F);
    SettingsCard(width, [&](float inner) {
        SettingsCardHead("Game display",
                         modern ? "Presentation, window, resolution and frame rate."
                                : "Presentation and window.",
                         inner);
        SettingsColumns fields(inner, single);
        fields.Begin();
        if (SettingsDropdown("presentation", "Presentation style", &profile, {"Accurate", "Modern"},
                             fields.column, false,
                             "Accurate keeps the original look at 30 FPS. Modern unlocks the options on this page.")) {
            // The player is choosing the profile by hand now; retire any note
            // that Track Lab switched it for them.
            g_track_lab_modern_notice.clear();
            const auto old_profile = enh::presentation_profile();
            if (old_profile == enh::PresentationProfile::Modern) RememberModernGraphics(config);
            const auto next_profile = static_cast<enh::PresentationProfile>(profile);
            enh::set_presentation_profile(next_profile);
            ApplyProfileGraphics(config, next_profile);
            resolution = static_cast<int>(config.res_option);
            aspect = static_cast<int>(config.ar_option);
            aa = static_cast<int>(config.msaa_option);
            hpfb = static_cast<int>(config.hpfb_option);
            downsample = std::clamp(config.ds_option, 1, 4);
            changed = true;
        }
        if (!g_track_lab_modern_notice.empty()) {
            PaddockGap(8.0F);
            SettingsHelp(g_track_lab_modern_notice, fields.column, 0xFFCF83);
        }
        fields.End();
        modern = enh::modern_options_visible(enh::presentation_profile());

        fields.Begin();
        changed |= SettingsDropdown("window-mode", "Window mode", &window, {"Windowed", "Fullscreen"},
                                    fields.column, false, "Play in a window or use the full screen.");
        config.wm_option = static_cast<WindowMode>(window);
        fields.End();

        if (modern) {
            fields.Begin();
            changed |= SettingsDropdown("resolution", "Resolution", &resolution,
                                        {"Original (240p)", "Original 2x", "Automatic integer scale"},
                                        fields.column, false,
                                        "Higher resolutions draw more pixels and can increase graphics load.");
            fields.End();
            fields.Begin();
            changed |= SettingsDropdown("aspect", "Aspect ratio", &aspect, {"Original 4:3", "Fit to window"},
                                        fields.column, false,
                                        "Fit to window fills wide screens; Original keeps the 4:3 picture.");
            fields.End();
        }
        if (enh::modern_presentation_enabled()) {
            int refresh_mode = g_modern_refresh_mode == RefreshRate::Manual ? 1 : 0;
            fields.Begin();
            if (SettingsDropdown("presentation-rate", "Presentation rate", &refresh_mode,
                                 {"Match display", "Manual target"}, fields.column, false,
                                 "Match display follows your monitor's refresh rate.")) {
                g_modern_refresh_mode = refresh_mode == 0 ? RefreshRate::Display : RefreshRate::Manual;
                config.rr_option = g_modern_refresh_mode;
                changed = true;
            }
            fields.End();
            if (g_modern_refresh_mode == RefreshRate::Manual) {
                fields.Begin();
                if (SettingsRange("##refresh-target", "Frame-rate target", &g_modern_refresh_target, 30, 500,
                                  fields.column,
                                  [](int value) { return SettingsRangeValue{std::to_string(value), "FPS"}; },
                                  "Targets above the monitor refresh can reduce input-to-present latency, but "
                                  "cannot add visible refreshes and use more CPU/GPU power.")) {
                    g_modern_refresh_target = enh::clamp_presentation_rate(g_modern_refresh_target);
                    config.rr_manual_value = g_modern_refresh_target;
                    changed = true;
                }
                fields.End();
            }
            config.rr_option = g_modern_refresh_mode;
            config.rr_manual_value = g_modern_refresh_target;
        } else {
            config.rr_option = RefreshRate::Original;
            config.rr_manual_value = 30;
        }
        fields.Finish();

        if (!modern) {
            ApplyProfileGraphics(config, enh::PresentationProfile::Accurate);
            PaddockGap(16.0F);
            PaddockInlineNote("Original 4:3 at 30 FPS.",
                              "Fit to window, graphics tuning, the HUD layout and scenery options appear in "
                              "Modern. Accurate always uses the release-proven graphics API.",
                              inner);
            return;
        }
        config.res_option = static_cast<Resolution>(resolution);
        config.ar_option = static_cast<AspectRatio>(aspect);

        PaddockGap(18.0F);
        if (!SettingsDisclosure("##renderer", "Renderer & performance", state.renderer_open, inner)) {
            config.msaa_option = static_cast<Antialiasing>(aa);
            config.hpfb_option = static_cast<HighPrecisionFramebuffer>(hpfb);
            config.hr_option = HUDRatioMode::Original;
            config.ds_option = downsample;
            return;
        }
        PaddockGap(16.0F);
        SettingsColumns renderer(inner, single);
        renderer.Begin();
        changed |= SettingsDropdown("anti-aliasing", "Anti-aliasing", &aa,
                                    {"None", "MSAA 2x", "MSAA 4x", "MSAA 8x"}, renderer.column, false,
                                    "Smooths jagged edges. Higher settings cost more GPU time.");
        renderer.End();
        renderer.Begin();
        {
            constexpr std::array<int, 5> kAnisotropyLevels{1, 2, 4, 8, 16};
            int anisotropy_index = 0;
            for (std::size_t index = 0; index < kAnisotropyLevels.size(); ++index) {
                if (kAnisotropyLevels[index] == enh::anisotropy_level()) {
                    anisotropy_index = static_cast<int>(index);
                }
            }
            if (SettingsDropdown("anisotropic", "Anisotropic filtering", &anisotropy_index,
                                 {"1x (off)", "2x", "4x", "8x", "16x"}, renderer.column, false,
                                 "Improves angled track textures. Samplers are rebuilt on the next game launch.")) {
                enh::set_anisotropy_level(kAnisotropyLevels[static_cast<std::size_t>(anisotropy_index)]);
                changed = true;
            }
        }
        renderer.End();
        renderer.Begin();
        changed |= SettingsDropdown("high-precision", "High precision framebuffer", &hpfb,
                                    {"Automatic", "On", "Off"}, renderer.column, false,
                                    "More colour precision for effects and fades.");
        renderer.End();
        renderer.Begin();
        changed |= SettingsRange("##downsample", "Downsampling quality", &downsample, 1, 4, renderer.column,
                                 [](int value) {
                                     return value <= 1 ? SettingsRangeValue{"Off", ""}
                                                       : SettingsRangeValue{std::to_string(value), "x"};
                                 },
                                 "Renders extra pixels before the final image is reduced. Higher values are "
                                 "expensive; Off is recommended for high refresh rates.");
        renderer.End();
        renderer.Begin();
        {
            int bias = static_cast<int>(std::lround(enh::texture_lod_bias() * 20.0F));
            if (SettingsRange("##lod-bias", "Texture LOD bias", &bias, -40, 40, renderer.column,
                              [](int value) {
                                  char text[16]{};
                                  std::snprintf(text, sizeof(text), "%+.2f", static_cast<double>(value) / 20.0);
                                  return SettingsRangeValue{text, ""};
                              },
                              "Applies live. 0.00 is the unbiased default; negative values favour sharper mip "
                              "levels and positive values softer ones. Only textures with mipmaps change.")) {
                enh::set_texture_lod_bias_hundredths(bias * 5);
                changed = true;
            }
        }
        renderer.End();
        renderer.Begin();
        {
#if defined(_WIN32)
            int api_choice = config.api_option == GraphicsApi::D3D12 ? 1
                           : config.api_option == GraphicsApi::Vulkan ? 2 : 0;
            if (SettingsDropdown("graphics-api", "Graphics API", &api_choice,
                                 {"Automatic (recommended)", "Direct3D 12", "Vulkan"}, renderer.column, false,
                                 "Applied at the next game launch. Automatic remains the recovery choice.")) {
                config.api_option = api_choice == 1 ? GraphicsApi::D3D12
                                  : api_choice == 2 ? GraphicsApi::Vulkan : GraphicsApi::Auto;
                changed = true;
            }
#elif defined(__linux__)
            int api_choice = config.api_option == GraphicsApi::Vulkan ? 1 : 0;
            if (SettingsDropdown("graphics-api", "Graphics API", &api_choice,
                                 {"Automatic (recommended)", "Vulkan"}, renderer.column, false,
                                 "Applied at the next game launch. Automatic remains the recovery choice.")) {
                config.api_option = api_choice == 1 ? GraphicsApi::Vulkan : GraphicsApi::Auto;
                changed = true;
            }
#elif defined(__APPLE__)
            int api_choice = 0;
            SettingsDropdown("graphics-api", "Graphics API", &api_choice, {"Metal (automatic)"},
                             renderer.column, true);
#else
            int api_choice = 0;
            SettingsDropdown("graphics-api", "Graphics API", &api_choice, {"Automatic"}, renderer.column, true);
#endif
        }
        renderer.End();
        renderer.Finish();

        PaddockGap(16.0F);
        bool generate_mips = enh::generated_mipmaps_requested();
        if (SettingsCheck("##mipmaps", "Generate texture mipmaps", &generate_mips, inner,
                          "Default off. Generates mipmaps for eligible original and PNG pack textures on the "
                          "next game launch; LOD bias then adjusts them live. HUD, shadows, alpha cutouts and "
                          "unsafe subtiles keep their original sampling. Authored DDS mipmaps are preserved.")) {
            enh::set_generated_mipmaps_requested(generate_mips);
            changed = true;
        }
        if (live) {
            ImGui::Indent(36.0F);
            PaddockGap(4.0F);
            char active[160]{};
            std::snprintf(active, sizeof(active), "Active this game: %s.%s",
                          RT64::generatedMipSessionEnabled() ? "on" : "off",
                          generate_mips != RT64::generatedMipSessionEnabled()
                              ? " Change pending: launch a new game to apply." : "");
            SettingsHelp(active, inner - 36.0F, kSetText);
            const auto stats = RT64::generatedMipStatistics();
            char uploads[256]{};
            std::snprintf(uploads, sizeof(uploads),
                          "Texture uploads this game: %llu generated originals, %llu generated replacements, "
                          "%llu authored mip chains, %llu single-level.",
                          static_cast<unsigned long long>(stats.originals),
                          static_cast<unsigned long long>(stats.replacements),
                          static_cast<unsigned long long>(stats.authored),
                          static_cast<unsigned long long>(stats.singleLevel));
            SettingsHelp(uploads, inner - 36.0F);
            ImGui::Unindent(36.0F);
        }

        PaddockGap(16.0F);
        bool fps_overlay = g_fps_overlay_enabled;
        if (SettingsCheck("##fps-overlay", "Show performance overlay", &fps_overlay, inner,
                          "Display game performance statistics while playing.")) {
            g_fps_overlay_enabled = fps_overlay;
            SaveSettings();
        }
        if (g_fps_overlay_enabled) {
            PaddockGap(16.0F);
            SettingsColumns overlay(inner, single);
            overlay.Begin();
            if (SettingsDropdown("fps-position", "Overlay position", &g_fps_overlay_position,
                                 {"Top left", "Top right", "Bottom left", "Bottom right"}, overlay.column)) {
                SaveSettings();
            }
            overlay.End();
            overlay.Begin();
            if (SettingsDropdown("fps-detail", "Detail preset", &g_fps_overlay_detail,
                                 {"FPS only", "Standard", "Detailed", "Custom"}, overlay.column)) {
                SaveSettings();
            }
            overlay.End();
            overlay.Begin();
            int fps_layout = g_fps_overlay_single_row ? 1 : 0;
            if (SettingsDropdown("fps-layout", "Metric layout", &fps_layout, {"Stacked", "Single row"},
                                 overlay.column)) {
                g_fps_overlay_single_row = fps_layout == 1;
                SaveSettings();
            }
            overlay.End();
            overlay.Begin();
            if (SettingsRange("##fps-font", "Font size", &g_fps_font_size, 16, 64, overlay.column,
                              [](int value) { return SettingsRangeValue{std::to_string(value), "px"}; })) {
                SaveSettings();
            }
            overlay.End();
            overlay.Begin();
            if (GraphicsColourField("fps-fill", "Font fill colour", g_fps_fill_colour, overlay.column)) {
                SaveSettings();
            }
            overlay.End();
            overlay.Begin();
            if (GraphicsColourField("fps-outline", "Font outline colour", g_fps_outline_colour, overlay.column)) {
                SaveSettings();
            }
            overlay.End();
            overlay.Finish();
            if (g_fps_overlay_detail == 3) {
                PaddockGap(16.0F);
                SettingsColumns metrics(inner, single, 8.0F);
                const std::array<std::pair<const char*, bool*>, 8> kMetrics{{
                    {"Frame time", &g_fps_custom_frame_time},
                    {"Simulation rate", &g_fps_custom_simulation},
                    {"Graphics task rate", &g_fps_custom_graphics},
                    {"VI rate", &g_fps_custom_vi},
                    {"Interpolated frame rate", &g_fps_custom_interpolation},
                    {"Audio sample rate", &g_fps_custom_audio},
                    {"Presentation target", &g_fps_custom_target},
                    {"Viewport resolution", &g_fps_custom_resolution},
                }};
                bool custom_changed = false;
                for (const auto& [label, flag] : kMetrics) {
                    metrics.Begin();
                    ImGui::PushID(label);
                    custom_changed |= SettingsCheck("##metric", label, flag, metrics.column);
                    ImGui::PopID();
                    metrics.End();
                }
                metrics.Finish();
                if (custom_changed) SaveSettings();
            }
        }
        config.msaa_option = static_cast<Antialiasing>(aa);
        config.hpfb_option = static_cast<HighPrecisionFramebuffer>(hpfb);
        config.hr_option = HUDRatioMode::Original;
        config.ds_option = downsample;
    });

    if (changed) {
        if (modern) RememberModernGraphics(config);
        ultramodern::renderer::set_graphics_config(config);
        SaveSettings();
        state.can_undo = false;
        state.status.clear();
    }

    // ------------------------------------------------------------ hud, scenery
    if (modern) {
        PaddockGap(18.0F);
        SettingsCardPair("graphics", width, stacked, 18.0F, [&](float inner) {
            SettingsCardHead("HUD & effects", "Readability and the look of the game image.", inner);
            namespace hud = dkr::runtime::hud;
            int hud_layout = hud::basic_mode(hud::mode()) == hud::LayoutMode::FitToViewport ? 1 : 0;
            if (SettingsDropdown("hud-layout", "HUD layout", &hud_layout, {"4:3", "Fit to window"}, inner, false,
                                 "For one and two players. HUD size stays at 100%; three and four-player HUDs "
                                 "are unchanged.")) {
                state.hud_save_failed = !hud::apply_basic_mode(
                    hud_layout == 1 ? hud::LayoutMode::FitToViewport : hud::LayoutMode::Original);
            }
            if (state.hud_save_failed) {
                PaddockGap(6.0F);
                SettingsHelp("Could not save HUD settings. The previous layout is still active.", inner, 0xFFCF83);
            }
            PaddockGap(16.0F);
            int fov_offset = enh::fov_offset();
            ImGui::BeginDisabled(owned_camera_settings_deferred);
            if (SettingsRange("##fov", "Field of view", &fov_offset, enh::kMinimumFovOffset,
                              enh::kMaximumFovOffset, inner,
                              [](int value) {
                                  return SettingsRangeValue{value > 0 ? "+" + std::to_string(value)
                                                          : value < 0 ? "-" + std::to_string(-value) : "0",
                                                            "\xC2\xB0"};
                              },
                              "Adjusts each gameplay level from its authored camera. Menus, character select "
                              "and cutscenes keep their original framing.")) {
                enh::set_fov_offset(fov_offset);
                SaveSettings();
            }
            PaddockGap(16.0F);
            namespace cam = dkr::runtime::modern_camera;
            bool modern_camera = cam::requested();
            if (SettingsCheck("##modern-camera", "Modern camera", &modern_camera, inner,
                              "A chase camera that swings into corners, opens up with speed, shows the "
                              "side of your kart in a drift and reacts to boosts and landings. The right "
                              "stick (or the mouse with its right button held) turns it around your "
                              "racer and RT accelerates. During a race the "
                              "right stick then stops sending C-buttons (camera distance, course arrows, "
                              "minimap). Local play only.")) {
                cam::set_enabled(modern_camera);
                SaveSettings();
            }
            PaddockGap(16.0F);
            bool look_back = cam::look_back_requested();
            if (SettingsCheck("##look-back", "Look back", &look_back, inner,
                              "Hold Look back (right stick click by default, rebind under Controls) to "
                              "see behind your racer. Local play only.")) {
                cam::set_look_back_enabled(look_back);
                SaveSettings();
            }
            if (live) {
                PaddockGap(12.0F);
                if (SettingsButton("Tune the Modern camera (F7)")) {
                    OpenCameraTuning();
                    g_overlay_visible.store(false, std::memory_order_release);
                }
                PaddockGap(6.0F);
                SettingsHelp("A panel over the race that adjusts the camera while you drive, with "
                             "presets such as Comfort. Use the mouse; the controller keeps driving.", inner);
            }
            ImGui::EndDisabled();
            PaddockGap(16.0F);
            bool crt = g_crt_enabled;
            if (SettingsCheck("##crt", "Enable CRT overlay", &crt, inner,
                              "Apply a CRT filter to the game image. Launcher text and performance overlays "
                              "stay clear. Pick the filter under Textures.")) {
                g_crt_enabled = crt;
                SaveSettings();
            }
        }, [&](float inner) {
            SettingsCardHead("Vehicles & scenery", "More visible detail can increase CPU and GPU load.", inner);
#if defined(DKR_EXPERIMENTAL_RACE_TEST)
            bool diagnostics = dkr::runtime::netplay::experimental::performance::enabled();
            if (SettingsCheck("##rollback-diagnostics", "Experimental performance diagnostics", &diagnostics, inner,
                              "Records bounded timing summaries in runtime.log without changing speed or quality. "
                              "No GPU diagnostic drain. Enable before the match for a complete capture.")) {
                dkr::runtime::netplay::experimental::performance::set_enabled(diagnostics);
            }
            PaddockGap(16.0F);
#endif
            if (owned_camera_settings_deferred) {
                SettingsHelp("Experimental Rollback: keep-scenery, view-distance and retention apply locally to "
                             "restored retail scenery billboards, including removed tree tops; they do not yet "
                             "extend all actors or level geometry. Vehicle LOD, FOV, animated/effect ranges, water "
                             "detail and CPU culling remain canonical. Your preferences are kept for single-player "
                             "and legacy online modes.", inner);
                PaddockGap(16.0F);
            }
            const auto check = [&](const char* id, const char* label, bool value, auto setter,
                                   const char* help, bool first = false, bool disabled = false) {
                if (!first) PaddockGap(16.0F);
                ImGui::BeginDisabled(disabled);
                if (SettingsCheck(id, label, &value, inner, help)) {
                    setter(value);
                    SaveSettings();
                }
                ImGui::EndDisabled();
            };
            check("##max-detail", "Maximum vehicle detail", enh::maximum_detail_requested(),
                  enh::set_maximum_detail_enabled, "Keep racers on their most detailed models.", true,
                  owned_camera_settings_deferred);
            check("##hub-scenery", "Keep hub scenery rendered", enh::keep_hub_scenery_requested(),
                  enh::set_keep_hub_scenery_enabled, "Keep forward-visible scenery in the adventure hub.");
            check("##track-scenery", "Keep track and boss scenery rendered", enh::keep_track_scenery_requested(),
                  enh::set_keep_track_scenery_enabled,
                  "Keep forward-visible scenery during races and boss challenges.");
            check("##minigame-scenery", "Keep minigame and battle scenery rendered",
                  enh::keep_minigame_scenery_requested(), enh::set_keep_minigame_scenery_enabled,
                  "Keep forward-visible scenery in minigames and battle arenas.");
            check("##ultrawide", "Ultrawide scenery guard", enh::extended_culling_requested(),
                  enh::set_extended_culling_enabled,
                  "Expand visibility checks to reduce scenery disappearing near wide-screen edges.", false,
                  owned_camera_settings_deferred);
            if (enh::extended_culling_requested()) {
                PaddockGap(16.0F);
                ImGui::Indent(36.0F);
                int guard = enh::frustum_guard_percent();
                ImGui::BeginDisabled(owned_camera_settings_deferred);
                if (SettingsRange("##guard", "Culling safety margin", &guard, 0, 20, inner - 36.0F,
                                  [](int value) { return SettingsRangeValue{std::to_string(value), "%"}; },
                                  "Objects directly behind the camera still cull normally.")) {
                    enh::set_frustum_guard_percent(guard);
                    SaveSettings();
                }
                ImGui::EndDisabled();
                ImGui::Unindent(36.0F);
            }
            PaddockGap(18.0F);
            if (!SettingsDisclosure("##distances", "View distances", state.distances_open, inner)) return;
            PaddockGap(16.0F);
            const auto distance = [&](const char* id, const char* label, int value, int maximum, auto setter,
                                      bool disabled = false) {
                ImGui::BeginDisabled(disabled);
                if (SettingsRange(id, label, &value, enh::kMinimumViewDistanceMultiplier, maximum, inner,
                                  GraphicsTimes)) {
                    setter(value);
                    SaveSettings();
                }
                ImGui::EndDisabled();
                PaddockGap(16.0F);
            };
            distance("##view-distance", "Scenery and object view distance", enh::view_distance_multiplier(),
                     enh::kMaximumViewDistanceMultiplier, enh::set_view_distance_multiplier);
            int retention = static_cast<int>(enh::scenery_retention_mode());
            if (SettingsDropdown("retention", "Scenery retention", &retention,
                                 {"Authored", "Current region", "Visible + adjacent", "Full forward view"}, inner)) {
                enh::set_scenery_retention_mode(enh::normalise_scenery_retention_mode(retention));
                SaveSettings();
            }
            PaddockGap(16.0F);
            distance("##animated-distance", "Animated scenery distance",
                     enh::animated_scenery_distance_multiplier(), enh::kMaximumAnimatedSceneryMultiplier,
                     enh::set_animated_scenery_distance_multiplier, owned_camera_settings_deferred);
            distance("##billboard-distance", "Billboard and effect distance",
                     enh::billboard_effect_distance_multiplier(), enh::kMaximumBillboardEffectMultiplier,
                     enh::set_billboard_effect_distance_multiplier, owned_camera_settings_deferred);
            distance("##water-distance", "Water and lava detail distance", enh::water_lava_detail_multiplier(),
                     enh::kMaximumWaterLavaDetailMultiplier, enh::set_water_lava_detail_multiplier,
                     owned_camera_settings_deferred);
            SettingsHelp("Only forward-visible regions are retained; objects behind the camera still cull. "
                         "Higher settings can increase CPU and GPU load on handheld systems.",
                         inner);
        });
    }

#if defined(DKR_ENHANCED_WATER)
    // Shared by launcher and in-game overlay; these are local renderer
    // preferences, not synchronized water physics or guest settings.
    PaddockGap(18.0F);
    SettingsCard(width, [&](float inner) {
        namespace water = dkr::runtime::water::enhanced;
        SettingsCardHead("Ray tracing & water", "Optional water presentation. Original preserves the game's water.", inner);
        ImGui::BeginDisabled(!enh::modern_options_visible(enh::presentation_profile()));
        int quality=int(water::requested_quality.load());
        int preset=water::preset();
        if(SettingsDropdown("water-quality", "Water preset", &preset,
            {"Original", "Performance", "Balanced", "High", "Custom"}, inner)) {
            water::apply_preset(preset);quality=int(water::requested_quality.load());SaveSettings();
        }
        PaddockGap(16.0F);
        int reflections=int(water::requested_reflection.load());
        ImGui::BeginDisabled(quality==0);
        if(SettingsDropdown("water-reflections", "Reflections", &reflections,
            {"Sky", "Screen-space", "Hardware ray tracing (experimental)"}, inner)) {
            water::set_reflection(reflections); SaveSettings();
        }
        PaddockGap(16.0F);
        bool wakes=water::wakes_enabled.load();
        if(SettingsCheck("##water-wakes", "Racer wakes", &wakes, inner)) {
            water::wakes_enabled.store(wakes); SaveSettings();
        }
        PaddockGap(16.0F);
        bool spray=water::spray_enabled.load();
        if(SettingsCheck("##water-spray", "Water spray", &spray, inner)) {
            water::spray_enabled.store(spray); SaveSettings();
        }
        PaddockGap(16.0F);
        static bool lava_advanced=false;
        if(SettingsDisclosure("##lava-lighting", "RTX lava lighting", lava_advanced, inner)) {
            ImGui::BeginDisabled(reflections!=int(water::Reflection::Hardware));
            const auto lava_percent=[&](const char* id,const char* label,std::atomic<int>& setting,int low,int high,const char* help) {
                int value=setting.load();
                if(SettingsRange(id,label,&value,low,high,inner,
                    [](int v){return SettingsRangeValue{std::to_string(v),"%"};},help)) {
                    setting.store(value);SaveSettings();
                }
                PaddockGap(12.0F);
            };
            lava_percent("##lava-intensity","Lighting intensity",water::lava_lighting,0,300,
                "Zero disables the lighting pass. Higher values brighten nearby walls and ground without changing the lava surface brightness.");
            lava_percent("##lava-pulse","Pulse strength",water::lava_pulse_strength,0,100,
                "Zero gives a steady glow. Higher values deepen the smooth brightness pulse.");
            lava_percent("##lava-speed","Pulse speed",water::lava_pulse_speed,25,200,
                "Controls how quickly the underglow pulses. Animation stays local in online play.");
            lava_percent("##lava-range","Light range",water::lava_light_range,25,200,
                "Changes the reach of the glow. Occlusion is ray-tested against static level geometry; this is not a full multi-bounce renderer.");
            ImGui::EndDisabled();
            SettingsHelp("Requires enhanced water and Hardware ray tracing. Unsupported devices keep the emissive lava surface without this lighting pass. At most two shadow rays are traced per affected pixel.",inner);
        }
        PaddockGap(16.0F);
        static bool water_advanced=false;
        if(SettingsDisclosure("##water-advanced", "Advanced water settings", water_advanced, inner)) {
            const auto detail=[&](const char* id,const char* label,std::atomic<int>& setting) {
                int value=setting.load()+1;
                if(SettingsDropdown(id,label,&value,{"Preset default", "Low", "Medium", "High"},inner)) {
                    setting.store(value-1);SaveSettings();
                }
                PaddockGap(12.0F);
            };
            detail("water-reflection-detail","Reflection detail",water::reflection_detail);
            detail("water-wake-detail","Wake quality",water::wake_detail);
            detail("water-spray-detail","Spray detail",water::spray_detail);
            const auto percent=[&](const char* id,const char* label,std::atomic<int>& setting,int low,int high,const char* help) {
                int value=setting.load();
                if(SettingsRange(id,label,&value,low,high,inner,
                    [](int v){return SettingsRangeValue{std::to_string(v),"%"};},help)) {
                    setting.store(value);SaveSettings();
                }
                PaddockGap(12.0F);
            };
            percent("##water-ray-range","Hardware reflection distance",water::reflection_distance,25,200,
                "100% preserves the existing trace distance. Farther rays can cost more GPU time.");
            percent("##water-reflect-strength","Reflection strength",water::reflection_strength,0,200,
                "Changes the look. Zero skips reflection tracing; other values do not reduce game resolution.");
            percent("##water-roughness","Surface roughness",water::roughness_scale,25,200,
                "100% uses each track's authored water profile. Higher values soften highlights.");
            percent("##water-ripple-strength","Fine ripple strength",water::ripple_scale,0,200,
                "Visual ripples only. Waves, collisions and buoyancy are unchanged.");
            percent("##water-foam-strength","Foam intensity",water::foam_scale,0,200,
                "Adjusts surface and shoreline foam without changing the race simulation.");
            percent("##water-wake-strength","Wake visibility",water::wake_strength,0,200,
                "Adjusts the visible racer trails independently of wake simulation quality.");
            percent("##water-clarity","Water clarity",water::clarity_scale,25,200,
                "100% preserves each track's underwater visibility. Higher values show deeper receivers.");
            percent("##water-refraction","Refraction strength",water::refraction_scale,0,200,
                "Adjusts underwater image distortion. This does not affect physics.");
            percent("##water-waterfall","Waterfall flowing detail",water::waterfall_detail,0,200,
                "Zero keeps the original artwork. 100% uses continuous flowing water; higher values brighten the film. Authored transparency is retained.");
            bool track_colours=water::use_track_colours.load();
            if(SettingsCheck("##water-track-colours","Use track colours",&track_colours,inner)) {
                water::use_track_colours.store(track_colours);SaveSettings();
            }
            PaddockGap(12.0F);
            if(!track_colours) {
                percent("##water-red","Water tint: red",water::tint_red,0,200,"100% keeps the original red component.");
                percent("##water-green","Water tint: green",water::tint_green,0,200,"100% keeps the original green component.");
                percent("##water-blue","Water tint: blue",water::tint_blue,0,200,"100% keeps the original blue component.");
            }
            if(SettingsButton("Reset water appearance")) {water::reset_appearance();SaveSettings();}
            PaddockGap(8.0F);
            if(SettingsButton("Restore existing water quality defaults")) {water::reset_tuning();SaveSettings();}
            PaddockGap(12.0F);
            bool timings=water::timing_enabled.load();
            if(SettingsCheck("##water-timings","Log water GPU timings",&timings,inner,
                "Takes effect on the next game launch. Records bounded timing summaries without an extra GPU drain.")) {
                water::timing_enabled.store(timings);SaveSettings();
            }
            PaddockGap(12.0F);
            SettingsHelp("Water presets never change the game's resolution, MSAA, FPS target or reflection method. "
                "Quality changes are applied at a safe renderer workload boundary; appearance stays local in online play.",inner);
        }
        ImGui::EndDisabled();
        ImGui::EndDisabled();
        PaddockGap(16.0F);
        const int active=water::active_reflection.load();
        if(quality!=0 && active>=0 && active!=reflections)
            SettingsHelp(active==0 ? "Last rendered water used sky reflections." :
                active==1 ? "Last rendered water used screen-space reflections (hardware fallback)." :
                "Last rendered water used hardware reflections.",inner);
        if(water::scratch_fallbacks.load())
            SettingsHelp("This renderer encountered a bounded water allocation fallback. See runtime.log for dimensions and the reason.",inner);
        if(reflections==int(water::Reflection::Hardware)) {
            const int capability=water::hardware_ray_capability.load();
            SettingsHelp(capability<0 ? "GPU capability is checked when you start the game." :
                capability==0 ? "This renderer cannot use hardware ray queries. Screen-space reflections will be used." :
                "GPU ray queries are available. Hardware-scene failures retain screen-space reflections; allocation fallbacks are recorded in runtime.log.", inner);
            PaddockGap(8.0F);
        }
        SettingsHelp("Screen-space reflections use visible scene depth, not hardware ray tracing. "
            "Hardware ray tracing needs a compatible GPU and driver; otherwise screen-space reflections are used. "
            "The experimental hardware path traces opaque level geometry; moving racers use screen-space reflections. "
            "Off-screen surfaces currently use their vertex colours, not their texture-pack detail. "
            "Waterfalls and fountains use a continuous flowing-water material while retaining authored transparency and fog. "
            "Hot Top Volcano's lava uses an emissive molten material with the same quality and reflection settings, without water wakes or refraction. "
            "These settings do not change collisions, buoyancy or online simulation.", inner);
    });
#endif

    // ------------------------------------------------------------ footer
    PaddockGap(18.0F);
    {
        const ImVec2 at = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled(at, {at.x + width, at.y + 1.0F}, PaddockCol(kSetBorder));
        PaddockGap(1.0F + 18.0F);
        PaddockText(PaddockSign(19.0F, 1.0F, 0.0F), PaddockRgb(kSetText), "Restore graphics defaults", width);
        PaddockGap(6.0F);
        SettingsHelp("Switch back to Accurate, the original presentation. Your Modern settings are kept, and "
                     "you can undo until your next graphics change or until you leave this page.",
                     width);
        PaddockGap(12.0F);
        const bool accurate = !enh::modern_options_visible(enh::presentation_profile());
        if (SettingsRaceButton("RESTORE GRAPHICS DEFAULTS", accurate,
                               "Graphics settings already match the defaults.")) {
            RememberModernGraphics(config);
            enh::set_presentation_profile(enh::PresentationProfile::Accurate);
            ApplyProfileGraphics(config, enh::PresentationProfile::Accurate);
            ultramodern::renderer::set_graphics_config(config);
            SaveSettings();
            state.can_undo = true;
            state.status = "Accurate restored. Your Modern settings are kept.";
        }
        if (state.can_undo) {
            ImGui::SameLine(0.0F, 12.0F);
            if (SettingsRaceButton("UNDO", false)) {
                enh::set_presentation_profile(enh::PresentationProfile::Modern);
                ApplyProfileGraphics(config, enh::PresentationProfile::Modern);
                ultramodern::renderer::set_graphics_config(config);
                SaveSettings();
                state.can_undo = false;
                state.status = "Your previous graphics settings have been restored.";
            }
        }
        if (!state.status.empty()) {
            PaddockGap(12.0F);
            SettingsHelp(state.status, width, kSetWarm);
        }
    }
    ImGui::PopStyleVar();
}
