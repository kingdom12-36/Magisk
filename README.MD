# ShadowMask

**ShadowMask** is a hardened fork of [Magisk](https://github.com/topjohnwu/Magisk) focused on deep root-hiding and detection evasion — built for Android 6.0 and above.

---

## What makes ShadowMask different

Standard Magisk leaves several fingerprints that game anti-cheat SDKs, Play Integrity checks, and banking apps can detect. ShadowMask closes those gaps:

| Vector | How ShadowMask hides it |
|--------|------------------------|
| Zygisk native bridge | Spoofs `ro.dalvik.vm.native.bridge` → `"0"` instead of leaking `"libzygisk.so"` |
| `/proc` self-maps | Filters Magisk/Zygisk entries from `/proc/self/maps` and `/proc/self/mountinfo` |
| SELinux context | Spoofs `/proc/self/attr/current` reads |
| `xattr` / file metadata | Hooks `getxattr`, `lgetxattr`, `fgetxattr` to hide magic mount markers |
| `stat` / `stat64` / `lstat64` | Returns clean results for hidden paths |
| `/data/adb` existence | Access/open/stat hooks intercept and deny visibility |
| PLT spoof hooks | Patched GOT entries in `libjavacore.so` and sibling libs; Magisk stays mapped to prevent use-after-unmap crashes (SIGSEGV SEGV_MAPERR) |
| Root by default | Root access is **denied to all apps** unless explicitly granted — no accidental leaks |

---

## Features

- **ShadowMaskSU** — Root access, hidden by default from all apps
- **Magisk Modules** — Full module support (read-only partition overlays)
- **MagiskBoot** — Unpack / repack Android boot images
- **Zygisk** — Code injection into every app process, with deep native-bridge hiding
- **DenyList** — Per-app root hiding with extended native-level evasion

---

## Downloads

Get the latest build from the [Releases](../../releases/latest) page of this repository.

Install `shadowmask-release.apk` to get the full ShadowMask app.

> **Warning:** Only install APKs from this repository. Files from unknown sources can be malicious.

---

## Building

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/kingdom12-36/ShadowMask.git
cd ShadowMask

# Build (requires Android NDK + JDK 17+)
./build.py -v fast
```

CI builds are triggered automatically on every push to `master` and published to GitHub Releases.

---

## Strings / Translations

Default string resources are located at:

- `app/core/src/main/res/values/strings.xml`
- `app/stub-res/src/main/res/values/strings.xml`

Place translations in `[module]/src/main/res/values-[lang]/strings.xml`.

---

## License

```
ShadowMask, including all git submodules, is free software:
you can redistribute it and/or modify it under the terms of the
GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
```
