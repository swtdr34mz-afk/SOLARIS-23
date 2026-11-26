/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* The plugin's panel as a picture, drawn by its own editor without a host:  s23_panel <out.png> [scale]
 * s23_panel --rightclick: every control right-clicked and right-dragged in a (pretend) host: no value
 * may change, and in the host each control's click opens the host's menu for its parameter */
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Controls.h"
#include <cstring>

using namespace juce;

namespace
{
int shown = 0;
const AudioProcessorParameter *lastParam = nullptr;
struct Menu : HostProvidedContextMenu {
    PopupMenu getEquivalentPopupMenu() const override { return {}; }
    void showNativeMenu(Point<int>) const override { shown++; }
};
struct Host : AudioProcessorEditorHostContext {
    std::unique_ptr<HostProvidedContextMenu> getContextMenuForParameter(const AudioProcessorParameter *p) const override
    {
        lastParam = p;
        return std::make_unique<Menu>();
    }
};

bool clicks(Component &c)
{
    bool self = false, kids = false;
    c.getInterceptsMouseClicks(self, kids);
    return self;
}

int rightClicks(SolarisProcessor &proc, AudioProcessorEditor &ed, bool host)
{
    Host h;
    ed.setHostContext(host ? &h : nullptr);
    auto &params = proc.getParameters();
    std::vector<float> before;
    for (auto *p : params)
        before.push_back(p->getValue());
    int controls = 0, failures = 0;
    std::function<void(Component &)> walk = [&](Component &c) {
        for (auto *k : c.getChildren())
            walk(*k);
        if (&c == &ed || c.getWidth() < 4 || c.getHeight() < 4 || !clicks(c) || c.getNumChildComponents() > 0)
            return;
        controls++;
        auto src = Desktop::getInstance().getMainMouseSource();
        auto mods = ModifierKeys(ModifierKeys::rightButtonModifier);
        for (auto at : {Point<float>(c.getWidth() * 0.3f, c.getHeight() * 0.5f), Point<float>(c.getWidth() * 0.7f, c.getHeight() * 0.5f)}) {
            auto now = Time::getCurrentTime();
            MouseEvent down(src, at, mods, 1.0f, 0, 0, 0, 0, &c, &c, now, at, now, 1, false);
            MouseEvent drag(src, at + Point<float>(0.0f, -60.0f), mods, 1.0f, 0, 0, 0, 0, &c, &c, now, at, now, 1, true);
            int s0 = shown;
            c.mouseDown(down);
            c.mouseDrag(drag);
            c.mouseUp(drag);
            if (host && shown == s0 && (dynamic_cast<s23::Knob *>(&c) || dynamic_cast<s23::Switch *>(&c))) {
                std::printf("no host menu: %s\n", c.getName().toRawUTF8());
                failures++;
            }
        }
    };
    walk(ed);
    for (size_t i = 0; i < before.size(); i++)
        if (params[(int)i]->getValue() != before[i]) {
            std::printf("changed by a right click: %s\n", params[(int)i]->getName(40).toRawUTF8());
            failures++;
        }
    std::printf("%s: %d controls right-clicked, %d host menus, %d failures\n", host ? "in a host" : "standalone", controls, shown, failures);
    ed.setHostContext(nullptr);
    return failures;
}
} // namespace

int main(int argc, char **argv)
{
    ScopedJuceInitialiser_GUI gui;
    if (argc < 2) {
        std::fprintf(stderr, "usage: s23_panel <out.png> [scale]\n");
        return 2;
    }
    if (!std::strcmp(argv[1], "--rightclick")) {
        SolarisProcessor proc;
        std::unique_ptr<AudioProcessorEditor> ed(proc.createEditor());
        ed->setSize(SolarisEditor::W, SolarisEditor::H);
        int f = rightClicks(proc, *ed, true);   /* (the standalone's own choice list needs a screen) */
        return f == 0 ? 0 : 1;
    }
    float scale = argc > 2 ? (float)std::atof(argv[2]) : 1.0f;
    SolarisProcessor proc;
    std::unique_ptr<AudioProcessorEditor> ed(proc.createEditor());
    ed->setSize(SolarisEditor::W, SolarisEditor::H);
    Image img = ed->createComponentSnapshot(ed->getLocalBounds(), true, scale);
    File f = File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    f.deleteFile();
    FileOutputStream out(f);
    return out.openedOk() && PNGImageFormat().writeImageToStream(img, out) ? 0 : 1;
}
