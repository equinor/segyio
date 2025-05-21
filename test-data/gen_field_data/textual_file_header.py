import ebcdic  # type: ignore[import-untyped]


class TextualFileHeader:
    TEXTUAL_FILE_HEADER_SIZE = 3200

    def __init__(self, data: bytes | None = None):
        if data is None:
            data = bytearray(TextualFileHeader.TEXTUAL_FILE_HEADER_SIZE)

        assert isinstance(data, (bytes, bytearray)), "Data must be bytes or bytearray"
        assert len(data) == TextualFileHeader.TEXTUAL_FILE_HEADER_SIZE
        self.data = data
        # Decode the data using cp1140 encoding
        # 1140	Australia, Brazil, Canada, New Zealand, Portugal, South Africa, USA
        # Full list https://en.wikipedia.org/wiki/EBCDIC
        self.text = data.decode("cp1140")

        self.lines = []
        for i in range(int(TextualFileHeader.TEXTUAL_FILE_HEADER_SIZE / 80)):
            self.lines.append(self.text[i * 80 : (i + 1) * 80])

    def print_lines(self):
        for line in self.lines:
            print(line)
        print()

    def __str__(self):
        out_data = (
            f"\nTextual File Header [{TextualFileHeader.TEXTUAL_FILE_HEADER_SIZE}]:\n"
        )
        for i, line in enumerate(self.lines):
            line_out = ""
            for char in line:
                if ord(char) <= 32:
                    line_out += " "
                else:
                    line_out += char

            out_data += f"[{i:04d} :{line_out}]\n"
        return out_data
