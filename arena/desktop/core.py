import copy
import json
import math
import socket
import time
from pathlib import Path

ANIMATIONS = ['Static', 'Scroll left', 'Scroll right', 'Scroll up', 'Scroll down',
              'Fade in/out', 'Zoom pulse', 'Bounce', 'Typewriter', 'Shake']
RANGES = {
    'size': (8, 500), 'opacity': (0, 1), 'x': (-2, 2), 'y': (-2, 2),
    'rotation': (-360, 360), 'scale': (.05, 5), 'letter_spacing': (-20, 100),
    'line_spacing': (.5, 3), 'outline': (0, 20), 'glow': (0, 3),
    'speed': (0, 10), 'intensity': (0, 2), 'audio_scale': (0, 2),
    'audio_glow': (0, 3), 'audio_brightness': (0, 2), 'audio_shake': (0, 2),
    'sensitivity': (.1, 20), 'threshold': (0, 1), 'attack': (.001, 1),
    'release': (.01, 3), 'strobe_rate': (.1, 30), 'strobe_intensity': (0, 1),
    'pulse_width': (.01, .95), 'bpm': (20, 300), 'subdivision': (.25, 8),
    'trigger_threshold': (0, 1), 'trigger_interval': (.05, 3), 'timeout': (.3, 10),
}
DEFAULT = dict(text='PulseText', font='Segoe UI', size=96, bold=False, italic=False,
               colour=[.3, .85, 1, 1], background=[0, 0, 0, 0], opacity=1,
               horizontal='Centre', vertical='Middle', x=0, y=0, rotation=0,
               scale=1, letter_spacing=0, line_spacing=1.2, outline=0, glow=.2,
               animation='Static', speed=1, intensity=1, audio_scale=0,
               audio_glow=0, audio_brightness=0, audio_shake=0,
               sensitivity=4, threshold=.02, attack=.03, release=.3,
               strobe=False, strobe_rate=4, strobe_intensity=1, pulse_width=.2,
               strobe_clock='Rate', strobe_target='Text', bpm=120, subdivision=1,
               trigger_threshold=.15, trigger_interval=.2, timeout=2, blackout=False)

def validate(data):
    if not isinstance(data, dict):
        raise ValueError('Preset settings must be an object')
    result = copy.deepcopy(DEFAULT)
    for key, value in data.items():
        if key not in result:
            continue
        if key in RANGES:
            lo, hi = RANGES[key]
            if isinstance(value, bool) or not isinstance(value, (float, int)) or not math.isfinite(value) or not lo <= value <= hi:
                raise ValueError(f'{key} must be between {lo} and {hi}')
        elif key in ('colour', 'background'):
            if not isinstance(value, list) or len(value) != 4 or any(isinstance(x, bool) or not isinstance(x, (int, float)) or not math.isfinite(x) or not 0 <= x <= 1 for x in value):
                raise ValueError(f'{key} must contain four values from 0 to 1')
        elif isinstance(result[key], bool):
            if not isinstance(value, bool):
                raise ValueError(f'{key} must be true or false')
        elif not isinstance(value, str):
            raise ValueError(f'{key} must be text')
        result[key] = value
    for key, options in {'animation': ANIMATIONS, 'horizontal': ['Left', 'Centre', 'Right'],
                         'vertical': ['Top', 'Middle', 'Bottom'], 'strobe_clock': ['Rate', 'BPM', 'Audio'],
                         'strobe_target': ['Text', 'Background', 'Whole output']}.items():
        if result[key] not in options:
            raise ValueError(f'Invalid {key}')
    if len(result['text'].encode('utf-8')) > 16000 or len(result['font']) > 256:
        raise ValueError('Text is limited to 16,000 UTF-8 bytes; font name to 256 characters')
    return result

def save_preset(path, settings):
    path = Path(path)
    document = {'schema': 1, 'settings': validate(settings)}
    # Safety state is never recalled armed.
    document['settings'].update(strobe=False, blackout=False)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(document, ensure_ascii=False, indent=2), encoding='utf-8')
    temporary.replace(path)

def load_preset(path):
    if Path(path).stat().st_size > 64000:
        raise ValueError('Preset file is too large')
    document = json.loads(Path(path).read_text(encoding='utf-8'))
    if not isinstance(document, dict) or document.get('schema') != 1:
        raise ValueError('Unsupported preset schema')
    settings = validate(document.get('settings'))
    settings.update(strobe=False, blackout=False)
    return settings

def osc_string(value):
    data = value.encode('utf-8') + b'\0'
    return data + b'\0' * (-len(data) % 4)

def encode(address, payload=None):
    data = osc_string(address) + osc_string(',s' if payload is not None else ',')
    if payload is not None:
        data += osc_string(json.dumps(payload, ensure_ascii=False, separators=(',', ':'), allow_nan=False))
    if len(data) > 48000:
        raise ValueError('OSC message exceeds 48 KB')
    return data

def decode(data):
    def read(pos):
        end = data.index(b'\0', pos)
        return data[pos:end].decode('utf-8'), (end + 4) & ~3
    address, pos = read(0)
    types, pos = read(pos)
    if types == ',':
        return address, None
    if types != ',s':
        raise ValueError('Unsupported OSC arguments')
    value, pos = read(pos)
    return address, json.loads(value)

class Connection:
    def __init__(self, host='127.0.0.1', port=12000, instance=1):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.bind(('0.0.0.0', 0))
        self.socket.setblocking(False)
        self.destination = (socket.gethostbyname(host), int(port))
        self.instance = int(instance)
        self.last_response = 0
        self.instances = 0

    def send(self, command, payload=None):
        self.socket.sendto(encode(f'/pulsetext/{self.instance}/{command}', payload), self.destination)

    def poll(self):
        for _ in range(64):
            try:
                data, peer = self.socket.recvfrom(49152)
            except BlockingIOError:
                break
            except ConnectionResetError:
                # Windows reports an ICMP port-unreachable from an earlier UDP send.
                continue
            try:
                address, payload = decode(data)
                if peer == self.destination and address == '/pulsetext/status' and payload['id'] == self.instance:
                    self.instances = int(payload['instances'])
                    self.last_response = time.monotonic()
            except (ValueError, KeyError, TypeError, UnicodeError):
                continue

    @property
    def status(self):
        if time.monotonic() - self.last_response > 2:
            return 'No response'
        return 'Connected' if self.instances == 1 else f'{self.instances} instances: check Control ID'

    def close(self):
        self.socket.close()
