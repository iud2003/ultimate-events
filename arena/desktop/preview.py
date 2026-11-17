import math
import time
from PySide6.QtCore import Qt, QRectF, QPointF
from PySide6.QtGui import QColor, QFont, QPainter, QTextDocument, QTextCursor, QTextBlockFormat, QTextOption
from PySide6.QtWidgets import QWidget
from .core import DEFAULT

class Preview(QWidget):
    def __init__(self):
        super().__init__()
        self.settings = dict(DEFAULT)
        self.audio = dict(bass=0, mid=0, treble=0, flash=0)
        self.epoch = time.monotonic()
        self.setMinimumSize(480, 270)

    def restart(self):
        self.epoch = time.monotonic()
        self.update()

    def paintEvent(self, event):
        p = QPainter(self)
        p.fillRect(self.rect(), QColor('#10131a'))
        factor = min(self.width() / 1280, self.height() / 720)
        p.translate((self.width() - 1280 * factor) / 2, (self.height() - 720 * factor) / 2)
        p.scale(factor, factor)
        p.setClipRect(QRectF(0, 0, 1280, 720))
        for y in range(0, 720, 40):
            for x in range(0, 1280, 40):
                p.fillRect(x, y, 40, 40, QColor('#202530' if (x + y) // 40 % 2 else '#191e28'))
        s = self.settings
        if s['blackout']:
            return
        elapsed = time.monotonic() - self.epoch
        t = elapsed * s['speed']
        energy = self.audio['bass'] * .5 + self.audio['mid'] * .3 + self.audio['treble'] * .2
        flash = 0
        if s['strobe']:
            rate = s['bpm'] / 60 * s['subdivision'] if s['strobe_clock'] == 'BPM' else s['strobe_rate']
            flash = self.audio['flash'] if s['strobe_clock'] == 'Audio' else float((elapsed * rate) % 1 < s['pulse_width'])
            flash *= s['strobe_intensity']
        bg = QColor.fromRgbF(*s['background'])
        if s['strobe_target'] == 'Background' and flash:
            bg = QColor.fromRgbF(1, 1, 1, flash)
        p.fillRect(QRectF(0, 0, 1280, 720), bg)
        x, y, scale, fade = s['x'] * 1280, s['y'] * 720, s['scale'] * (1 + energy * s['audio_scale']), 1
        animation = s['animation']
        if animation == 'Scroll left': x += (1 - t * .2 % 2) * 1280
        if animation == 'Scroll right': x += (t * .2 % 2 - 1) * 1280
        if animation == 'Scroll up': y += (1 - t * .2 % 2) * 720
        if animation == 'Scroll down': y += (t * .2 % 2 - 1) * 720
        if animation == 'Fade in/out': fade = math.sin(t * math.pi) * .5 + .5
        if animation == 'Zoom pulse': scale *= 1 + math.sin(t * 2 * math.pi) * .2 * s['intensity']
        if animation == 'Bounce': y -= abs(math.sin(t * math.pi)) * .3 * s['intensity'] * 720
        shake = (.025 * s['intensity'] if animation == 'Shake' else 0) + .04 * energy * s['audio_shake']
        x += math.sin(elapsed * 91) * shake * 1280
        y += math.cos(elapsed * 113) * shake * 720
        p.translate(640 + x, 360 + y)
        p.rotate(s['rotation'])
        p.scale(scale, scale)
        p.translate(-640, -360)
        text = s['text'][:int(t * 12)] if animation == 'Typewriter' else s['text']
        font = QFont(s['font'])
        font.setPixelSize(round(s['size']))
        font.setBold(s['bold'])
        font.setItalic(s['italic'])
        font.setLetterSpacing(QFont.SpacingType.AbsoluteSpacing, s['letter_spacing'])
        doc = QTextDocument()
        doc.setDocumentMargin(0)
        doc.setDefaultFont(font)
        option = QTextOption()
        option.setAlignment({'Left': Qt.AlignLeft, 'Centre': Qt.AlignHCenter, 'Right': Qt.AlignRight}[s['horizontal']])
        doc.setDefaultTextOption(option)
        doc.setPlainText(text)
        doc.setTextWidth(1280)
        cursor = QTextCursor(doc)
        cursor.select(QTextCursor.SelectionType.Document)
        block = QTextBlockFormat()
        block.setLineHeight(s['size'] * s['line_spacing'], QTextBlockFormat.LineHeightTypes.FixedHeight.value)
        cursor.mergeBlockFormat(block)
        colour = QColor.fromRgbF(*s['colour'])
        brightness = 1 + energy * s['audio_brightness']
        colour = QColor.fromRgbF(*[min(1, colour.getRgbF()[i] * brightness) for i in range(3)], colour.alphaF())
        if s['strobe_target'] == 'Text' and flash:
            colour = QColor.fromRgbF(1, 1, 1, colour.alphaF())
        doc.setDefaultStyleSheet('')
        fmt = cursor.charFormat()
        fmt.setForeground(colour)
        cursor.mergeCharFormat(fmt)
        top = {'Top': 0, 'Middle': (720 - doc.size().height()) / 2, 'Bottom': 720 - doc.size().height()}[s['vertical']]
        p.translate(0, top)
        # Approximate halo; Arena uses an alpha-mask shader.
        glow = s['glow'] + energy * s['audio_glow']
        if glow or s['outline']:
            radius = s['outline'] + glow * 5
            p.setOpacity(min(.2, .05 + glow * .08) * s['opacity'] * fade)
            for k in range(12):
                p.save()
                p.translate(math.cos(k * math.tau / 12) * radius, math.sin(k * math.tau / 12) * radius)
                doc.drawContents(p)
                p.restore()
        p.setOpacity(s['opacity'] * fade)
        doc.drawContents(p)
        if s['strobe_target'] == 'Whole output' and flash:
            p.resetTransform()
            p.setOpacity(flash)
            p.fillRect(self.rect(), Qt.white)
