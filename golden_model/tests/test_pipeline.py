"""Drive the model through its public C API, as a Python testbench would."""

import ctypes as c
from pathlib import Path
import sys
import tempfile
import unittest


class Result(c.Structure):
    """The output registers exposed by the C ABI."""

    _fields_ = [("valid", c.c_bool), ("port", c.c_uint8)]
    valid: bool
    port: int


def open_library(path: Path) -> c.CDLL:
    """Bind the model API with its C argument and result types."""
    lib = c.CDLL(str(path.resolve()))
    lib.lulea_model_create.argtypes = [c.c_char_p, c.POINTER(c.c_char), c.c_size_t]
    lib.lulea_model_create.restype = c.c_void_p
    lib.lulea_model_destroy.argtypes = [c.c_void_p]
    lib.lulea_model_destroy.restype = None
    lib.lulea_model_reset.argtypes = [c.c_void_p]
    lib.lulea_model_reset.restype = None
    lib.lulea_model_tick.argtypes = [c.c_void_p, c.c_bool, c.c_uint32]
    lib.lulea_model_tick.restype = Result
    return lib


def write_bank(path: Path, width: int, cells: list[int]) -> None:
    """Write packed LSB-first entries with zero padding in the last byte."""
    bits = sum(cell << (index * width) for index, cell in enumerate(cells))
    path.write_bytes(bits.to_bytes((len(cells) * width + 7) // 8, "little"))


def write_level(directory: Path, level: int, chunks: list[int], seq: list[int]) -> None:
    """Write a level and derive the running count before each chunk."""
    sums: list[int] = []
    count = 0
    for chunk in chunks:
        sums.append(count)
        count += chunk.bit_count()
    write_bank(directory / f"l{level}_pop.bin", 64, chunks)
    write_bank(directory / f"l{level}_chunk.bin", 18, sums)
    write_bank(directory / f"l{level}_seq.bin", 18 if level == 1 else 9, seq)


class PipelineTest(unittest.TestCase):
    """Check the output of every edge against the fixed-latency interface."""

    lib: c.CDLL
    fixture_root: Path

    def setUp(self) -> None:
        self.images = self.fixture_root / self.id()
        self.images.mkdir()
        chunks = [0] * 4096
        chunks[0] = (1 << 1) | (1 << 63)
        chunks[-1] = 1 << 63
        write_level(self.images, 1, chunks, [0, (1 << 17) | 1, 255, 29])
        write_level(self.images, 2, [0, 1 | (1 << 62) | (1 << 63)], [4, 11, (1 << 8) | 1, 12])
        write_level(self.images, 3, [0, 0, 0, 0, 1 << 63, 1, 1 << 31, 1 << 63],
                    [21, 22, 23, 24, 25])

    def _create_model(self) -> int:
        error = c.create_string_buffer(256)
        model: int | None = self.lib.lulea_model_create(bytes(self.images), error, len(error))
        if model is None:
            self.fail(error.value.decode())
        self.addCleanup(self.lib.lulea_model_destroy, model)
        return model

    def test_mixed_paths_and_bubbles(self) -> None:
        """Keep L1, L2 and L3 answers in input order at one request per edge."""
        model = self._create_model()
        l2 = 1 << 14
        l3 = l2 | (62 << 8)
        requests: list[tuple[int, int | None]] = [
            (0, 0), (l2, 11), (l3, 21), (0, None), (l3 | 63, 22),
            (l3 | 64, 23), (l3 | 128, 23), (l3 | 159, 24), (l3 | 254, 24),
            (l3 | 255, 25), (l2 | (63 << 8), 12), (63 << 14, 255), (0xffffffff, 29),
        ]
        for edge, (ip, port) in enumerate(requests + [(0, None)] * 5):
            result = self.lib.lulea_model_tick(model, port is not None, ip)
            expected = requests[edge - 5][1] if edge >= 5 else None
            self.assertEqual(result.valid, expected is not None, f"edge {edge + 1}")
            if expected is not None:
                self.assertEqual(result.port, expected, f"edge {edge + 1}")

    def test_reset_and_independent_instances(self) -> None:
        """Drop in-flight requests only in the instance being reset."""
        first = self._create_model()
        second = self._create_model()
        self.assertFalse(self.lib.lulea_model_tick(first, True, 0).valid)
        self.assertFalse(self.lib.lulea_model_tick(second, True, 0xffffffff).valid)
        self.lib.lulea_model_reset(first)
        for _ in range(4):
            self.assertFalse(self.lib.lulea_model_tick(second, False, 0).valid)
        result = self.lib.lulea_model_tick(second, False, 0)
        self.assertTrue(result.valid)
        self.assertEqual(result.port, 29)
        for edge in range(6):
            result = self.lib.lulea_model_tick(first, edge == 0, 63 << 14)
            self.assertEqual(result.valid, edge == 5)
        self.assertEqual(result.port, 255)

    def test_empty_lower_levels(self) -> None:
        """Allow unused lower-level banks in cache_gen output."""
        write_level(self.images, 1, [0] * 4096, [17])
        write_level(self.images, 2, [], [])
        write_level(self.images, 3, [], [])
        model = self._create_model()
        for edge in range(6):
            result = self.lib.lulea_model_tick(model, edge == 0, 0xffffffff)
            self.assertEqual(result.valid, edge == 5)
        self.assertEqual(result.port, 17)

    def test_rejects_oversized_bank(self) -> None:
        """Reject an L3 bitmap that exceeds the selected allocation."""
        (self.images / "l3_pop.bin").write_bytes(bytes(513 * 8))
        error = c.create_string_buffer(256)
        self.assertIsNone(self.lib.lulea_model_create(bytes(self.images), error, len(error)))
        self.assertIn(b"l3_pop.bin", error.value)

    def test_rejects_invalid_pointer(self) -> None:
        """Reject a sequence entry pointing outside the loaded bank."""
        write_bank(self.images / "l1_seq.bin", 18, [0, (1 << 17) | 2, 255, 29])
        error = c.create_string_buffer(256)
        self.assertIsNone(self.lib.lulea_model_create(bytes(self.images), error, len(error)))
        self.assertIn(b"Inconsistent cache images", error.value)


if __name__ == "__main__":
    PipelineTest.lib = open_library(Path(sys.argv.pop(1)))
    with tempfile.TemporaryDirectory() as fixture_dir:
        PipelineTest.fixture_root = Path(fixture_dir)
        unittest.main()
