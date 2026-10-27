#!/usr/bin/env python3
"""
Doom II RPG - Nintendo 64 Asset Extractor & Analyzer
Extracts and structures game assets from 'Doom 2 RPG.ipa' for use with Libdragon / DragonFS.
"""

import os
import sys
import zipfile
import wave
import struct
from pathlib import Path
from collections import defaultdict

DEFAULT_IPA_PATHS = [
    "Doom 2 RPG.ipa",
    "/Users/gcoursaget/Library/CloudStorage/Dropbox/64dev/doom-ii-rpg-64/Doom 2 RPG.ipa",
]

TARGET_PACKAGE_PREFIX = "Payload/Doom2rpg.app/Packages/"

def find_ipa(custom_path=None):
    if custom_path and os.path.isfile(custom_path):
        return custom_path
    for p in DEFAULT_IPA_PATHS:
        if os.path.isfile(p):
            return p
    return None

def inspect_ipa(ipa_path):
    print(f"[*] Opening: {ipa_path}")
    with zipfile.ZipFile(ipa_path, 'r') as zf:
        total_files = 0
        total_uncompressed = 0
        pkg_files = 0
        pkg_uncompressed = 0
        
        categories = {
            'Audio (WAV)': {'count': 0, 'size': 0},
            'Data (.bin)': {'count': 0, 'size': 0},
            'UI Images (.bmp)': {'count': 0, 'size': 0},
            'PNG Images (.png)': {'count': 0, 'size': 0},
            'ComicBook': {'count': 0, 'size': 0},
            'Other': {'count': 0, 'size': 0},
        }

        for info in zf.infolist():
            if info.is_dir():
                continue
            total_files += 1
            total_uncompressed += info.file_size
            
            if info.filename.startswith(TARGET_PACKAGE_PREFIX):
                pkg_files += 1
                pkg_uncompressed += info.file_size
                rel = info.filename[len(TARGET_PACKAGE_PREFIX):]
                ext = os.path.splitext(rel)[1].lower()
                
                if 'ComicBook/' in rel:
                    categories['ComicBook']['count'] += 1
                    categories['ComicBook']['size'] += info.file_size
                elif ext == '.wav':
                    categories['Audio (WAV)']['count'] += 1
                    categories['Audio (WAV)']['size'] += info.file_size
                elif ext == '.bin':
                    categories['Data (.bin)']['count'] += 1
                    categories['Data (.bin)']['size'] += info.file_size
                elif ext == '.bmp':
                    categories['UI Images (.bmp)']['count'] += 1
                    categories['UI Images (.bmp)']['size'] += info.file_size
                elif ext == '.png':
                    categories['PNG Images (.png)']['count'] += 1
                    categories['PNG Images (.png)']['size'] += info.file_size
                else:
                    categories['Other']['count'] += 1
                    categories['Other']['size'] += info.file_size

        print("\n--- Asset Breakdown inside Packages ---")
        for cat, stat in categories.items():
            mb = stat['size'] / (1024 * 1024)
            print(f"  • {cat:20} : {stat['count']:4} files | {mb:6.2f} MB")
        print(f"Total game package: {pkg_files} files, {pkg_uncompressed / (1024 * 1024):.2f} MB")

def extract_assets(ipa_path, out_dir="filesystem/Packages"):
    out_path = Path(out_dir)
    out_path.mkdir(parents=True, exist_ok=True)
    print(f"\n[*] Extracting package assets to: {out_path.resolve()}")

    extracted_count = 0
    extracted_bytes = 0

    with zipfile.ZipFile(ipa_path, 'r') as zf:
        for info in zf.infolist():
            if not info.filename.startswith(TARGET_PACKAGE_PREFIX) or info.is_dir():
                continue
            
            rel_name = info.filename[len(TARGET_PACKAGE_PREFIX):]
            dest_file = out_path / rel_name
            dest_file.parent.mkdir(parents=True, exist_ok=True)
            
            with zf.open(info) as src, open(dest_file, "wb") as dst:
                data = src.read()
                dst.write(data)
                extracted_bytes += len(data)
                extracted_count += 1

    print(f"[+] Successfully extracted {extracted_count} files ({extracted_bytes / (1024 * 1024):.2f} MB).")

def main():
    custom_path = sys.argv[1] if len(sys.argv) > 1 else None
    ipa_path = find_ipa(custom_path)
    if not ipa_path:
        print("[-] Error: 'Doom 2 RPG.ipa' not found.")
        print("    Please provide the path as argument: python3 tools/extract_assets.py <path_to_ipa>")
        sys.exit(1)
        
    inspect_ipa(ipa_path)
    extract_assets(ipa_path)

if __name__ == "__main__":
    main()
