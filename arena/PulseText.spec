# Build from repository root: pyinstaller --clean --noconfirm PulseText.spec
from PyInstaller.utils.hooks import copy_metadata
import os
metadata = []
for package in ['PySide6', 'shiboken6', 'numpy', 'PyAudioWPatch']:
    metadata += copy_metadata(package)
a = Analysis(['run.py'], pathex=['.'], binaries=[],
             datas=[('presets', 'presets'), ('LICENSE', '.'), ('THIRD_PARTY.md', '.'), ('docs', 'docs')] + metadata,
             hiddenimports=['pyaudiowpatch'], hookspath=[], runtime_hooks=[], excludes=[])
pyz = PYZ(a.pure)
exe = EXE(pyz, a.scripts, [], exclude_binaries=True, name='PulseText', debug=False,
          bootloader_ignore_signals=False, strip=False, upx=False, console=bool(os.environ.get('PULSETEXT_CONSOLE')))
coll = COLLECT(exe, a.binaries, a.datas, strip=False, upx=False, name='PulseText')
