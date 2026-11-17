#!/usr/bin/env python3
"""Check release ZIPs retain their plugin bundle and required notice files."""
import glob
import pathlib
import sys
import zipfile

packages = [pathlib.Path(p) for arg in sys.argv[1:] for p in glob.glob(arg)]
if not packages:
    sys.exit("No release packages found")
for path in packages:
    with zipfile.ZipFile(path) as archive:
        names = {n.replace("\\", "/").lstrip("./") for n in archive.namelist()}
        required = {"LICENSE", "THIRD_PARTY_LICENSES.md", "OFL.txt"}
        missing = required - names
        bundle = "Grainveil.component/" if "-AU-" in path.name else "Grainveil.vst3/"
        if missing or not any(n.startswith(bundle) and not n.endswith("/") for n in names):
            sys.exit(f"Invalid package {path}: missing notices {sorted(missing)} or bundle {bundle}")
        print(f"{path}: bundle and all license notices present")
