#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 swtdr34mz (SOLARIS-23)
"""SOLARIS-23 for the MPC: builds the plugin folder with Akai's MPC Plugin SDK.

The SDK builds a project of C++ files in one src/ folder. This script makes that project from this repository, so
the MPC plugin always has the same engine as the VST3 (nothing copied by hand, nothing in two places):

  mpc/build/SOLARIS-23/    plugin.json, README.md and skins/ (from mpc/plugin/), and in src/ the plugin
                           (mpc/plugin/src/) with the engine (dsp/: its .cpp, .h and params_list.inc)

then runs the SDK's tools/build.py (native build + host simulator, arm32 and arm64, the skin, version.xml,
validation).

    python3 mpc/build.py --sdk /path/to/mpc-plugin-builder-sdk [--target native|arm32|arm64|all] [--toolchain zig|gcc]

The SDK: https://github.com/inmusic-software/mpc-plugin-builder-sdk (or MPC_SDK=/path). The result:
mpc/build/SOLARIS-23/build/swtdr34mz - VST - SOLARIS-23/ (copy it into "VST2 Plugins" on a drive of the MPC).
"""
import argparse
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
NAME = "SOLARIS-23"
ENGINE = ["engine.cpp", "fx.cpp", "params.cpp", "presets.cpp", "solaris.h", "dsp.h", "fx.h", "maps.h",
          "params_list.inc"]


def run(argv, **kw):
    print("+ " + " ".join('"%s"' % a if " " in a else a for a in argv), flush=True)
    subprocess.check_call(argv, **kw)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--sdk", default=os.environ.get("MPC_SDK", os.path.join(ROOT, "..", "mpc-plugin-builder-sdk")),
                    help="the MPC Plugin SDK's folder (default: $MPC_SDK, or beside this repository)")
    ap.add_argument("--target", default="all", help="native, arm32, arm64 or all (the SDK's build.py)")
    ap.add_argument("--toolchain", default=None, help="zig (the SDK's default), gcc or docker")
    ap.add_argument("--no-validate", action="store_true")
    ap.add_argument("--project-only", action="store_true", help="make the SDK project and stop")
    a = ap.parse_args()

    proj = os.path.join(HERE, "build", NAME)
    src = os.path.join(proj, "src")
    if os.path.isdir(src):
        shutil.rmtree(src)
    os.makedirs(src)
    for f in ("plugin.json", "README.md"):
        shutil.copy2(os.path.join(HERE, "plugin", f), os.path.join(proj, f))
    skins = os.path.join(proj, "skins")  # (the SDK copies a project's skins/ as they are, as "Plugin Skins")
    if os.path.isdir(skins):
        shutil.rmtree(skins)
    if os.path.isdir(os.path.join(HERE, "plugin", "skins")):
        shutil.copytree(os.path.join(HERE, "plugin", "skins"), skins)
    for f in os.listdir(os.path.join(HERE, "plugin", "src")):
        shutil.copy2(os.path.join(HERE, "plugin", "src", f), os.path.join(src, f))
    for f in ENGINE:
        shutil.copy2(os.path.join(ROOT, "dsp", f), os.path.join(src, f))
    if a.project_only:
        print("the SDK project: %s" % proj)
        return 0

    sdk = os.path.abspath(a.sdk)
    sdk_build = os.path.join(sdk, "tools", "build.py")
    if not os.path.isfile(sdk_build):
        sys.exit("the MPC Plugin SDK is not at %s (--sdk or MPC_SDK; git clone "
                 "https://github.com/inmusic-software/mpc-plugin-builder-sdk)" % sdk)
    argv = [sys.executable, sdk_build, proj, "--target", a.target]
    if a.toolchain:
        argv += ["--toolchain", a.toolchain]
    if a.no_validate:
        argv.append("--no-validate")
    run(argv)
    out = os.path.join(proj, "build", "swtdr34mz - VST - " + NAME)
    if os.path.isdir(out):
        print("\nthe plugin folder: %s" % out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
