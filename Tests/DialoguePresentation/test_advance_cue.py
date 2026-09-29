import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
import wave

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'Tools/DialoguePresentation'))
import generate_advance_cue as g


class CueTests(unittest.TestCase):
    def test_pcm_format_duration_and_bounds(self):
        raw = g.cue_bytes()
        with wave.open(io.BytesIO(raw), 'rb') as wav:
            self.assertEqual((wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getnframes()), (1, 2, 48000, 5280))
            frames = struct.unpack('<5280h', wav.readframes(5280))
        self.assertEqual((frames[0], frames[-1]), (0, 0))
        self.assertTrue(1000 < max(abs(x) for x in frames) <= 11000)
        self.assertTrue(any(frames[:192]))  # Audible onset within first 4 ms.
        self.assertTrue(all(x == 0 for x in frames[4560:]))
        self.assertLess(abs(sum(frames) / len(frames)), 100)
        self.assertEqual(len(raw), 10604)

    def test_exact_artifact_pin_and_repeatability(self):
        expected = json.loads(g.PIN.read_text())
        self.assertEqual(hashlib.sha256(g.cue_bytes()).hexdigest(), '5e59dc027766133335bfcce8f1625d167631f038d2849ed3304a863866340834')
        self.assertEqual(g.manifest(g.cue_bytes()), expected)
        self.assertEqual(g.cue_bytes(), g.cue_bytes())

    def test_fresh_generation_and_check(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / 'audio'
            g.run(output)
            g.run(output, check=True)
            self.assertEqual((output / g.NAME).read_bytes(), g.cue_bytes())

    def test_overwrite_is_refused(self):
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaises(FileExistsError):
                g.run(Path(temp))

    def test_modified_output_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / 'audio'
            g.run(output)
            (output / g.NAME).write_bytes(b'bad')
            with self.assertRaises(ValueError):
                g.run(output, check=True)

    def test_source_tree_output_rejected(self):
        with self.assertRaises(ValueError):
            g.run(ROOT / 'GeneratedAudio')

    def test_incorrect_pin_rejected_before_output(self):
        with tempfile.TemporaryDirectory() as temp:
            p = Path(temp) / 'bad-pin.json'
            p.write_text('{}\n')
            output = Path(temp) / 'out'
            with patch.object(g, 'PIN', p), self.assertRaises(ValueError):
                g.run(output)
            self.assertFalse(output.exists())

    def test_crlf_pin_is_portable(self):
        with tempfile.TemporaryDirectory() as temp:
            p = Path(temp) / 'crlf.json'
            p.write_bytes(g.PIN.read_bytes().replace(b'\n', b'\r\n'))
            with patch.object(g, 'PIN', p):
                g.run(Path(temp) / 'out')

    def test_symlink_output_file_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / 'out'
            g.run(output)
            original = output / g.NAME
            moved = Path(temp) / 'moved.wav'
            original.rename(moved)
            try:
                original.symlink_to(moved)
            except OSError:
                self.skipTest('symlink creation requires platform privileges')
            with self.assertRaises(ValueError):
                g.run(output, check=True)


if __name__ == '__main__':
    unittest.main()
