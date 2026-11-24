/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* The plugin's panel as a picture, drawn by its own editor without a host:  s23_panel <out.png> [scale] */
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

int main(int argc, char **argv)
{
    ScopedJuceInitialiser_GUI gui;
    if (argc < 2) {
        std::fprintf(stderr, "usage: s23_panel <out.png> [scale]\n");
        return 2;
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
