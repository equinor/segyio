import unittest
import subprocess
import os

class TestSegyB(unittest.TestCase):

    actual_output = [
        b"jobid\t0",
        b"lino\t0",
        b"reno\t0",
        b"ntrpr\t25",
        b"nart\t0",
        b"hdt\t4000",
        b"dto\t0",
        b"hns\t50",
        b"nso\t0",
        b"format\t1",
        b"fold\t0",
        b"tsort\t0",
        b"vscode\t0",
        b"hsfs\t0",
        b"hsfe\t0",
        b"hslen\t0",
        b"hstyp\t0",
        b"schn\t0",
        b"hstas\t0",
        b"hstae\t0",
        b"htatyp\t0",
        b"hcorr\t0",
        b"bgrcv\t0",
        b"rcvm\t0",
        b"mfeet\t0",
        b"polyt\t0",
        b"vpol\t0",
        b"rev\t0",
        b"trflag\t0",
        b"exth\t0"
    ]
    work_dir = '../'
    cmd_base = './applications/segyio-catb'
    FNULL = open(os.devnull, 'w')

    def test_segy_in(self):
        filepath = self.work_dir + '/python/test-data/small.sgy'
        cmd = self.cmd_base + ' python/test-data/small.sgy'
        result = subprocess.check_output(cmd, shell=True, cwd=self.work_dir)
        result_list = result.split(b'\n')
        for i in range(len(self.actual_output)):
            self.assertEqual(result_list[i], self.actual_output[i])

if __name__ == '__main__':
    unittest.main()

