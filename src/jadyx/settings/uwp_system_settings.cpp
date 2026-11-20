// SPDX-FileCopyrightText: Copyright 2026 Jadyx Project
// SPDX-License-Identifier: GPL-3.0-or-later
#include "jadyx/settings/uwp_system_settings.h"

#include <sstream>
#include <filesystem>
#include <fmt/format.h>
#include "jadyx/settings/title_profile.h"
#include "jadyx/updates/update_storage.h"
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.System.UserProfile.h>
#include "common/logging.h"
#include "common/settings.h"
#include "video_core/performance_overlay.h"
#include "core/hle/service/acc/profile_manager.h"

namespace Jadyx {
namespace {
constexpr auto UiLanguageKey = L"system_ui_language";
constexpr auto GameLanguageKey = L"system_game_language";
constexpr auto RegionKey = L"system_region";
constexpr auto UserNameKey = L"system_user_name";
constexpr auto PerformanceStatsKey = L"system_show_performance_stats";
constexpr auto AdultModsKey = L"system_show_adult_mods";
constexpr auto PulseSessionsKey = L"system_pulse_sessions";
constexpr auto PlayTimeKey = L"system_track_play_time";
constexpr auto UpscalingKey = L"system_upscaling";
constexpr auto ShaderShareKey = L"system_shader_share_url";

/// Saved as the enum's value, or -1 for "follow the system". A value that is not a selectable
/// option (an older or newer build's) also falls back to the system.
template <typename T>
std::optional<T> LoadChoice(const winrt::Windows::Foundation::Collections::IPropertySet& values,
                            const wchar_t* key, std::span<const T> options) {
    if (!values.HasKey(key)) return std::nullopt;
    const auto raw = winrt::unbox_value<int32_t>(values.Lookup(key));
    for (const T option : options)
        if (static_cast<int32_t>(option) == raw) return option;
    return std::nullopt;
}
template <typename T>
int32_t ChoiceValue(std::optional<T> choice) {
    return choice ? static_cast<int32_t>(*choice) : -1;
}
} // namespace

SystemSettings LoadSystemSettings() {
    SystemSettings settings;
    try {
        const auto values = winrt::Windows::Storage::ApplicationData::Current().LocalSettings().Values();
        if (values.HasKey(L"system_development_picker"))
            settings.development_picker = winrt::unbox_value<bool>(values.Lookup(L"system_development_picker"));
        if (values.HasKey(UiLanguageKey)) {
            settings.ui_language = winrt::unbox_value<winrt::hstring>(values.Lookup(UiLanguageKey)).c_str();
            if (!I18n::FindUiLanguage(settings.ui_language)) settings.ui_language.clear();
        }
        if (values.HasKey(L"optimization_profile_v1")) {
            std::istringstream profile{winrt::to_string(winrt::unbox_value<winrt::hstring>(values.Lookup(L"optimization_profile_v1")))};
            for (std::string line; std::getline(profile, line);) settings.optimization.Apply(line);
        }
        settings.game_language = LoadChoice(values, GameLanguageKey, SelectableGameLanguages());
        settings.region = LoadChoice(values, RegionKey, SelectableRegions());
        if (values.HasKey(UserNameKey))
            settings.user_name = SanitizeUserName(
                winrt::unbox_value<winrt::hstring>(values.Lookup(UserNameKey)).c_str());
        if (values.HasKey(PerformanceStatsKey)) settings.show_performance_stats = winrt::unbox_value<bool>(values.Lookup(PerformanceStatsKey));
        if (values.HasKey(AdultModsKey)) settings.show_adult_mods = winrt::unbox_value<bool>(values.Lookup(AdultModsKey));
        if (values.HasKey(PulseSessionsKey)) settings.pulse_sessions = winrt::unbox_value<bool>(values.Lookup(PulseSessionsKey));
        if (values.HasKey(PlayTimeKey)) settings.track_play_time = winrt::unbox_value<bool>(values.Lookup(PlayTimeKey));
        if (values.HasKey(UpscalingKey))
            settings.upscaling = UpscalingFromValue(winrt::unbox_value<int32_t>(values.Lookup(UpscalingKey)))
                                     .value_or(Upscaling::Bilinear);
        // Preserve older in-game upscaling preferences on the first upgraded launch.
        const auto legacy = ToSettings(settings.upscaling);
        settings.graphics.fsr = legacy.filter == Settings::ScalingFilter::Fsr;
        settings.graphics.sharpness = legacy.fsr_sharpening_slider;
        if (values.HasKey(L"graphics_profile_v1")) {
            std::istringstream profile{winrt::to_string(winrt::unbox_value<winrt::hstring>(values.Lookup(L"graphics_profile_v1")))};
            for (std::string line; std::getline(profile, line);) settings.graphics.Apply(line);
        }
        settings.upscaling = FromSettings(settings.graphics.fsr ? Settings::ScalingFilter::Fsr : Settings::ScalingFilter::Bilinear,
                                          settings.graphics.sharpness);
        if (values.HasKey(ShaderShareKey))
            settings.shader_share_url = winrt::unbox_value<winrt::hstring>(values.Lookup(ShaderShareKey)).c_str();
    } catch (...) { /* Missing or invalid values keep following the system. */ }
    return settings;
}

bool SaveSystemSettings(const SystemSettings& settings) {
    try {
        const auto values = winrt::Windows::Storage::ApplicationData::Current().LocalSettings().Values();
        values.Insert(L"optimization_profile_v1", winrt::box_value(winrt::to_hstring(settings.optimization.Serialize())));
        values.Insert(L"graphics_profile_v1", winrt::box_value(winrt::to_hstring(settings.graphics.Serialize())));
        values.Insert(L"system_development_picker", winrt::box_value(settings.development_picker));
        values.Insert(UiLanguageKey, winrt::box_value(winrt::hstring{settings.ui_language}));
        values.Insert(GameLanguageKey, winrt::box_value(ChoiceValue(settings.game_language)));
        values.Insert(RegionKey, winrt::box_value(ChoiceValue(settings.region)));
        values.Insert(UserNameKey, winrt::box_value(winrt::hstring{SanitizeUserName(settings.user_name)}));
        values.Insert(PerformanceStatsKey, winrt::box_value(settings.show_performance_stats));
        values.Insert(AdultModsKey, winrt::box_value(settings.show_adult_mods));
        values.Insert(PulseSessionsKey, winrt::box_value(settings.pulse_sessions));
        values.Insert(PlayTimeKey, winrt::box_value(settings.track_play_time));
        values.Insert(UpscalingKey, winrt::box_value(static_cast<int32_t>(settings.upscaling)));
        values.Insert(ShaderShareKey, winrt::box_value(winrt::hstring{settings.shader_share_url}));
        return true;
    } catch (...) { return false; }
}

SystemLocale DetectSystemLocale() {
    SystemLocale locale;
    using winrt::Windows::System::UserProfile::GlobalizationPreferences;
    try {
        for (const auto& language : GlobalizationPreferences::Languages())
            locale.languages.emplace_back(language.c_str());
    } catch (const winrt::hresult_error&) {}
    try {
        locale.region = GlobalizationPreferences::HomeGeographicRegion().c_str();
    } catch (const winrt::hresult_error&) {}
    return locale;
}

ResolvedSystem ApplySystemSettings() {
    const auto locale = DetectSystemLocale();
    const auto settings = LoadSystemSettings();
    VideoCore::performance_overlay_enabled.store(settings.show_performance_stats, std::memory_order_relaxed);
    ApplyGraphicsSettings(settings.graphics);
    const auto resolved = ResolveSystem(settings, locale);
    I18n::SetUiLanguage(*resolved.ui_language);
    Settings::values.language_index = resolved.game_language;
    Settings::values.region_index = resolved.region;
    // The console's nickname, which some games show; Eden's default is its own name.
    Settings::values.device_name.SetValue("Jadyx");
    LOG_INFO(Frontend, "System: Windows language {}, region {}; interface {}, game language {}, region {}",
             locale.languages.empty() ? std::string{"(none)"} : I18n::ToUtf8(locale.languages.front()),
             locale.region.empty() ? std::string{"(none)"} : I18n::ToUtf8(locale.region),
             I18n::ToUtf8(resolved.ui_language->tag), Settings::CanonicalizeEnum(resolved.game_language),
             Settings::CanonicalizeEnum(resolved.region));
    return resolved;
}

namespace {
std::filesystem::path TitleProfilePath(std::uint64_t title) {
    return std::filesystem::path{std::wstring{winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path()}} /
        "optimization" / (fmt::format("{:016x}", title) + ".cfg");
}
}
bool LoadTitleSettings(std::uint64_t title, SystemSettings& settings) {
    try {
        const auto path = TitleProfilePath(title);
        if (!std::filesystem::exists(path)) return true;
        const auto text = Updates::Storage::ReadBounded(path, 64 * 1024);
        if (!text) return false;
        ApplyTitleProfile(*text, settings.optimization, settings.graphics);
        return true;
    } catch (...) { return false; }
}
bool SaveTitleSetting(std::uint64_t title, std::string_view assignment) {
    if (!title) return false;
    try {
        const auto path = TitleProfilePath(title);
        std::filesystem::create_directories(path.parent_path());
        std::string existing;
        if (std::filesystem::exists(path)) {
            const auto text = Updates::Storage::ReadBounded(path, 64 * 1024);
            if (!text) return false;
            existing = *text;
        }
        const auto updated = EditTitleProfile(existing, assignment);
        if (updated.size() > 64 * 1024) return false;
        return Updates::Storage::Persist(path, updated);
    } catch (...) { return false; }
}
void ApplyGraphicsSettings(const GraphicsSettings& settings) {
    Settings::values.use_docked_mode.SetValue(settings.docked ? Settings::ConsoleMode::Docked : Settings::ConsoleMode::Handheld);
    Settings::values.aspect_ratio.SetValue(static_cast<Settings::AspectRatio>(settings.aspect_ratio));
    Settings::values.scaling_filter.SetValue(settings.fsr ? Settings::ScalingFilter::Fsr : Settings::ScalingFilter::Bilinear);
    Settings::values.fsr_sharpening_slider.SetValue(static_cast<int>(settings.sharpness));
}

void ApplyUpscaling(Upscaling upscaling) {
    const UpscalingSettings values = ToSettings(upscaling);
    Settings::values.scaling_filter.SetValue(values.filter);
    Settings::values.fsr_sharpening_slider.SetValue(values.fsr_sharpening_slider);
}

bool ApplyUserName(Service::Account::ProfileManager& profiles, std::wstring_view name) {
    // No saved name is the default one: a profile renamed before goes back to it.
    auto clean = SanitizeUserName(name);
    if (clean.empty()) clean = DefaultUserName;
    const auto index = static_cast<std::size_t>(std::clamp<s32>(
        Settings::values.current_user.GetValue(), 0, static_cast<s32>(Service::Account::MAX_USERS) - 1));
    const auto uuid = profiles.GetUser(index);
    Service::Account::ProfileBase base{};
    if (!uuid || !profiles.GetProfileBase(*uuid, base)) return false;
    // Ten UTF-16 units are at most 30 UTF-8 bytes, inside the 32-byte field with room to spare.
    const auto utf8 = I18n::ToUtf8(clean);
    Service::Account::ProfileUsername username{};
    std::copy_n(utf8.begin(), std::min(utf8.size(), username.size()), username.begin());
    if (base.username == username) return true;
    base.username = username;
    const bool saved = profiles.SetProfileBase(*uuid, base);
    LOG_INFO(Frontend, "System: profile {} renamed to '{}'{}", index, utf8, saved ? "" : " (failed)");
    return saved;
}
} // namespace Jadyx
