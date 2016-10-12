from unittest.case import TestCase

from segyio import Enum


class TestEnum(Enum):
    ZERO = 0
    ONE = 1
    TWO = 2


class EnumTest(TestCase):

    def test_enum(self):
        self.assertEqual(TestEnum.ONE, 1)

        one = TestEnum(TestEnum.ONE)
        self.assertIsInstance(one, TestEnum)
        self.assertEqual(str(one), "ONE")
        self.assertEqual(int(one), 1)
        self.assertEqual(one, 1)

        self.assertListEqual(TestEnum.enums(), TestEnum.enums())
        self.assertListEqual(TestEnum.enums(), [0, 1, 2])
