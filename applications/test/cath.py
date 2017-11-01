import unittest
import subprocess
import os

class TestSegyH(unittest.TestCase):

    actual = [
        b"C 2 AN INCREASE IN AMPLITUDE EQUALS AN INCREASE IN ACOUSTIC IMPEDANCE",
        b"C 3 Written by libsegyio (python)",
        b"C 4",
        b"C 5",
        b"C 6",
        b"C 7",
        b"C 8",
        b"C 9",
        b"C10",
        b"C11 TRACE HEADER POSITION:",
        b"C12   INLINE BYTES 189-193    | OFFSET BYTES 037-041",
        b"C13   CROSSLINE BYTES 193-197 |",
        b"C14",
        b"C15 END EBCDIC HEADER",
    ]
    work_dir = '../'
    cmd_base = './applications/segyio-cath'
    FNULL = open(os.devnull, 'w')

    def test_segy_in(self):
        cmd = self.cmd_base + ' python/test-data/small.sgy'
        result = subprocess.check_output(cmd, shell=True,
                 cwd=self.work_dir).split(b'\n')
        for i in range(len(self.actual)):
            self.assertEqual(self.actual[i].strip(), b''+result[i + 1].strip())

    def test_none_segy_in(self):
        cmd = self.cmd_base + ' ../CMakeLists.txt'
        result = subprocess.call(cmd, shell=True,
                 stderr=self.FNULL, cwd=self.work_dir)
        self.assertEqual(result, 0)

if __name__ == '__main__':
    unittest.main()
