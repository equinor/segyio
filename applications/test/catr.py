import unittest
import subprocess

class appTest(unittest.TestCase):
    def setUp(self):
        self.file_path = "../python/test-data/small.sgy"
        self.cmd_base = ".././applications/segyio-catr"
        self.ref_list = [
                    b'tracl\t0',
                    b'tracr\t0',
                    b'fldr\t0',
                    b'tracf\t0',
                    b'ep\t0',
                    b'cdp\t0',
                    b'cdpt\t0',
                    b'trid\t0',
                    b'nvs\t0',
                    b'nhs\t0',
                    b'duse\t0',
                    b'offset\t1',
                    b'gelev\t0',
                    b'selev\t0',
                    b'sdepth\t0',
                    b'gdel\t0',
                    b'sdel\t0',
                    b'swdep\t0',
                    b'gwdep\t0',
                    b'scalel\t0',
                    b'scalco\t0',
                    b'sx\t0',
                    b'sy\t0',
                    b'gx\t0',
                    b'gy\t0',
                    b'counit\t0',
                    b'wevel\t0',
                    b'swevel\t0',
                    b'sut\t0',
                    b'gut\t0',
                    b'sstat\t0',
                    b'gstat\t0',
                    b'tstat\t0',
                    b'laga\t0',
                    b'lagb\t0',
                    b'delrt\t0',
                    b'muts\t0',
                    b'mute\t0',
                    b'ns\t0',
                    b'dt\t0',
                    b'gain\t0',
                    b'igc\t0',
                    b'igi\t0',
                    b'corr\t0',
                    b'sfs\t0',
                    b'sfe\t0',
                    b'slen\t0',
                    b'styp\t0',
                    b'stat\t0',
                    b'stae\t0',
                    b'tatyp\t0',
                    b'afilf\t0',
                    b'afils\t0',
                    b'nofilf\t0',
                    b'nofils\t0',
                    b'lcf\t0',
                    b'hcf\t0',
                    b'lcs\t0',
                    b'hcs\t0',
                    b'year\t0',
                    b'day\t0',
                    b'hour\t0',
                    b'minute\t0',
                    b'sec\t0',
                    b'timbas\t0',
                    b'trwf\t0',
                    b'grnors\t0',
                    b'grnofr\t0',
                    b'grnlof\t0',
                    b'gaps\t0',
                    b'otrav\t0',
                    b'cdpx\t0',
                    b'cdpy\t0',
                    b'iline\t2',
                    b'xline\t20',
                    b'sp\t0',
                    b'scalsp\t0',
                    b'trunit\t0',
                    b'tdcm\t0',
                    b'tdcp\t0',
                    b'tdunit\t0',
                    b'triden\t0',
                    b'sctrh\t0',
                    b'stype\t0',
                    b'sedm\t0',
                    b'sede\t0',
                    b'smm\t0',
                    b'sme\t0',
                    b'smunit\t0',
                    b'uint1\t0',
                    b'uint2\t0']

    def test_segycatr_output_t(self):
        cmd = [self.cmd_base, self.file_path, "-t 6"]
        res = subprocess.Popen(cmd, stdout=subprocess.PIPE).communicate()[0]
        res_list = res.split(b"\n")
        for i in range(len(self.ref_list)):
            self.assertEquals(self.ref_list[i], res_list[i])

    def test_segycatr_output_r1(self):
        cmd = [self.cmd_base, self.file_path, "-r 6"]
        res = subprocess.Popen(cmd, stdout=subprocess.PIPE).communicate()[0]
        res_list = res.split(b"\n")
        for i in range(len(self.ref_list)):
            self.assertEquals(self.ref_list[i], res_list[i])

    def test_segycatr_output_r2(self):
        cmd = [self.cmd_base, self.file_path, "-r 1 6"]
        res = subprocess.Popen(cmd, stdout=subprocess.PIPE).communicate()[0]
        res_list = res.split(b"\n")
        for i in range(len(self.ref_list)):
            self.assertEquals(self.ref_list[i], res_list[i+455])

    def test_segycatr_output_r3(self):
        cmd = [self.cmd_base, self.file_path, "-r 5 7 1"]
        res = subprocess.Popen(cmd, stdout=subprocess.PIPE).communicate()[0]
        res_list = res.split(b"\n")
        for i in range(len(self.ref_list)):
            self.assertEquals(self.ref_list[i], res_list[i+91])

    def test_segycatr_output_r_multiple(self):
        cmd = [self.cmd_base, self.file_path, "-r 1 2", "-r 6 7"]
        res = subprocess.Popen(cmd, stdout=subprocess.PIPE).communicate()[0]
        res_list = res.split(b"\n")
        for i in range(len(self.ref_list)):
            self.assertEquals(self.ref_list[i], res_list[i+182])
            
    def test_segycatr_output_r_t(self):
        cmd = [self.cmd_base, self.file_path, "-r 4 9", "-t 12"]
        res = subprocess.Popen(cmd, stdout=subprocess.PIPE).communicate()[0]
        res_list = res.split(b"\n")
        for i in range(len(self.ref_list)):
            self.assertEquals(self.ref_list[i], res_list[i+182])

if __name__ == '__main__':
    unittest.main()
