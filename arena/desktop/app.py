import copy
import os
from pathlib import Path
import sys
import time
from PySide6.QtCore import Qt, QTimer, QStandardPaths
from PySide6.QtGui import QColor, QFont, QKeySequence, QShortcut
from PySide6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QFormLayout, QLabel, QPushButton, QPlainTextEdit, QLineEdit, QSpinBox, QDoubleSpinBox,
    QComboBox, QCheckBox, QColorDialog, QFontComboBox, QListWidget, QFileDialog,
    QInputDialog, QMessageBox, QScrollArea, QTabWidget, QProgressBar, QSplitter)
from .core import DEFAULT, RANGES, ANIMATIONS, Connection, validate, save_preset, load_preset
from .audio import AudioWorker, devices
from .preview import Preview

STYLE = '''
QWidget { background:#121620; color:#e7ecf4; font-family:"Segoe UI"; font-size:12px; }
QMainWindow {background:#121620;}
QLineEdit,QPlainTextEdit,QSpinBox,QDoubleSpinBox,QComboBox,QListWidget {background:#1c2331; border:1px solid #343e51; border-radius:5px; padding:6px; selection-background-color:#216d8c;}
QPushButton {background:#273348; border:1px solid #40516b; border-radius:5px; padding:9px 14px; font-weight:600;}
QPushButton:hover {background:#354964;}
QPushButton#apply {background:#087e94; border-color:#29c6d5;}
QPushButton#stop {background:#9b3547; border-color:#ed6b7f;}
QPushButton:checked {background:#975f18;}
QTabBar::tab {padding:9px; background:#1c2331;} QTabBar::tab:selected {color:#60dce6;}
QProgressBar {border:0; background:#283143; max-height:14px;} QProgressBar::chunk {background:#48d5ab;}
QLabel#heading {font-size:23px; font-weight:700; color:#6be1e9;}
QSplitter::handle {background:#263044;}
'''

class Window(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('PulseText • Live text controller')
        self.resize(1450, 960)
        self.settings = copy.deepcopy(DEFAULT)
        self.loading = False
        self.controls = {}
        self.connection = None
        self.worker = None
        self.last_audio = 0
        self.levels = dict(bass=0., mid=0., treble=0., flash=0., level=0.)
        self.error = ''
        self.preset_dir = Path(QStandardPaths.writableLocation(QStandardPaths.AppLocalDataLocation)) / 'presets'
        self.preset_dir.mkdir(parents=True, exist_ok=True)
        self.root = QWidget()
        self.setCentralWidget(self.root)
        layout = QVBoxLayout(self.root)
        top = QHBoxLayout()
        title = QLabel('PULSE / TEXT'); title.setObjectName('heading'); top.addWidget(title)
        self.status = QLabel('No response'); top.addWidget(self.status, 1)
        self.host = QLineEdit('127.0.0.1'); self.host.setMaximumWidth(150)
        self.port = QSpinBox(); self.port.setRange(1024, 65535); self.port.setValue(12000)
        self.target = QSpinBox(); self.target.setRange(1, 999)
        for name, widget in [('Host', self.host), ('Port', self.port), ('Control ID', self.target)]:
            top.addWidget(QLabel(name)); top.addWidget(widget)
        self.button(top, 'Connect target', self.connect_target)
        layout.addLayout(top)
        actions = QHBoxLayout()
        self.button(actions, 'Apply  Ctrl+Enter', self.apply, 'apply')
        self.button(actions, 'Restart animation', self.restart)
        self.button(actions, 'STOP STROBE  Esc', self.stop_strobe, 'stop')
        self.blackout = QPushButton('Blackout'); self.blackout.setCheckable(True); self.blackout.toggled.connect(self.toggle_blackout); actions.addWidget(self.blackout)
        layout.addLayout(actions)
        splitter = QSplitter()
        left = QWidget(); ll = QVBoxLayout(left); ll.addWidget(QLabel('PRESETS'))
        self.presets = QListWidget(); self.presets.itemDoubleClicked.connect(self.recall); ll.addWidget(self.presets)
        for name, callback in [('Load selected', self.recall), ('Save as…', self.save_as), ('Import…', self.import_preset), ('Export…', self.export_preset)]:
            self.button(ll, name, callback)
        splitter.addWidget(left)
        center = QWidget(); cl = QVBoxLayout(center)
        cl.addWidget(QLabel('LIVE TEXT  •  edits apply after 180 ms'))
        self.editor = QPlainTextEdit(self.settings['text']); self.editor.setMinimumHeight(120); self.editor.setMaximumHeight(210)
        self.editor.setPlaceholderText('Type multiline Unicode text…'); self.editor.textChanged.connect(self.text_changed); cl.addWidget(self.editor)
        cl.addWidget(QLabel('APPROXIMATE PREVIEW  •  1280 × 720  •  checkerboard = transparency'))
        self.preview = Preview(); cl.addWidget(self.preview, 1)
        cl.addWidget(QLabel('Arena uses DirectWrite and GPU effects. Font metrics, wrapping and glow can differ.'))
        splitter.addWidget(center)
        tabs = QTabWidget()
        appearance = self.form_tab(tabs, 'Appearance')
        font = QFontComboBox(); font.currentFontChanged.connect(lambda value: self.change('font', value.family())); appearance.addRow('Font', font); self.controls['font'] = font
        for key, label in [('size', 'Size (output pixels)'), ('opacity', 'Opacity')]: self.numeric(appearance, key, label)
        for key, label in [('bold', 'Bold'), ('italic', 'Italic')]: self.check(appearance, key, label)
        for key, label in [('colour', 'Text colour / alpha'), ('background', 'Background / alpha')]:
            button = QPushButton(label); button.clicked.connect(lambda checked=False, k=key: self.choose_colour(k)); appearance.addRow(button); self.controls[key] = button
        self.combo(appearance, 'horizontal', 'Horizontal', ['Left', 'Centre', 'Right'])
        self.combo(appearance, 'vertical', 'Vertical', ['Top', 'Middle', 'Bottom'])
        for key, label in [('x','X position'), ('y','Y position'), ('rotation','Rotation °'), ('scale','Scale'), ('letter_spacing','Letter spacing px'), ('line_spacing','Line spacing'), ('outline','Outline px'), ('glow','Glow')]: self.numeric(appearance, key, label)
        animation = self.form_tab(tabs, 'Animation')
        self.combo(animation, 'animation', 'Animation', ANIMATIONS)
        self.numeric(animation, 'speed', 'Speed')
        self.numeric(animation, 'intensity', 'Intensity')
        animation.addRow(QLabel('Elapsed-time animation; Restart resets phase.'))
        splitter.addWidget(tabs); splitter.setSizes([190, 780, 360]); layout.addWidget(splitter, 1)
        bottom = QTabWidget(); bottom.setMaximumHeight(310)
        audio = self.form_tab(bottom, 'Audio input & reaction')
        device_row = QHBoxLayout(); self.device = QComboBox(); device_row.addWidget(self.device, 1)
        self.button(device_row, 'Refresh', self.refresh_devices)
        self.button(device_row, 'Start input', self.start_audio)
        self.button(device_row, 'Stop input', self.stop_audio)
        audio.addRow(device_row)
        self.audio_status = QLabel('Input stopped'); audio.addRow(self.audio_status)
        self.meter = QProgressBar(); self.meter.setRange(0, 1000); self.meter.setTextVisible(False); audio.addRow('Input level', self.meter)
        self.bands = QLabel('Bass 0.00   Mid 0.00   Treble 0.00'); audio.addRow(self.bands)
        for key, label in [('sensitivity','Sensitivity'), ('threshold','Noise threshold'), ('attack','Attack seconds'), ('release','Release seconds'), ('audio_scale','Scale mapping'), ('audio_glow','Glow mapping'), ('audio_brightness','Brightness mapping'), ('audio_shake','Shake mapping')]: self.numeric(audio, key, label)
        strobe = self.form_tab(bottom, 'Strobe • flashing-light notice')
        notice = QLabel('FLASHING LIGHTS may trigger photosensitive reactions. Strobe starts OFF. Esc always stops flashing.'); notice.setStyleSheet('color:#ffbc7d'); notice.setWordWrap(True); strobe.addRow(notice)
        self.check(strobe, 'strobe', 'Enable strobe')
        self.combo(strobe, 'strobe_clock', 'Clock', ['Rate', 'BPM', 'Audio'])
        self.combo(strobe, 'strobe_target', 'Flash target', ['Text', 'Background', 'Whole output'])
        for key, label in [('strobe_rate','Rate Hz'), ('strobe_intensity','Flash intensity'), ('pulse_width','Pulse width fraction'), ('bpm','BPM'), ('subdivision','Flashes per beat'), ('trigger_threshold','Bass transient threshold'), ('trigger_interval','Minimum trigger interval s'), ('timeout','Connection timeout s')]: self.numeric(strobe, key, label)
        strobe.addRow(QLabel('Audio mode detects bass transients; it does not track musical beats.'))
        layout.addWidget(bottom)
        self.debounce = QTimer(self); self.debounce.setSingleShot(True); self.debounce.setInterval(180); self.debounce.timeout.connect(self.apply)
        self.continuous = QTimer(self); self.continuous.setSingleShot(True); self.continuous.setInterval(50); self.continuous.timeout.connect(self.apply)
        self.tick_timer = QTimer(self); self.tick_timer.timeout.connect(self.tick); self.tick_timer.start(33)
        self.ping_timer = QTimer(self); self.ping_timer.timeout.connect(self.ping); self.ping_timer.start(500)
        self.safety_shortcut = QShortcut(QKeySequence('Escape'), self); self.safety_shortcut.activated.connect(self.stop_strobe)
        for sequence, callback in [('Ctrl+Return', self.apply), ('Ctrl+R', self.restart), ('Ctrl+B', lambda: self.blackout.toggle())]:
            shortcut = QShortcut(QKeySequence(sequence), self)
            shortcut.activated.connect(lambda cb=callback: self.guarded_shortcut(cb))
        self.load_controls()
        self.seed_presets(); self.refresh_presets(); self.refresh_devices(); self.connect_target()

    def form_tab(self, tabs, title):
        widget = QWidget(); form = QFormLayout(widget); form.setFieldGrowthPolicy(QFormLayout.FieldGrowthPolicy.AllNonFixedFieldsGrow)
        scroll = QScrollArea(); scroll.setWidgetResizable(True); scroll.setWidget(widget); tabs.addTab(scroll, title)
        return form

    def button(self, layout, title, callback, name=None):
        button = QPushButton(title)
        if name: button.setObjectName(name)
        button.clicked.connect(callback); layout.addWidget(button)
        return button

    def numeric(self, form, key, label):
        spin = QDoubleSpinBox(); spin.setRange(*RANGES[key]); spin.setDecimals(3 if key in ('attack', 'pulse_width') else 2)
        spin.setSingleStep(.05 if RANGES[key][1] <= 10 else 1)
        spin.valueChanged.connect(lambda value: self.change(key, value)); form.addRow(label, spin); self.controls[key] = spin

    def combo(self, form, key, label, options):
        combo = QComboBox(); combo.addItems(options); combo.currentTextChanged.connect(lambda text: self.change(key, text)); form.addRow(label, combo); self.controls[key] = combo

    def check(self, form, key, label):
        check = QCheckBox(label); check.toggled.connect(lambda value: self.change(key, value)); form.addRow(check); self.controls[key] = check

    def load_controls(self):
        self.loading = True
        self.editor.setPlainText(self.settings['text'])
        for key, widget in self.controls.items():
            value = self.settings[key]
            if isinstance(widget, QFontComboBox): widget.setCurrentFont(QFont(value))
            elif isinstance(widget, QComboBox): widget.setCurrentText(value)
            elif isinstance(widget, QCheckBox): widget.setChecked(value)
            elif isinstance(widget, QDoubleSpinBox): widget.setValue(value)
            elif isinstance(widget, QPushButton): widget.setStyleSheet('border-left:8px solid ' + QColor.fromRgbF(*value).name() + ';')
        self.blackout.setChecked(self.settings['blackout'])
        self.loading = False
        self.preview.settings = copy.deepcopy(self.settings)

    def change(self, key, value):
        if self.loading: return
        self.settings[key] = value
        self.preview.settings = copy.deepcopy(self.settings)
        if self.worker: self.worker.configure(self.settings)
        if key == 'strobe' and not value:
            self.send('stop')
        if not self.continuous.isActive(): self.continuous.start()

    def text_changed(self):
        if self.loading: return
        self.settings['text'] = self.editor.toPlainText()
        self.preview.settings = copy.deepcopy(self.settings)
        self.debounce.start()

    def choose_colour(self, key):
        colour = QColorDialog.getColor(QColor.fromRgbF(*self.settings[key]), self, key, QColorDialog.ColorDialogOption.ShowAlphaChannel)
        if colour.isValid():
            self.change(key, list(colour.getRgbF())); self.load_controls()

    def send(self, command, payload=None):
        if not self.connection: return
        try: self.connection.send(command, payload)
        except (OSError, ValueError) as error: self.error = str(error)

    def connect_target(self):
        self.stop_strobe()
        if self.connection: self.connection.close()
        self.connection = None
        try:
            self.connection = Connection(self.host.text().strip(), self.port.value(), self.target.value())
            self.error = ''; self.apply(); self.ping()
        except OSError as error: self.error = str(error)

    def apply(self):
        try:
            settings = validate(self.settings)
            self.error = ''
            self.send('state', settings)
            self.preview.settings = copy.deepcopy(settings)
        except ValueError as error: self.error = str(error)

    def restart(self):
        self.send('restart'); self.preview.restart()

    def stop_strobe(self):
        self.settings['strobe'] = False
        if 'strobe' in self.controls:
            widget = self.controls['strobe']; widget.blockSignals(True); widget.setChecked(False); widget.blockSignals(False)
        self.preview.settings = copy.deepcopy(self.settings)
        # Repeat stop datagrams on successive timer ticks to tolerate occasional UDP loss.
        self.send('stop')
        if self.connection:
            connection = self.connection
            def repeat():
                if self.connection is connection and not self.settings['strobe']: self.send('stop')
            QTimer.singleShot(40, repeat); QTimer.singleShot(100, repeat)

    def toggle_blackout(self, enabled):
        if self.loading: return
        self.settings['blackout'] = enabled; self.preview.settings = copy.deepcopy(self.settings)
        self.send('blackout', {'enabled': enabled})

    def guarded_shortcut(self, callback):
        focus = QApplication.focusWidget()
        if isinstance(focus, (QLineEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox)): return
        callback()

    def ping(self):
        self.send('ping'); self.send('heartbeat')

    def tick(self):
        if self.connection:
            try: self.connection.poll()
            except OSError as error: self.error = str(error)
            self.status.setText(self.error or self.connection.status)
        else: self.status.setText(self.error or 'No response')
        if time.monotonic() - self.last_audio > .5:
            self.levels = dict(bass=0., mid=0., treble=0., flash=0., level=0.)
            if self.worker and self.worker.isRunning(): self.audio_status.setText('Waiting for input / device unavailable')
        self.send('audio', self.levels)
        self.preview.audio = dict(self.levels)
        self.meter.setValue(round(self.levels['level'] * 1000))
        self.bands.setText('Bass %.2f   Mid %.2f   Treble %.2f' % tuple(self.levels[k] for k in ('bass', 'mid', 'treble')))
        self.preview.update()
        self.levels['flash'] = 0.

    def refresh_devices(self):
        self.device.clear()
        try:
            for index, name in devices(): self.device.addItem(name, index)
            if not self.device.count(): self.audio_status.setText('No microphone or loopback input available')
        except Exception as error: self.audio_status.setText('Audio enumeration failed: ' + str(error))

    def start_audio(self):
        if self.worker and self.worker.isRunning():
            self.audio_status.setText('Stop the current input before switching devices'); return
        if self.device.currentData() is None: return
        self.worker = AudioWorker(self.device.currentData(), self.settings)
        self.worker.levels.connect(self.on_levels); self.worker.status.connect(self.audio_status.setText)
        self.audio_status.setText('Opening input…'); self.worker.start()

    def on_levels(self, levels):
        if self.worker and self.worker.isInterruptionRequested(): return
        self.levels = levels; self.last_audio = time.monotonic()

    def stop_audio(self):
        if self.worker: self.worker.requestInterruption()
        self.levels = dict(bass=0., mid=0., treble=0., flash=0., level=0.)
        self.audio_status.setText('Input stopped')

    def seed_presets(self):
        source = Path(getattr(sys, '_MEIPASS', Path(__file__).resolve().parent.parent)) / 'presets'
        for path in source.glob('*.json'):
            target = self.preset_dir / path.name
            if not target.exists():
                try: save_preset(target, load_preset(path))
                except (OSError, ValueError): pass

    def refresh_presets(self):
        self.presets.clear()
        for path in sorted(self.preset_dir.glob('*.json')): self.presets.addItem(path.stem)

    def recall(self, *_):
        item = self.presets.currentItem()
        if not item: return
        try:
            self.stop_strobe(); self.settings = load_preset(self.preset_dir / (item.text() + '.json'))
            self.load_controls(); self.apply(); self.restart()
        except (OSError, ValueError) as error: QMessageBox.warning(self, 'Preset error', str(error))

    def save_as(self):
        name, ok = QInputDialog.getText(self, 'Save preset', 'Preset name')
        if not ok: return
        name = ''.join(c for c in name if c.isalnum() or c in ' -_').strip()[:80]
        if not name: return
        try: save_preset(self.preset_dir / (name + '.json'), self.settings); self.refresh_presets()
        except (OSError, ValueError) as error: QMessageBox.warning(self, 'Save error', str(error))

    def import_preset(self):
        path, _ = QFileDialog.getOpenFileName(self, 'Import preset', '', 'PulseText presets (*.json)')
        if not path: return
        try:
            settings = load_preset(path)
            safe_name = ''.join(c for c in Path(path).stem if c.isalnum() or c in ' -_')[:80] or 'Imported'
            save_preset(self.preset_dir / (safe_name + '.json'), settings); self.refresh_presets()
        except (OSError, ValueError) as error: QMessageBox.warning(self, 'Import error', str(error))

    def export_preset(self):
        path, _ = QFileDialog.getSaveFileName(self, 'Export preset', 'PulseText.json', 'PulseText presets (*.json)')
        if not path: return
        try: save_preset(path, self.settings)
        except (OSError, ValueError) as error: QMessageBox.warning(self, 'Export error', str(error))

    def closeEvent(self, event):
        self.stop_strobe(); self.stop_audio()
        if self.worker and self.worker.isRunning() and not self.worker.wait(1500):
            self.audio_status.setText('Waiting for the audio driver to release input…')
            QTimer.singleShot(250, self.close); event.ignore(); return
        if self.connection: self.connection.close()
        event.accept()

def main():
    app = QApplication(sys.argv); app.setApplicationName('PulseText'); app.setOrganizationName('PulseText')
    app.setStyle('Fusion'); app.setStyleSheet(STYLE)
    window = Window()
    if '--smoke-test' in sys.argv:
        # Exercise packaged imports, widget creation and painting without a visible window.
        window.preview.grab()
        QTimer.singleShot(250, window.close)
        QTimer.singleShot(500, app.quit)
    else:
        window.show()
    sys.exit(app.exec())

if __name__ == '__main__': main()
