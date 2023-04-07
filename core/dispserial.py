# set up pyserial on /dev/ttyACM0

import serial
import time
from struct import Struct

DISPLAY_SIZE = (240, 240)
BUFSIZE = 16 * 1024

SERIAL = serial.Serial("/dev/ttyACM0", 921600, timeout=1)

# one byte read/write, one byte command, two bytes little-endian length
INITIAL_PACKET = Struct("<BBH")


# fmt: off
ONE = [
    0, 0, 1, 0, 0, 0,
    0, 1, 1, 0, 0, 0,
    0, 0, 1, 0, 0, 0,
    0, 0, 1, 0, 0, 0,
    0, 0, 1, 0, 0, 0,
    1, 1, 1, 1, 1, 1
]

TWO = [
    1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 1, 0,
    0, 0, 0, 1, 0, 0,
    0, 0, 1, 0, 0, 0,
    0, 1, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1
]

THREE = [
    0, 1, 1, 1, 0, 0,
    1, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 1, 0,
    1, 0, 0, 0, 1, 0,
    0, 1, 1, 1, 0, 0
]

FOUR = [
    0, 0, 0, 1, 0, 0,
    0, 0, 1, 1, 0, 0,
    0, 1, 0, 1, 0, 0,
    1, 0, 0, 1, 0, 0,
    1, 1, 1, 1, 1, 1,
    0, 0, 0, 1, 0, 0
]
# fmt: on


def cmd_write(cmd: int, data: bytes) -> None:
    # write a command to the display
    # cmd: command byte
    # data: data bytes
    assert len(data) <= BUFSIZE
    initial_packet = INITIAL_PACKET.pack(0x00, cmd, len(data))
    SERIAL.write(initial_packet + data)


def cmd_read(cmd: int, length: int) -> bytes:
    # read a command from the display
    # cmd: command byte
    # length: number of bytes to read
    assert length <= BUFSIZE
    initial_packet = INITIAL_PACKET.pack(0x01, cmd, length)
    SERIAL.write(initial_packet)
    return SERIAL.read(length)


def set_window(x0, y0, x1, y1):
    # CASET 0x2A
    cmd_write(0x2A, x0.to_bytes(2, "big") + x1.to_bytes(2, "big"))
    # RASET 0x2B
    cmd_write(0x2B, y0.to_bytes(2, "big") + y1.to_bytes(2, "big"))


def pixels(data):
    # RAMWR 0x2C
    cmd_write(0x2C, b"")
    # split data into chunks of BUFSIZE max
    for i in range(0, len(data), BUFSIZE):
        cmd_write(0, data[i : i + BUFSIZE])


def render_num(number: list[int]) -> None:
    buf = bytearray()
    for n in number:
        buf.extend(b"\x00\x00" if not n else b"\xFF\xFF")

    set_window(200, 200, 205, 205)
    pixels(buf)


def _intensity(i: float) -> int:
    """Convert intensity to 6-bit value."""
    return int(i * 63)


def rgb565(r: float, g: float, b: float) -> bytes:
    """Convert RGB to RGB565."""
    return (
        ((_intensity(r) >> 1) << 11) | (_intensity(g) << 5) | (_intensity(b) >> 1)
    ).to_bytes(2, "little")


def rgb565i(r: int, g: int, b: int) -> bytes:
    """Convert RGB to RGB565."""
    r &= 0b11111
    g &= 0b111111
    b &= 0b11111
    return ((r << 11) | (g << 5) | b).to_bytes(2, "little")


def image_sampler() -> bytes:
    """Generate a 240x240 image sampler of gray, red, blue and green gradients.
    The resulting buffer is RGB565, two bytes per pixel.
    """
    result = bytearray()
    gray_line = bytearray()
    red_line = bytearray()
    green_line = bytearray()
    blue_line = bytearray()

    for x in range(24):
        intensity = x * 32 // 24
        gray_line.extend(rgb565i(intensity, intensity * 2, intensity) * 10)
        red_line.extend(rgb565i(intensity, 0, 0) * 10)
        green_line.extend(rgb565i(0, intensity * 2, 0) * 10)
        blue_line.extend(rgb565i(0, 0, intensity) * 10)

    result.extend(gray_line * 60)
    result.extend(red_line * 60)
    result.extend(green_line * 60)
    result.extend(blue_line * 60)

    return bytes(result)


if __name__ == "__main__":
    # # # set up display
    # cmd_write(0x11, b"")  # SLPOUT 0x11
    # time.sleep(0.1)
    # cmd_write(0x36, b"\x00")  # MADCTL 0x36
    # cmd_write(0x3A, b"\x05")  # COLMOD 0x3A
    # cmd_write(0x29, b"")  # DISPON 0x29
    set_window(0, 0, *DISPLAY_SIZE)
    pixels(image_sampler())

    while True:
        # set gamma curve 1
        cmd_write(0x26, b"\x01")
        render_num(ONE)
        input()
        # set gamma curve 2
        cmd_write(0x26, b"\x02")
        render_num(TWO)
        input()
        # set gamma curve 3
        cmd_write(0x26, b"\x04")
        render_num(THREE)
        input()
        # set gamma curve 4
        cmd_write(0x26, b"\x08")
        render_num(FOUR)
        input()

    # write some pixels
    # # read some pixels
    # set_window(0, 0, 10, 10)
    # print(cmd_read(0x2E, 11 * 11 * 2))

    # # clean up
    # cmd_write(0x10, b"")  # SLPIN 0x10
    # time.sleep(0.1)
    # SERIAL.close()
