"""Generate an original, sample-free dialogue-advance cue. No network or TTS."""
from __future__ import annotations
import argparse
import hashlib
import io
import json
import struct
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PIN = ROOT / "Content/Starter/Audio/UI/expected-manifest.json"
NAME = "dialogue_advance_original_v1.wav"
RATE = 48000
FRAMES = 5280  # 110 ms, including a short quiet tail.


def triangle(phase: int) -> int:
    phase %= RATE
    return (4 * phase * 32767 // RATE - 32767) if phase < RATE // 2 else (3 * 32767 - 4 * phase * 32767 // RATE)


def cue_bytes() -> bytes:
    samples = []
    for frame in range(FRAMES):
        value = 0
        # Newly authored pitches/envelopes. No game recording was used.
        for start, frequency, length, gain in ((0, 1320, 3360, 6200), (1200, 1760, 3360, 4400)):
            age = frame - start
            if 0 <= age < length:
                attack = min(age, 144)
                remaining = length - 1 - age
                envelope = attack * remaining * remaining
                denominator = 144 * (length - 1) * (length - 1)
                value += triangle(age * frequency) * gain * envelope // (32767 * denominator)
        samples.append(value)
    samples[0] = samples[-1] = 0
    if max(abs(x) for x in samples) > 11000:
        raise ValueError("unexpected peak; do not silently normalize")
    stream = io.BytesIO()
    with wave.open(stream, "wb") as wav:
        wav.setparams((1, 2, RATE, FRAMES, "NONE", "not compressed"))
        wav.writeframes(struct.pack(f"<{FRAMES}h", *samples))
    return stream.getvalue()


def manifest(payload: bytes) -> dict:
    return {"schema_version": 1, "event": "ui.dialogue.advance", "file": NAME,
            "status": "generated_source_not_runtime_integrated", "origin": "original_procedural_no_external_samples",
            "generator_version": "astral-dialogue-cue-1", "sample_rate_hz": RATE,
            "channels": 1, "bits_per_sample": 16, "frame_count": FRAMES,
            "byte_count": len(payload), "sha256": hashlib.sha256(payload).hexdigest()}


def run(output: Path, check: bool = False) -> None:
    output = output.resolve()
    if output == ROOT or ROOT in output.parents:
        raise ValueError("output must be outside the source worktree")
    payload = cue_bytes()
    expected = json.dumps(manifest(payload), indent=2) + "\n"
    # Canonical LF JSON pin; safe in Windows CRLF checkouts.
    if PIN.read_text(encoding="utf-8") != expected:
        raise ValueError("checked-in manifest mismatch; review before repinning")
    files = {NAME: payload, "manifest.json": expected.encode("utf-8")}
    if check:
        for name, data in files.items():
            if (output / name).is_symlink() or (output / name).read_bytes() != data:
                raise ValueError(f"generated artifact mismatch: {name}")
    else:
        # A fresh directory avoids overwriting any previous evidence or asset.
        output.mkdir(parents=True, exist_ok=False)
        for name, data in files.items():
            with (output / name).open("xb") as f:
                f.write(data)
    print(f"PASS: original 110 ms advance cue; source-only ({'checked' if check else 'generated'})")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    try:
        run(args.output, args.check)
    except (OSError, ValueError) as exc:
        parser.exit(1, f"FAIL: {exc}\n")


if __name__ == "__main__":
    main()
