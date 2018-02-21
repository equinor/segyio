from segyio import Enum

def test_enum():
    class TestEnum(Enum):
        ZERO = 0
        ONE = 1
        TWO = 2

    assert TestEnum.ONE == 1

    one = TestEnum(TestEnum.ONE)
    assert isinstance(one, TestEnum)
    assert str(one) == "ONE"
    assert int(one) == 1
    assert one == 1

    assert TestEnum.enums() == TestEnum.enums()
    assert TestEnum.enums() == [0, 1, 2]
