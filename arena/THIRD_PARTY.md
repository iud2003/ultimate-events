# Third-party components

PulseText's original source is MIT licensed (LICENSE). Upstream notices remain in the vendored SDK. This is not a Resolume product or an endorsement by Resolume.

* Resolume FFGL SDK v2.2, commit `fda8d4a5904eaf09dd97ac0f95c246bd7f9f9a46`: https://github.com/resolume/ffgl. The FreeFrame/FFGL files carry BSD-style redistribution notices; see `vendor/ffgl/source/lib/ffgl/FFGLLib.h` and source headers. The SDK includes glsdk/glload with its own retained notices. Preserve upstream source notices and reproduce relevant binary redistribution notices when shipping plugins.
* nlohmann/json 3.11.3: MIT, https://github.com/nlohmann/json/blob/v3.11.3/LICENSE.MIT. CMake fetches the pinned archive and retains its license in the build dependency tree.
* PySide6 / Shiboken 6.8.3 and Qt: LGPLv3/GPL/commercial options, https://doc.qt.io/qtforpython-6/licenses.html. The desktop package uses dynamically loaded Qt libraries; preserve license texts, replacement/relinking rights and applicable source offers when distributing. PyInstaller's one-folder layout keeps those libraries separate.
* NumPy 2.2.4: BSD and bundled component notices, https://github.com/numpy/numpy/blob/v2.2.4/LICENSE.txt.
* PyAudioWPatch 0.2.12.7: MIT with bundled PortAudio notices, https://github.com/s0d3s/PyAudioWPatch.
* PyInstaller 6.12.0: GPL with bootloader exception, https://pyinstaller.org/en/v6.12.0/license.html. Other packaging dependencies retain their distribution metadata and notices.
* Local MinGW GCC build uses statically linked runtime libraries subject to the GCC Runtime Library Exception and applicable mingw-w64/winpthreads notices. MSVC is the documented alternative build toolchain.

When redistributing, include this file, LICENSE, upstream SDK notices, the fetched JSON license, and the Python package `.dist-info` licenses/metadata and Qt licenses from your installed environment. The source tree and build instructions permit recipients to rebuild and replace the dynamically loaded desktop libraries. This file identifies dependencies; it does not replace their license terms.
