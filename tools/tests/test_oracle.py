import socket
import struct
import threading
import unittest
from tools.oracle import Pine

class PineProtocol(unittest.TestCase):
    def test_fragmented_response_and_request(self):
        listener = socket.socket()
        listener.bind(('127.0.0.1', 0))
        listener.listen(1)
        received = []
        def serve():
            with listener:
                connection, _ = listener.accept()
                with connection:
                    received.append(Pine.receive(connection, 5))
                    for fragment in [b'\x09', b'\x00\x00\x00\x00', b'\x01\x00\x00\x00']:
                        connection.sendall(fragment)
        worker = threading.Thread(target=serve)
        worker.start()
        self.assertEqual(Pine(listener.getsockname()[1]).status(), 'paused')
        worker.join(3)
        self.assertEqual(received, [struct.pack('<I', 5) + b'\x0f'])

    def test_capture_address_validation(self):
        for address in [-1, 1, 0x2000000]:
            with self.assertRaises(ValueError): Pine().read32(address)
        with self.assertRaises(ValueError): Pine().load_slot(10)
