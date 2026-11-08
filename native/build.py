#!/usr/bin/env python3
"""Build native Tffere Strike with a pinned raylib source dependency.

Examples:
  python3 native/build.py --platform linux
  python3 native/build.py --platform windows --zig /path/to/zig
Windows cross builds use Zig 0.13.0. Install it with pip install ziglang==0.13.0.
Linux requires standard X11/OpenGL development headers and gcc/g++.
"""
from __future__ import annotations
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parent
CACHE = ROOT / '.cache'
RAYLIB_SHA = 'aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e'
RAYLIB_URL = 'https://codeload.github.com/raysan5/raylib/tar.gz/refs/tags/5.5'
SOURCES = ['rcore', 'rshapes', 'rtextures', 'rtext', 'rmodels', 'raudio', 'utils', 'rglfw']

def run(command):
    print('+', ' '.join(map(str, command)), flush=True)
    subprocess.run(list(map(str, command)), check=True)

def source():
    CACHE.mkdir(parents=True, exist_ok=True)
    archive = CACHE / 'raylib-5.5.tar.gz'
    if not archive.exists():
        urllib.request.urlretrieve(RAYLIB_URL, archive)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != RAYLIB_SHA:
        raise SystemExit('raylib archive SHA256 mismatch; remove the bad cache and retry')
    directory = CACHE / 'raylib-5.5'
    if not directory.exists():
        with tarfile.open(archive) as tar:
            # The signed-off archive hash is checked first; reject traversal too.
            for item in tar.getmembers():
                destination = (CACHE / item.name).resolve()
                if CACHE.resolve() not in destination.parents:
                    raise SystemExit('Unsafe raylib archive member')
            tar.extractall(CACHE)
    return directory / 'src'

def find_zig(argument):
    if argument:
        return argument
    if shutil.which('zig'):
        return shutil.which('zig')
    try:
        import ziglang
        return str(Path(ziglang.__file__).parent / ('zig.exe' if os.name == 'nt' else 'zig'))
    except ImportError:
        raise SystemExit('Windows cross build needs Zig 0.13.0: pip install ziglang==0.13.0, or pass --zig')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform', choices=['linux', 'windows'], default='windows' if os.name == 'nt' else 'linux')
    parser.add_argument('--zig')
    parser.add_argument('--sysroot', type=Path, help='Optional Linux development-header prefix')
    parser.add_argument('--jobs', type=int, default=min(2, os.cpu_count() or 1))
    parser.add_argument('--library-only', action='store_true')
    parser.add_argument('--package', action='store_true', help='Create a portable Windows ZIP in native/build')
    args = parser.parse_args()
    os.environ.setdefault('ZIG_GLOBAL_CACHE_DIR', str(CACHE / 'zig-global'))
    os.environ.setdefault('ZIG_LOCAL_CACHE_DIR', str(CACHE / 'zig-local'))
    src = source()
    out = ROOT / 'build' / args.platform
    out.mkdir(parents=True, exist_ok=True)
    if args.platform == 'windows':
        zig = find_zig(args.zig)
        cc, cxx, ar = [zig, 'cc', '-target', 'x86_64-windows-gnu'], [zig, 'c++', '-target', 'x86_64-windows-gnu'], [zig, 'ar']
        libraries = ['-lopengl32', '-lgdi32', '-lwinmm']
        executable = out / 'TffereStrike.exe'
        extra = []
    else:
        cc, cxx, ar = ['gcc'], ['g++'], ['ar']
        libraries = ['-lm', '-lpthread', '-ldl', '-lrt', '-l:libX11.so.6', '-l:libGL.so.1']
        executable = out / 'TffereStrike'
        extra = ['-D_GLFW_X11']
        if args.sysroot:
            extra += ['-I' + str(args.sysroot / 'usr/include')]
    flags = ['-O2', '-std=gnu99', '-D_GNU_SOURCE', '-DPLATFORM_DESKTOP_GLFW', '-DGRAPHICS_API_OPENGL_33', '-fno-strict-aliasing', '-I' + str(src), '-I' + str(src / 'external/glfw/include')] + extra
    def compile_module(name):
        obj = out / (name + '.o')
        if not obj.exists() or obj.stat().st_mtime < (src / (name + '.c')).stat().st_mtime:
            run(cc + flags + ['-c', src / (name + '.c'), '-o', obj])
        return obj
    with ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        objects = list(pool.map(compile_module, SOURCES))
    library = out / 'libraylib.a'
    run(ar + ['rcs', library] + objects)
    if args.library_only:
        return
    run(cxx + ['-O2', '-std=c++17', '-I' + str(src), ROOT / 'main.cpp', library, '-o', executable] + libraries + (['-Wl,--subsystem,windows'] if args.platform == 'windows' else []))
    run(cxx + ['-O2', '-std=c++17', ROOT / 'tests.cpp', '-o', out / ('gameplay-tests.exe' if args.platform == 'windows' else 'gameplay-tests')])
    if args.platform == 'linux':
        run([out / 'gameplay-tests'])
    if (ROOT / 'assets').exists():
        shutil.copytree(ROOT / 'assets', out / 'assets', dirs_exist_ok=True)
    shutil.copy2(src.parent / 'LICENSE', out / 'LICENSE-raylib.txt')
    print('Built:', executable)
    if args.package:
        if args.platform != 'windows':
            raise SystemExit('--package currently produces the Windows distribution only')
        readme = out / 'READ-ME.txt'
        readme.write_text('TFFERE STRIKE - WINDOWS x64\n\nExtract this whole ZIP to a folder, then double-click TffereStrike.exe.\nKeep the assets folder beside the executable. No Node.js, Python, or browser is needed.\n\nWASD move; mouse look/fire; 1 rifle; 2 sniper; right-click scope; R reload;\nSpace jump; Shift sprint; Escape pause; Enter deploy/restart.\n\nWindows 10/11 x64 and an OpenGL 3.3-capable graphics driver are required.\nOriginal game source and build instructions: https://github.com/tffere/tffere-strike\n')
        package = ROOT / 'build' / 'TffereStrike-Windows-x64.zip'
        with zipfile.ZipFile(package, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
            for item in (executable, readme, out / 'LICENSE-raylib.txt'):
                archive.write(item, 'TffereStrike/' + item.name)
            for item in sorted((out / 'assets').rglob('*')):
                if item.is_file():
                    archive.write(item, 'TffereStrike/' + str(item.relative_to(out)))
        print('Portable package:', package)

if __name__ == '__main__':
    main()
