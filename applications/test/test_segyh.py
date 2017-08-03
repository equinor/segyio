import unittest
import subprocess

class TestSegyH(unittest.TestCase):

    work_dir = '../../build'
    cmd_base = './applications/segyio-cath'

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

    def test_help(self):
        cmd = self.cmd_base + ' --help'
        result = subprocess.check_output(cmd, shell=True, cwd=self.work_dir)
        self.assertEqual(result[0:18], b'Usage: segyio-cath')

    def test_version(self):
        cmd = self.cmd_base + ' --version'
        result = subprocess.check_output(cmd, shell=True, cwd=self.work_dir)
        self.assertEqual(result[-5:-2], b'1.2')

    def test_segy_in(self):
        cmd = self.cmd_base + ' python/test-data/small.sgy'
        result = subprocess.check_output(cmd, shell=True,
                 cwd=self.work_dir).split(b'\n')
        for i in range(len(self.actual)):
            self.assertEqual(self.actual[i].strip(), b''+result[i + 1].strip())

    def test_none_segy_in(self):
        cmd = self.cmd_base + ' ../CMakeLists.txt'
        result = subprocess.check_output(cmd, shell=True,
                 stderr=subprocess.STDOUT, cwd=self.work_dir)
        self.assertEqual(result, b'Unable to read binary header\n')

    def test_none_file(self):
        cmd = self.cmd_base + ' nothing'
        result = subprocess.check_output(cmd, shell=True,
                 stderr=subprocess.STDOUT, cwd=self.work_dir)
        self.assertEqual(result[22:],b'No such file or directory\n')

if __name__ == '__main__':
    unittest.main()
