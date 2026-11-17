"""Read plugin receive/render diagnostics: python -m desktop.diagnose --port 12000."""
import argparse
import json
import socket
import time
from .core import encode, decode

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', default='127.0.0.1')
    parser.add_argument('--port', type=int, default=12000)
    parser.add_argument('--id', type=int, default=1)
    args = parser.parse_args()
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as client:
        client.settimeout(2)
        client.sendto(encode(f'/pulsetext/{args.id}/ping'), (args.host, args.port))
        try:
            data, peer = client.recvfrom(49152)
            address, payload = decode(data)
            print(json.dumps({'address': address, 'endpoint': peer, 'status': payload}, ensure_ascii=False, indent=2))
        except (OSError, ValueError) as error:
            parser.exit(1, f'No valid plugin status: {error}\n')

if __name__ == '__main__':
    main()
