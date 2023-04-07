import tkinter as tk


class EditBoxComponent(tk.Frame):
    def __init__(self, master, max, label):
        super().__init__(master)
        self.max = max
        self.label = tk.Label(self, text=label)
        self.label.pack(side="bottom")
        self.edit_box = tk.Entry(self, width=5)
        self.edit_box.pack(side="bottom")
        self.bar = tk.Canvas(self, width=10, height=64)
        self.bar.pack(side="top")

        self.edit_box.bind("<KeyRelease>", self.update_bar)

    def set_value(self, value):
        value = max(0, min(self.max, value))
        self.edit_box.delete(0, tk.END)
        self.edit_box.insert(0, str(value))
        self.bar.delete("all")
        value_scaled = int(value * 64 / self.max)
        self.bar.create_rectangle(0, 64 - value_scaled, 10, 64, fill="blue")


    def update_bar(self, event):
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

        self.set_value(value)


class EditBoxGUI:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Edit Box GUI")
        self.edit_box_components = []

        # Create empty labels to reserve space for the bars
        for i in range(17):
            label = tk.Label(self.root, text=" ")
            label.grid(row=0, column=i)

        for i in range(17):
            # Create edit box component
            edit_box_component = EditBoxComponent(self.root, 64, f"VP{i+1}")
            edit_box_component.grid(row=1, column=i)
            self.edit_box_components.append(edit_box_component)

        self.root.mainloop()


if __name__ == "__main__":
    gui = EditBoxGUI()
