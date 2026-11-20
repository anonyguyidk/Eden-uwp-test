// SPDX-FileCopyrightText: Copyright 2026 Eden-UWP experimental fork contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cassert>
#include <cstdio>
#include "jadyx/settings/system_page.h"
#include "jadyx/settings/title_profile.h"
#include "jadyx/storage/development_folder_browser.h"
#include "jadyx/library/game_options_page.h"

int main() {
    using namespace Jadyx;
    GraphicsSettings graphics;
    const auto defaults = graphics;
    for (const auto line : {"graphics_rcas=-1", "graphics_rcas=201", "graphics_rcas=4294967296",
         "graphics_rcas=", "graphics_rcas=10junk", "graphics_docked=2", "graphics_fsr1=true",
         "graphics_aspect=5", "graphics_bad=0", "graphics_rcas"}) {
        assert(!graphics.Apply(line));
        assert(graphics == defaults);
    }
    OptimizationSettings memory;
    ApplyTitleProfile("graphics_docked=0\r\ngraphics_fsr1=1\ngraphics_rcas=75\ngraphics_aspect=2\n"
                      "jit_cache_mib=32\nunknown=1\ngraphics_rcas=300\n", memory, graphics);
    assert(!graphics.docked && graphics.fsr && graphics.sharpness == 75 && graphics.aspect_ratio == 2);
    assert(memory.jit_cache_mib == 32 && memory.upload_ring_mib == 64);
    GraphicsSettings restored;
    OptimizationSettings restored_memory;
    ApplyTitleProfile(graphics.Serialize() + memory.Serialize(), restored_memory, restored);
    assert(restored == graphics && restored_memory == memory);

    const std::string profile = "# keep me\r\nunknown=ok\ngraphics_fsr1=0\ngraphics_fsr1=1\n"
                                "upload_ring_mib=128\nastc_mip0=drop\n";
    const auto edited = EditTitleProfile(profile, "graphics_fsr1=0");
    assert(edited == "# keep me\nunknown=ok\nupload_ring_mib=128\nastc_mip0=drop\ngraphics_fsr1=0\n");
    const auto mip = EditTitleProfile(edited, "astc_mip_drop=0");
    assert(mip.find("astc_mip0") == std::string::npos);
    assert(EditTitleProfile(mip + "graphics_rcas=invalid\n", {}) == "# keep me\nunknown=ok\n");
    // Unspecified keys inherit later changes to global defaults.
    GraphicsSettings inherited;
    inherited.sharpness = 160;
    ApplyTitleProfile(edited, memory, inherited);
    assert(inherited.sharpness == 160 && !inherited.fsr);

    SystemPage page;
    page.open = page.graphics = true;
    assert(SelectedSystemRow(page) == SystemPage::GraphicsScope);
    assert(HandleSystemInput(page, SystemInput::Confirm) == SystemPageEffect::None);
    page.graphics_title = 1;
    assert(HandleSystemInput(page, SystemInput::Right) == SystemPageEffect::GraphicsScope && page.per_game);
    HandleSystemInput(page, SystemInput::Down);
    assert(SelectedSystemRow(page) == SystemPage::ConsoleMode);
    HandleSystemInput(page, SystemInput::Confirm);
    assert(SystemSettingAssignment(page) == "graphics_docked=0");
    HandleSystemInput(page, SystemInput::Down);
    HandleSystemInput(page, SystemInput::Confirm);
    assert(SystemSettingAssignment(page) == "graphics_fsr1=1");
    HandleSystemInput(page, SystemInput::Down);
    HandleSystemInput(page, SystemInput::Right);
    assert(page.settings.graphics.sharpness == 20);
    for (unsigned i = 0; i < 100; ++i) HandleSystemInput(page, SystemInput::Right);
    assert(page.settings.graphics.sharpness == 0);
    for (unsigned i = 0; i < 100; ++i) HandleSystemInput(page, SystemInput::Left);
    assert(page.settings.graphics.sharpness == 200);
    assert(SystemSettingAssignment(page) == "graphics_rcas=200");
    page.selected = 0;
    HandleSystemInput(page, SystemInput::Up);
    assert(SelectedSystemRow(page) == SystemPage::ResetGraphics);
    assert(HandleSystemInput(page, SystemInput::Confirm) == SystemPageEffect::ResetGraphics);
    HandleSystemInput(page, SystemInput::Down);
    assert(page.selected == 0);
    for (size_t i = 0; i < GraphicsRows().size(); ++i) {
        page.selected = i;
        const auto row = SelectedSystemRow(page);
        const auto assignment = SystemSettingAssignment(page);
        if (row == SystemPage::GraphicsScope || row == SystemPage::InternalResolution || row == SystemPage::ResetGraphics)
            assert(assignment.empty());
        else {
            auto opt = OptimizationSettings{};
            auto gfx = GraphicsSettings{};
            assert(!assignment.empty() && (opt.Apply(assignment) || gfx.Apply(assignment)));
        }
    }
    assert(GameOptions().back() == GameOption::Graphics);
    assert(GameOptionHasManager(GameOption::Graphics));

    DevelopmentFolderBrowser browser;
    assert(std::wstring_view{browser.Root} == L"D:\\DevelopmentFiles");
    browser.Move(true);
    assert(browser.selected == DevelopmentFolderBrowser::Authorize);
    browser.Move(false);
    assert(browser.selected == 0);
    browser.folders = {L"Games", L"Firmware", L"Keys"};
    browser.Move(true);
    assert(browser.selected == 6);
    for (size_t i = 0; i < 14; ++i) browser.Move(false);
    assert(browser.selected == 6);
    std::puts("Graphics profiles, bounded settings, inheritance, navigation and folder rows: passed");
}
