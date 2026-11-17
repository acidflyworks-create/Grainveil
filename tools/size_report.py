#!/usr/bin/env python3
"""Report source assets and uncompressed plugin bundle sizes (decimal bytes)."""
import argparse
import hashlib
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
ASSET_LIMIT = 60_000_000
PLUGIN_TARGET = 40_000_000
PLUGIN_LIMIT = 200_000_000
EMBEDDED = {"assets/fonts/Lato-Regular.ttf", "assets/fonts/Lato-SemiBold.ttf"}


def files_under(path):
    if path.is_file() and not path.is_symlink():
        return [path]
    if not path.is_dir() or path.is_symlink():
        raise ValueError(f"missing or invalid path: {path}")
    return sorted(p for p in path.rglob("*") if p.is_file() and not p.is_symlink())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bundle", type=pathlib.Path, help="uncompressed .vst3 or .component bundle")
    parser.add_argument("--output", type=pathlib.Path, help="optional JSON report")
    args = parser.parse_args()
    try:
        assets = []
        for p in files_under(ROOT / "assets"):
            rel = p.relative_to(ROOT).as_posix()
            assets.append({"path": rel, "bytes": p.stat().st_size,
                           "sha256": hashlib.sha256(p.read_bytes()).hexdigest(),
                           "embedded": rel in EMBEDDED})
        if not EMBEDDED.issubset({a["path"] for a in assets}):
            raise ValueError("an embedded font is missing")
        asset_bytes = sum(a["bytes"] for a in assets)
        report = {"units": "decimal bytes; regular files only; symlinks not counted twice",
                  "assets": assets, "asset_bytes": asset_bytes,
                  "embedded_bytes": sum(a["bytes"] for a in assets if a["embedded"]),
                  "asset_limit_bytes": ASSET_LIMIT}
        if asset_bytes > ASSET_LIMIT:
            raise ValueError(f"assets exceed hard limit: {asset_bytes} > {ASSET_LIMIT}")
        if args.bundle is not None:
            bundle_files = files_under(args.bundle)
            if not bundle_files:
                raise ValueError("bundle contains no regular files")
            size = sum(p.stat().st_size for p in bundle_files)
            report["plugin"] = {"bundle": args.bundle.name, "file_count": len(bundle_files),
                                "uncompressed_bytes": size, "target_bytes": PLUGIN_TARGET,
                                "hard_limit_bytes": PLUGIN_LIMIT,
                                "under_target": size < PLUGIN_TARGET}
            if size > PLUGIN_LIMIT:
                raise ValueError(f"plugin exceeds hard limit: {size} > {PLUGIN_LIMIT}")
            if size >= PLUGIN_TARGET:
                print(f"WARNING: {args.bundle.name} misses the 40 MB target ({size} bytes)", file=sys.stderr)
        text = json.dumps(report, indent=2, ensure_ascii=False) + "\n"
        print(text, end="")
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(text, encoding="utf-8")
    except (OSError, ValueError) as exc:
        print(f"size report error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
