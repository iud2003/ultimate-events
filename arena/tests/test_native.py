import os
from pathlib import Path
import subprocess
import time
import unittest
from desktop.core import Connection

HARNESS = os.environ.get('PULSETEXT_RECEIVER_TEST', 'build/mingw/native_receiver.exe')

@unittest.skipUnless(Path(HARNESS).exists(), 'Build native_receiver first')
class NativeTests(unittest.TestCase):
    def test_timeout_latches(self):
        result = subprocess.run([HARNESS, '--timeout'], timeout=5)
        self.assertEqual(result.returncode, 0)

    def test_native_instance_routing(self):
        process = subprocess.Popen([HARNESS])
        first = Connection(port=12345, instance=7)
        absent = Connection(port=12345, instance=99)
        try:
            for _ in range(30):
                first.send('ping'); absent.send('ping'); time.sleep(.03); first.poll(); absent.poll()
                if first.status == 'Connected': break
            self.assertEqual(first.status, 'Connected')
            self.assertEqual(absent.instances, 0)
            self.assertNotEqual(absent.last_response, 0)
            absent.send('state', {'text': 'must not reach either instance'})
            first.socket.sendto(b'malformed OSC', first.destination)
            first.send('state', {'text': 'native routing verified'})
            self.assertEqual(process.wait(timeout=5), 0)
        finally:
            first.close(); absent.close()
            if process.poll() is None: process.kill(); process.wait()

if __name__ == '__main__': unittest.main()
