import copy
import json
import math
from pathlib import Path
import socket
import tempfile
import time
import unittest
import numpy as np
from desktop.core import DEFAULT, Connection, encode, decode, save_preset, load_preset, validate
from desktop.audio import Analyzer

class ProtocolTests(unittest.TestCase):
    def test_unicode_roundtrip(self):
        data = {'text': 'සිංහල / العربية / 日本語\n🎶'}
        packet = encode('/pulsetext/17/state', data)
        self.assertEqual(len(packet) % 4, 0)
        self.assertEqual(decode(packet), ('/pulsetext/17/state', data))

    def test_real_udp_routing_and_status(self):
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as server:
            server.bind(('127.0.0.1', 0)); server.settimeout(1)
            client = Connection(port=server.getsockname()[1], instance=42)
            try:
                client.send('ping')
                request, peer = server.recvfrom(49152)
                self.assertEqual(decode(request), ('/pulsetext/42/ping', None))
                server.sendto(encode('/pulsetext/status', {'id': 41, 'instances': 1}), peer)
                time.sleep(.01); client.poll(); self.assertEqual(client.status, 'No response')
                server.sendto(encode('/pulsetext/status', {'id': 42, 'instances': 2}), peer)
                time.sleep(.01); client.poll(); self.assertIn('2 instances', client.status)
                server.sendto(encode('/pulsetext/status', {'id': 42, 'instances': 1}), peer)
                time.sleep(.01); client.poll(); self.assertEqual(client.status, 'Connected')
                client.last_response -= 3
                self.assertEqual(client.status, 'No response')
            finally: client.close()

    def test_preset_roundtrip_and_disarm(self):
        settings = copy.deepcopy(DEFAULT)
        settings.update(text='Unicode 🎤\nLine two', rotation=17.5, strobe=True, blackout=True)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'test.json'; save_preset(path, settings)
            loaded = load_preset(path)
        settings.update(strobe=False, blackout=False)
        self.assertEqual(loaded, settings)

    def test_reject_bad_presets(self):
        for value in [float('nan'), float('inf'), -1, True, '96']:
            with self.assertRaises(ValueError): validate({'size': value})
        with self.assertRaises(ValueError): validate({'colour': [1, 0, 0]})
        with self.assertRaises(ValueError): validate({'animation': 'Unknown'})
        with self.assertRaises(ValueError): validate({'text': 'x' * 16001})

class AudioTests(unittest.TestCase):
    def tone(self, frequency, amplitude=.5):
        return amplitude * np.sin(2 * np.pi * frequency * np.arange(4096) / 48000)

    def test_band_separation(self):
        for frequency, band in [(100, 'bass'), (1000, 'mid'), (8000, 'treble')]:
            analyzer = Analyzer()
            settings = dict(DEFAULT, sensitivity=1, threshold=0, attack=.001)
            result = analyzer.process(self.tone(frequency), 48000, settings, now=1)
            self.assertGreater(result[band], .3)
            for other in {'bass','mid','treble'} - {band}: self.assertLess(result[other], .02)

    def test_silence_release_and_transient_cooldown(self):
        analyzer = Analyzer(); settings = dict(DEFAULT, sensitivity=1, threshold=0, attack=.001, release=.1)
        first = analyzer.process(self.tone(100),48000,settings,now=0)
        self.assertEqual(first['flash'], 1)
        silent = analyzer.process(np.zeros(4096),48000,settings,now=.05)
        self.assertLess(silent['bass'],first['bass'])
        second = analyzer.process(self.tone(100),48000,settings,now=.1)
        self.assertEqual(second['flash'],0)
        analyzer.process(np.zeros(4096),48000,settings,now=.3)
        self.assertEqual(analyzer.process(self.tone(100),48000,settings,now=.4)['flash'],1)

if __name__ == '__main__': unittest.main()
