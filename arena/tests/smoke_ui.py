"""Offscreen UI/preview exercise; no plugin response is fabricated."""
import os
os.environ['QT_QPA_PLATFORM'] = 'offscreen'
from PySide6.QtWidgets import QApplication
from desktop.app import Window, STYLE
from PySide6.QtGui import QFontDatabase
from desktop.core import ANIMATIONS
app = QApplication([])
app.setStyle('Fusion'); app.setStyleSheet(STYLE)
for font in ['segoeui.ttf', 'seguisb.ttf', 'seguiemj.ttf', 'arial.ttf']:
    QFontDatabase.addApplicationFont(os.path.join(os.environ.get('WINDIR', 'C:/Windows'), 'Fonts', font))
window = Window()
window.show()
app.processEvents()
for animation in ANIMATIONS:
    window.settings['animation'] = animation
    window.settings['text'] = 'PulseText සිංහල 日本語\nUnicode 🎤'
    window.load_controls()
    app.processEvents()
    image = window.preview.grab().toImage()
    assert not image.isNull(), animation
window.controls['strobe'].setChecked(True)
assert window.settings['strobe'] is True
window.stop_strobe()
assert window.settings['strobe'] is False
assert window.connection.status == 'No response'
window.grab().save('.tools/PulseText-preview.png')
window.close()
print('UI initialized; all ten animations painted; stop and honest connection status checked.')
