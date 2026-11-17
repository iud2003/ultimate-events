import math
import threading
import time
import numpy as np
from PySide6.QtCore import QThread, Signal
from .core import DEFAULT

class Analyzer:
    """Windowed spectral bands and envelope follower; transient detection, not beat tracking."""
    def __init__(self):
        self.envelope = np.zeros(3)
        self.previous_bass = 0.
        self.last_trigger = -100.

    def process(self, samples, rate, settings, now=None):
        now = time.monotonic() if now is None else now
        samples = np.asarray(samples, dtype=np.float64)
        if samples.ndim == 2:
            samples = samples.mean(axis=1)
        samples = np.nan_to_num(samples)
        if len(samples) < 16:
            return dict(bass=0., mid=0., treble=0., flash=0., level=0.)
        samples = samples - samples.mean()
        window = np.hanning(len(samples))
        spectrum = np.abs(np.fft.rfft(samples * window)) * 2 / max(window.sum(), 1)
        frequencies = np.fft.rfftfreq(len(samples), 1 / rate)
        bands = np.array([math.sqrt(float(np.sum(spectrum[(frequencies >= lo) & (frequencies < hi)] ** 2)))
                          for lo, hi in [(30, 250), (250, 4000), (4000, min(18000, rate / 2))]])
        bands = np.clip((bands * settings['sensitivity'] - settings['threshold']) / max(.001, 1 - settings['threshold']), 0, 1)
        dt = len(samples) / rate
        tau = np.where(bands > self.envelope, settings['attack'], settings['release'])
        self.envelope += (bands - self.envelope) * (1 - np.exp(-dt / tau))
        transient = max(0., bands[0] - self.previous_bass)
        flash = transient >= settings['trigger_threshold'] and now - self.last_trigger >= settings['trigger_interval']
        if flash:
            self.last_trigger = now
        self.previous_bass = float(bands[0])
        return dict(zip(['bass', 'mid', 'treble'], map(float, self.envelope))) | dict(flash=float(flash), level=float(np.clip(np.sqrt(np.mean(samples ** 2)) * settings['sensitivity'], 0, 1)))

def devices():
    import pyaudiowpatch as pa
    with pa.PyAudio() as audio:
        result = []
        for index in range(audio.get_device_count()):
            device = audio.get_device_info_by_index(index)
            if device['maxInputChannels'] > 0:
                result.append((index, ('[System loopback] ' if device.get('isLoopbackDevice') else '') + device['name']))
        return result

class AudioWorker(QThread):
    levels = Signal(dict)
    status = Signal(str)

    def __init__(self, device, settings):
        super().__init__()
        self.device = device
        self.settings = dict(settings)
        self.lock = threading.Lock()

    def configure(self, settings):
        with self.lock:
            self.settings = dict(settings)

    def run(self):
        import pyaudiowpatch as pa
        try:
            with pa.PyAudio() as audio:
                device = audio.get_device_info_by_index(self.device)
                rate = int(device['defaultSampleRate'])
                channels = min(2, int(device['maxInputChannels']))
                analyzer = Analyzer()
                with audio.open(format=pa.paFloat32, channels=channels, rate=rate, input=True,
                                input_device_index=self.device, frames_per_buffer=2048) as stream:
                    self.status.emit('Input active: ' + device['name'])
                    while not self.isInterruptionRequested():
                        data = stream.read(2048, exception_on_overflow=False)
                        with self.lock:
                            settings = dict(self.settings)
                        samples = np.frombuffer(data, dtype=np.float32).reshape(-1, channels)
                        self.levels.emit(analyzer.process(samples, rate, settings))
        except Exception as error:
            self.status.emit('Input unavailable: ' + str(error))
        finally:
            self.levels.emit(dict(bass=0., mid=0., treble=0., flash=0., level=0.))
