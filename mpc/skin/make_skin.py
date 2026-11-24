#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 swtdr34mz (SOLARIS-23)
"""The MPC skin (Plugin Skins/: TUI.json, Q-Links.json and the pictures) from the renderer's output.

    python3 mpc/skin/make_skin.py <render folder> mpc/plugin/skins [--preview <folder>]

The renderer (render_skin.cpp) draws each MPC page with the VST3's own sections and cuts every control out at every
value; this places each picture where it was cut from, bound to the MPC plugin's parameter:

  knobs            film strips (a frame a value, the value's display in it), dragged; a double tap opens the MPC's
                   editor (its list for a choice); the cartridge slots are knobs over the 24 programs
  keys             a picture on and off, a tap switches
  labels           X, Y, Z's meaning on each side: a film strip that follows that side's program

The pages (the function keys): DRONES (DRONE 1/2, again: DRONE 4/5), CHAOS, VCO, MOD (LFO/JOY, again: SEQ/KEYS),
MIX/FX. On each the 16 Q-Links turn four groups of four, the top row of Q-Links the first group and so on down.
--preview also writes each page as the MPC shows it at the defaults (to look at). Standard library (+ Pillow to
shrink the pictures to 256 colours, and for --preview).
"""
import json
import os
import shutil
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def param_ids():
    ids = []
    with open(os.path.join(ROOT, "dsp", "params_list.inc")) as f:
        for line in f:
            if line.startswith("P("):
                ids.append(line.split('"')[1])
    return ids


IDS = param_ids()
MPC = {pid: i for i, pid in enumerate(IDS)}

# the Q-Links of each page, top row of knobs first (a group in brackets with False: not framed, its controls apart)
PAGES = {
    "DRONE 1/2": [["d1_t1", "d1_t2", "d1_t3", "d1_t4"], (["d1_t5", "d1_volt", "d1_att", "d1_rls"], False),
                  ["d2_t1", "d2_t2", "d2_t3", "d2_t4"], (["d2_t5", "d2_volt", "d2_att", "d2_rls"], False)],
    "DRONE 4/5": [["d4_t1", "d4_t2", "d4_t3", "d4_t4"], (["d4_t5", "d4_volt", "d4_att", "d4_rls"], False),
                  ["d5_t1", "d5_t2", "d5_t3", "d5_t4"], (["d5_t5", "d5_volt", "d5_att", "d5_rls"], False)],
    "CHAOS": [(["d3_rate", "d3_moda", "d3_pitch", "d3_div"], False), (["d3_noise", "d3_sh", "d3_att", "d3_rls"], False),
              (["d6_rate", "d6_moda", "d6_pitch", "d6_div"], False), (["d6_noise", "d6_sh", "d6_att", "d6_rls"], False)],
    "VCO": [(["va_wave", "va_pw", "va_tune", "va_oct"], False), ["va_a", "va_d", "va_s", "va_r"],
            (["vb_wave", "vb_pw", "vb_tune", "vb_oct"], False), ["vb_a", "vb_d", "vb_s", "vb_r"]],
    "LFO/JOY": [(["la_wave", "la_rate", "la_range", "la_depth"], False), (["lb_wave", "lb_rate", "lb_range", "lb_depth"], False),
                (["joy_x", "joy_y", "joy_xdest", "joy_ydest"], False), (["light_src", "light_rate", "light_depth", "glide"], False)],
    "SEQ/KEYS": [(["seq_s1", "seq_s2", "seq_s3", "seq_s4"], False), (["seq_s5", "seq_rate", "seq_stages", "seq_clock"], False),
                 (["seq_dest", "seq_depth", "la_dest", "lb_dest"], False), (["grid", "bend", "glide", "volume"], False)],
    "MIX/FX": [(["d1_vol", "d2_vol", "d3_vol", "va_vol"], False), (["vb_vol", "d4_vol", "d5_vol", "d6_vol"], False),
               (["fl_freq", "fl_res", "fr_freq", "fr_res"], False), (["fx_x", "fx_y", "fx_z", "blend"], False)],
}
# Program (Track) mode: the knobs as the MPC's own program knobs, bottom left Q-Link 1
PROGRAM_QLINKS = ["volume", "blend", "dist", "glide",
                  "fl_freq", "fl_res", "fr_freq", "fr_res",
                  "fx_x", "fx_y", "fx_z", "joy_y",
                  "joy_x", "la_rate", "lb_rate", "seq_rate"]


class O(list):
    """A JSON object kept in order, with repeated keys (the overlays' two "version"s)."""
    def __init__(self, *pairs):
        super().__init__(pairs)


def dump(v, ind=0):
    sp = "  " * ind
    if isinstance(v, O):
        if not v:
            return "{}"
        return "{\n" + ",\n".join("%s  %s: %s" % (sp, json.dumps(k), dump(x, ind + 1)) for k, x in v) + "\n" + sp + "}"
    if isinstance(v, list):
        if not v:
            return "[]"
        if all(not isinstance(x, (O, list)) for x in v):
            return "[" + ", ".join(json.dumps(x) for x in v) + "]"
        return "[\n" + ",\n".join(sp + "  " + dump(x, ind + 1) for x in v) + "\n" + sp + "]"
    return json.dumps(v)


def box(r):
    return "%d %d %d %d" % tuple(r)


def image_data(img):
    return O(("version", 2), ("imageType", "Regular"), ("colour", "0"), ("image", img))


def bounds(r, focus="No", hide_handles=None, show="Show"):
    if hide_handles:
        return O(("version", 2), ("acceptsHWFocus", focus), ("showWhenDataModelInvalid", show), ("whenVisible", "Always"),
                 ("boundsType", "Absolute"), ("bounds", box(r)), ("additionalInvalidatingHandles", hide_handles))
    return O(("version", 1), ("acceptsHWFocus", focus), ("showWhenDataModelInvalid", show), ("whenVisible", "Always"),
             ("boundsType", "Absolute"), ("bounds", box(r)))


def entry(name, typ, data, b, remap=None):
    return O(("version", 2),
             ("componentData", O(("version", 1), ("name", name), ("type", typ), ("data", data))),
             ("handle remapping", O(("version", 1), ("map", [O(("key", k), ("value", v)) for k, v in (remap or [])]))),
             ("bounds", b))


def action(on, handler, handle, extra):
    return O(("version", 1), ("onAction", on), ("handler", handler), ("handleName", handle), ("additionalData", extra))


def component(actions, children, ignore_mouse=False):
    clear = O(("version", 1), ("colour", "0"), ("image", ""))
    return O(("version", 2), ("actions", actions),
             ("backgroundData", O(("version", 1), ("focussed", clear), ("unfocussed", clear))),
             ("ignoreMousePresses", ignore_mouse), ("disableCoarseDataWheel", False), ("componentsData", children))


FOCUS_LOST = O(("version", 1), ("onAction", "Focus Lost"), ("handler", "Focus Lost"), ("handleName", "Focus Lost"),
               ("additionalData", ""))


def overlay(typ, data):
    return component([], [entry("overlay", typ, data, bounds((0, 0, 1280, 800), "Yes"))], ignore_mouse=True)


def handle(pid):
    return "Parameter %d" % MPC[pid]


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    src, out = argv[1], argv[2]
    preview = argv[argv.index("--preview") + 1] if "--preview" in argv else None
    m = json.load(open(os.path.join(src, "manifest.json")))
    for p in m["pages"]:
        for c in p["controls"]:
            if c["id"] not in MPC:
                sys.exit("%s: %s is not one of the plugin's parameters" % (p["name"], c["id"]))
    if os.path.isdir(out):
        shutil.rmtree(out)
    os.makedirs(out)
    used = set()

    def use(f):
        used.add(f)
        return f

    def knob_overlay(bipolar):
        return overlay("KnobOverlay", O(("version", 2), ("version", 1), ("actions", [FOCUS_LOST]), ("overlayType", "Knob With Header"),
                                        ("scaleType", "Linear"), ("polarityType", "Bipolar" if bipolar else "Unipolar"),
                                        ("showMinusSign", True), ("handleName", "Data")))
    defs = [("knob overlay", knob_overlay(False)), ("knob overlay bipolar", knob_overlay(True)),
            ("menu overlay", overlay("MenuOverlay", O(("version", 1), ("version", 1), ("actions", [FOCUS_LOST]),
                                                      ("withNumbers", False), ("handleName", "Data"))))]
    focus = O(("version", 1), ("backgroundColour", "19ffffff"), ("outlineColour", "fff2c14e"), ("backgroundInset", 1.0),
              ("outlineThickness", 1.0))
    names = {}

    def tab_children(page):
        kids = [entry("panel", "Image", image_data(use(page["background"])), bounds((0, 0, 1280, 628)))]
        boxes = []
        tag = page["name"].lower().replace("/", "")
        for i, c in enumerate(page["controls"]):
            h = handle(c["id"])
            if c["names"]:
                names[c["id"]] = c["names"]
            if c["kind"] == "knob":
                fr = c["frame"]
                kx, ky, w, hh = c["rect"]
                ox, oy = kx - fr[0], ky - fr[1]
                ov = "menu overlay" if c["names"] else "knob overlay bipolar" if c["bipolar"] else "knob overlay"
                knob = O(("version", 3), ("knobType", "FilmStrip"), ("filmStrip", use(c["strip"])), ("numFrames", c["frames"]),
                         ("invert", False), ("dragOrientation", "Vertical"), ("handleName", "Data"))
                name = "%s knob %d %s" % (tag, i, c["id"])
                defs.append((name, component(
                    [action("Mouse Down", "Q-Link", "Data", ""), action("Double Click", "Show Overlay", "Data", ov),
                     action("Enter Pressed", "Show Overlay", "Data", ov)],
                    [entry("knob", "Knob", knob, bounds((ox, oy, w, hh))),
                     entry("focus", "Focus", focus, O(("version", 1), ("acceptsHWFocus", "No"), ("showWhenDataModelInvalid", "Show"),
                                                      ("whenVisible", "WhenFocussed"), ("boundsType", "Absolute"),
                                                      ("bounds", box((0, 0, fr[2], fr[3])))))])))
                kids.append(entry(c["id"], name, O(("version", 1), ("handleName", "Data")), bounds(fr, "Yes"), [("Data", h)]))
                boxes.append((c["id"], fr))
            elif c["kind"] == "button":
                for v, img, nxt in ((0, c["off"], 1), (1, c["on"], 0)):
                    hide = ["IndexedEnabling/%d/2/%s" % (v, h)]
                    data = O(("version", 2), ("onImage", use(img)), ("offImage", img), ("buttonId", nxt),
                             ("numButtonsInGroup", 2), ("handleName", h), ("gestureBehaviour", "Instant"))
                    kids.append(entry(c["id"], "Button", data, bounds(c["rect"], "No", hide, "Hide")))
                boxes.append((c["id"], c["rect"]))
            else:                        # a label: follows its parameter
                strip = O(("version", 2), ("imageType", "FilmStrip"), ("colour", "0"), ("image", use(c["strip"])),
                          ("numFrames", c["frames"]), ("handleName", h))
                kids.append(entry(c["id"] + " label", "Image", strip, bounds(c["rect"])))
        return kids, boxes

    def frame(group, boxes):
        mine = [r for p, r in boxes if p in group]
        if not mine:
            return None
        x0, y0 = min(r[0] for r in mine), min(r[1] for r in mine)
        x1, y1 = max(r[0] + r[2] for r in mine), max(r[1] + r[3] for r in mine)
        return (max(0, x0 - 3), max(0, y0 - 3), min(1280, x1 + 3) - max(0, x0 - 3), min(628, y1 + 3) - max(0, y0 - 3))

    tabs, qmaps = [], []
    for page in m["pages"]:
        groups = PAGES[page["name"]]
        kids, boxes = tab_children(page)
        ids_on_page = {c["id"] for c in page["controls"]}
        rects, q = [], {}
        for row, g in enumerate(groups):
            ids, framed = (g, True) if isinstance(g, list) else g
            if framed:
                r = frame(ids, boxes)
                if r:
                    rects.append(r)
            for col, pid in enumerate(ids):
                if pid not in MPC:
                    sys.exit("%s: Q-Link %s is not a parameter" % (page["name"], pid))
                q[13 + col - 4 * row] = MPC[pid]
        tabs.append(O(("version", 1), ("tabName", page["name"]), ("fnKeyIndex", page["fn"]), ("fnKeySubIndex", page["sub"]),
                      ("componentDefinition", component([], kids)),
                      ("qlinkBoundsData", [box(r) for r in rects])))
        qmaps.append(O(("Tab", page["fn"] + 1), ("SubTab", page["sub"] + 1), ("Bank Direction", "RowTopDown"),
                       ("Q-Links", O(*[("Q-Link %d" % n, q.get(n, -1)) for n in range(1, 17)]))))

    repl = [O(("key", handle(pid)), ("value", v)) for pid, v in sorted(names.items(), key=lambda kv: MPC[kv[0]])]
    tui = O(("pageData", O(("version", 1),
                           ("componentDefinitions", O(("version", 2), ("importFiles", []),
                                                      ("localComponentDefinitions", [O(("key", k), ("value", v)) for k, v in defs]))),
                           ("replacementStrings", repl),
                           ("info", O(("version", 1), ("type", "CompleteDescription"))),
                           ("tabs", tabs))))
    ql = O(("version", 4), ("info", O(("version", 1), ("type", "CompleteDescription"))),
           ("Screen Mode Q-Links", O(("version", 4), ("map", qmaps))),
           ("Program Mode Q-Links", O(*[("Q-Link %d" % (n + 1), MPC[pid]) for n, pid in enumerate(PROGRAM_QLINKS)])))
    with open(os.path.join(out, "TUI.json"), "w") as f:
        f.write(dump(tui) + "\n")
    with open(os.path.join(out, "Q-Links.json"), "w") as f:
        f.write(dump(ql) + "\n")
    try:
        from PIL import Image
    except ImportError:
        Image = None
        print("(no Pillow: the pictures copied as they are, many times larger)")
    for f in sorted(used):
        if Image:
            im = Image.open(os.path.join(src, f))
            if f.endswith("_panel.png"):
                im = im.convert("RGB").quantize(256, method=Image.MEDIANCUT, dither=Image.NONE)
            else:                        # (the controls keep their transparency)
                im = im.convert("RGBA").quantize(256, method=Image.FASTOCTREE, dither=Image.NONE)
            im.save(os.path.join(out, f), optimize=True)
        else:
            shutil.copy2(os.path.join(src, f), os.path.join(out, f))
    if preview and Image:                # each page as the MPC draws it, at the defaults
        os.makedirs(preview, exist_ok=True)
        for page in m["pages"]:
            im = Image.open(os.path.join(src, page["background"])).convert("RGBA")
            for c in page["controls"]:
                if c["kind"] == "button":
                    im.alpha_composite(Image.open(os.path.join(src, c["off"])).convert("RGBA"), tuple(c["rect"][:2]))
                else:
                    st = Image.open(os.path.join(src, c["strip"])).convert("RGBA")
                    h = c["rect"][3]
                    fr = st.crop((0, 0, st.width, h))
                    im.alpha_composite(fr, tuple(c["rect"][:2]))
            im.convert("RGB").save(os.path.join(preview, page["name"].replace("/", "-") + ".png"))
    print("TUI.json: %d pages, %d components; Q-Links.json; %d pictures in %s" % (len(tabs), len(defs), len(used), out))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
