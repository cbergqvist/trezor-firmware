from pathlib import Path

import tkinter as tk

import serial
import construct as c

DISPLAY_SIZE = (240, 240)
BUFSIZE = 16 * 1024

SERIAL = serial.Serial("/dev/ttyACM0", 921600, timeout=1)

# one byte read/write, one byte command, two bytes little-endian length
Command = c.Struct(
    "read" / c.Byte,
    "cmd" / c.Byte,
    "length" / c.Int16ul,
)

DispStatus = c.BitStruct(
    c.Padding(8),
    "booster" / c.Bit,
    "my" / c.Bit,
    "mx" / c.Bit,
    "mv" / c.Bit,
    "ml" / c.Bit,
    "rgb" / c.Bit,
    "mh" / c.Bit,
    c.Padding(1),
    c.Padding(1),
    "ifpf" / c.BitsInteger(3),
    "idle" / c.Bit,
    "partial" / c.Bit,
    "slout" / c.Bit,
    "normal" / c.Bit,
    "vscroll" / c.Bit,
    "hscroll" / c.Bit,
    "inverted" / c.Bit,
    c.Padding(2),
    "dison" / c.Bit,
    "teon" / c.Bit,
    "gamma" / c.BitsInteger(3),
    "tem" / c.Bit,
    c.Padding(5),
    c.Terminated,
)


GammaVoltage = c.BitStruct(
    "v63" / c.BitsInteger(4),
    "v0" / c.BitsInteger(4),
    c.Padding(2),
    "v1" / c.BitsInteger(6),
    c.Padding(2),
    "v2" / c.BitsInteger(6),
    c.Padding(3),
    "v4" / c.BitsInteger(5),
    c.Padding(3),
    "v6" / c.BitsInteger(5),
    c.Padding(2),
    "j0" / c.BitsInteger(2),
    "v13" / c.BitsInteger(4),
    c.Padding(1),
    "v20" / c.BitsInteger(7),
    c.Padding(1),
    "v36" / c.BitsInteger(3),
    c.Padding(1),
    "v27" / c.BitsInteger(3),
    c.Padding(1),
    "v43" / c.BitsInteger(7),
    c.Padding(2),
    "j1" / c.BitsInteger(2),
    "v50" / c.BitsInteger(4),
    c.Padding(3),
    "v57" / c.BitsInteger(5),
    c.Padding(3),
    "v59" / c.BitsInteger(5),
    c.Padding(2),
    "v61" / c.BitsInteger(6),
    c.Padding(2),
    "v62" / c.BitsInteger(6),
    c.Terminated,
)


def cmd_write(cmd: int, data: bytes) -> None:
    # write a command to the display
    # cmd: command byte
    # data: data bytes
    assert len(data) <= BUFSIZE
    initial_packet = Command.build(dict(read=0x00, cmd=cmd, length=len(data)))
    SERIAL.write(initial_packet + data)


def cmd_read(cmd: int, length: int) -> bytes:
    # read a command from the display
    # cmd: command byte
    # length: number of bytes to read
    assert length <= BUFSIZE
    initial_packet = Command.build(dict(read=0x01, cmd=cmd, length=length))
    SERIAL.write(initial_packet)
    return SERIAL.read(length)


def set_window(x0, y0, x1, y1):
    # CASET 0x2A
    cmd_write(0x2A, x0.to_bytes(2, "big") + (x1 - 1).to_bytes(2, "big"))
    # RASET 0x2B
    cmd_write(0x2B, y0.to_bytes(2, "big") + (y1 - 1).to_bytes(2, "big"))


def pixels(data):
    # RAMWR 0x2C
    cmd_write(0x2C, b"")
    # split data into chunks of BUFSIZE max
    for i in range(0, len(data), BUFSIZE):
        cmd_write(0, data[i : i + BUFSIZE])


def rgb565i(r: int, g: int, b: int) -> bytes:
    """Convert RGB to RGB565."""
    r &= 0b11111
    g &= 0b111111
    b &= 0b11111
    return ((r << 11) | (g << 5) | b).to_bytes(2, "little")


def fill(color: bytes):
    """Fill the display with a single color."""
    set_window(0, 0, *DISPLAY_SIZE)
    pixels(color * (240 * 240))


def make_gradient() -> bytes:
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


def make_inverse_gradient() -> bytes:
    result = bytearray()
    gray_line = bytearray()
    red_line = bytearray()
    green_line = bytearray()
    blue_line = bytearray()

    for x in range(24):
        intensity = x * 32 // 24
        gray_line.extend(
            rgb565i(31 - intensity, 63 - intensity * 2, 31 - intensity) * 10
        )
        red_line.extend(rgb565i(31 - intensity, 63, 31) * 10)
        green_line.extend(rgb565i(31, 63 - intensity * 2, 31) * 10)
        blue_line.extend(rgb565i(31, 63, 31 - intensity) * 10)

    result.extend(gray_line * 60)
    result.extend(red_line * 60)
    result.extend(green_line * 60)
    result.extend(blue_line * 60)

    return bytes(result)


class ScaleEntryWidget(tk.Frame):
    def __init__(self, master, label, min_value, max_value, variable):
        super().__init__(master)

        self.min_value = min_value
        self.max_value = max_value
        self.variable = variable
        self.label = tk.Label(self, text=label)
        self.label.grid(row=0, column=0)

        self.scale = tk.Scale(
            self,
            from_=min_value,
            to=max_value,
            orient=tk.HORIZONTAL,
            variable=variable,
            command=self.update_entry,
        )
        self.scale.grid(row=0, column=1)

        self.entry_var = tk.StringVar()
        self.entry_var.trace("w", self.update_scale)
        self.entry = tk.Entry(
            self,
            width=4,
            textvariable=self.entry_var,
            validate="key",
            validatecommand=(self.register(self.validate_entry), "%P"),
        )
        self.entry.insert(0, self.variable.get())
        self.entry.grid(row=0, column=2)

    def validate_entry(self, value):
        try:
            value = int(value)
        except ValueError:
            value = 0

        return self.min_value <= value <= self.max_value

    def update_entry(self, *args):
        self.entry_var.set(self.variable.get())

    def update_scale(self, *args):
        try:
            value = int(self.entry_var.get())
        except ValueError:
            pass
        else:
            self.scale.set(value)


class RGBWidget(tk.LabelFrame):
    def __init__(self, master=None, **kwargs):
        super().__init__(master, text="RGB Widget", **kwargs)

        # Create IntVars to hold the red, green, and blue values
        self.red_var = tk.IntVar()
        self.red_var.trace_add("write", self.update_color)
        self.green_var = tk.IntVar()
        self.green_var.trace_add("write", self.update_color)
        self.blue_var = tk.IntVar()
        self.blue_var.trace_add("write", self.update_color)

        # Create ScaleEntry widgets to set the red, green, and blue values
        self.red_widget = ScaleEntryWidget(self, "Red", 0, 255, self.red_var)
        self.red_widget.grid(row=0, column=0, padx=5, pady=5)
        self.green_widget = ScaleEntryWidget(self, "Green", 0, 255, self.green_var)
        self.green_widget.grid(row=1, column=0, padx=5, pady=5)
        self.blue_widget = ScaleEntryWidget(self, "Blue", 0, 255, self.blue_var)
        self.blue_widget.grid(row=2, column=0, padx=5, pady=5)

        # Create a canvas to show the resulting color
        self.color_canvas = tk.Canvas(self, width=100, height=100, bg="black")
        self.color_canvas.grid(row=0, column=1, rowspan=3, padx=5, pady=5)

        self.color_canvas.create_text(50, 50, text="render", fill="white")
        self.color_canvas.bind("<Button-1>", self.on_click)

    def update_color(self, *args):
        # This function is called whenever any of the ScaleEntry widgets is updated
        red = self.red_var.get()
        green = self.green_var.get()
        blue = self.blue_var.get()

        # Update the color of the canvas
        color = "#{:02X}{:02X}{:02X}".format(red, green, blue)
        self.color_canvas.configure(bg=color)

    def on_click(self, *args):
        red = self.red_var.get()
        green = self.green_var.get()
        blue = self.blue_var.get()
        color_pixels = rgb565i(red >> 3, green >> 2, blue >> 3)
        fill(color_pixels)


class BoundedIntVar(tk.IntVar):
    def __init__(self, min: int, max: int) -> None:
        super().__init__()
        self.min = min
        self.max = max

    def set(self, value: int) -> None:
        super().set(max(self.min, min(self.max, value)))


class EditBoxComponent(tk.Frame):
    def __init__(self, master, label, variable: BoundedIntVar):
        super().__init__(master)
        self.variable = variable
        assert self.variable.min == 0
        self.label = tk.Label(self, text=label)
        self.label.pack(side="bottom")
        self.edit_box = tk.Entry(self, width=5)
        self.edit_box.pack(side="bottom")
        self.bar = tk.Canvas(self, width=10, height=63)
        self.bar.pack(side="top")

        self.edit_box.bind("<KeyRelease>", self.update_editbox)
        self.variable.trace_add("write", self.on_set)

    def on_set(self, *args):
        self.set_value()

    def set_value(self):
        value = self.variable.get()
        self.edit_box.delete(0, tk.END)
        self.edit_box.insert(0, str(value))
        self.bar.delete("all")
        value_scaled = int(value * 63 / self.variable.max)
        self.bar.create_rectangle(0, 63 - value_scaled, 10, 63, fill="blue")

    def update_editbox(self, event):
        # Update bar based on current value in edit box
        try:
            value = int(self.edit_box.get())
        except ValueError:
            value = 0

        # If up arrow key is pressed, increase value by 1
        if event.keysym == "Up":
            value += 1
        # If down arrow key is pressed, decrease value by 1
        elif event.keysym == "Down":
            value -= 1

        self.variable.set(value)


class GammaRow(tk.Frame):
    def __init__(self, master, positive: bool):
        super().__init__(master)
        self.variables = {}
        self.update_display = False
        self.update_callback = lambda: None
        if positive:
            self.letter = "P"
            self.cmd = 0xE0  # PVGAMCTRL
        else:
            self.letter = "N"
            self.cmd = 0xE1  # NVGAMCTRL

        self.stored = Path(f"gamma-{self.letter}.bin")

        def subconkey(name):
            return name[0], int(name[1:])

        for name in sorted(GammaVoltage.subcon._subcons, key=subconkey):
            # Create edit box component
            field = GammaVoltage.subcon._subcons[name]
            variable = BoundedIntVar(0, 2**field.length - 1)
            variable.trace_add("write", self.my_on_update)
            self.variables[name] = variable

            ltr, num = subconkey(name)
            label = f"{ltr}{self.letter}{num}".upper()
            vp = EditBoxComponent(self, label, variable)
            vp.pack(side="left")

        if self.stored.exists():
            data = self.stored.read_bytes()
            valdict = GammaVoltage.parse(data)
            for name, var in self.variables.items():
                var.set(valdict[name])

    def my_on_update(self, *args):
        if self.update_display:
            # Update gamma curve
            valdict = {name: var.get() for name, var in self.variables.items()}
            data = GammaVoltage.build(valdict)
            self.stored.write_bytes(data)
            print(f"cmd {self.cmd:02X} data {data.hex()}")
            cmd_write(self.cmd, data)


class EditBoxGUI:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Edit Box GUI")
        self.edit_box_components = []

        self.inverted = tk.BooleanVar()
        self.inverted.trace("w", self.set_inversion)
        self.gamma_curve = tk.Variable()
        self.gamma_curve.trace("w", self.set_gamma)

        # Create a frame for the first row of edit box components
        self.vp_frame = GammaRow(self.root, True)
        self.vp_frame.grid(row=0, column=0)

        # Create a frame for the second row of edit box components
        self.vn_frame = GammaRow(self.root, False)
        self.vn_frame.grid(row=1, column=0)

        self.row3_frame = tk.Frame(self.root)
        self.row3_frame.grid(row=2, column=0)

        patterns = tk.LabelFrame(self.row3_frame, text="Patterns")
        patterns.pack(side="left")
        # base gradient button
        base_gradient = tk.Button(
            patterns, text="Base Gradient", command=self.base_gradient
        )
        base_gradient.pack(side="top")
        # inverted gradient button
        inverted_gradient = tk.Button(
            patterns, text="Inverted Gradient", command=self.inverted_gradient
        )
        inverted_gradient.pack(side="top")

        rgb = RGBWidget(self.row3_frame)
        rgb.pack(side="left")

        gammactrl = tk.LabelFrame(self.row3_frame, text="Gamma Curve")
        gammactrl.pack()

        inversion = tk.Checkbutton(gammactrl, text="Inverted", variable=self.inverted)
        inversion.grid(row=0, column=0, sticky="w")

        for i, label in enumerate(("2.2", "1.8", "2.5", "1.0")):
            curve = tk.Radiobutton(
                gammactrl,
                text=f"Gamma {label} (GC{i})",
                value=i,
                variable=self.gamma_curve,
            )
            curve.grid(row=i + 1, column=0, sticky="w")

        custom = tk.Radiobutton(
            gammactrl, text="Custom", value="custom", variable=self.gamma_curve
        )
        custom.grid(row=i + 2, column=0, sticky="w")

    def base_gradient(self, *args):
        set_window(0, 0, *DISPLAY_SIZE)
        pixels(make_gradient())

    def inverted_gradient(self, *args):
        set_window(0, 0, *DISPLAY_SIZE)
        pixels(make_inverse_gradient())

    def set_inversion(self, *args):
        if self.inverted.get():
            # INVON 0x21
            cmd_write(0x21, b"")
        else:
            # INVOFF 0x20
            cmd_write(0x20, b"")
        self.configure()

    def set_gamma(self, *args):
        custom_gamma = self.gamma_curve.get() == "custom"
        self.vn_frame.update_display = custom_gamma
        self.vp_frame.update_display = custom_gamma
        if not custom_gamma:
            gamma_bit = 1 << int(self.gamma_curve.get())
            # GAMSET 0x26
            cmd_write(0x26, gamma_bit.to_bytes(1, "little"))
        else:
            self.vn_frame.my_on_update()
            self.vp_frame.my_on_update()

    def configure(self):
        # RDDST 0x09
        stats = cmd_read(0x09, 5)
        stats_parsed = DispStatus.parse(stats)
        self.inverted.set(stats_parsed.inverted)
        self.gamma_curve.set(stats_parsed.gamma)

    def mainloop(self):
        self.configure()
        self.root.mainloop()


if __name__ == "__main__":
    set_window(0, 0, *DISPLAY_SIZE)
    pixels(make_gradient())

    gui = EditBoxGUI()
    gui.mainloop()
