import queue
import threading
import tkinter as tk
from tkinter import messagebox, ttk

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    serial = None

BAUD = 115200
APP_TITLE = "Arduino Multi-Function Shield Lab 02"


class SerialWorker:
    def __init__(self, rx_queue):
        self.rx_queue = rx_queue
        self.ser = None
        self.running = False
        self.thread = None

    def ports(self):
        if serial is None:
            return []
        return [p.device for p in serial.tools.list_ports.comports()]

    def connect(self, port):
        self.disconnect()
        self.ser = serial.Serial(port, BAUD, timeout=0.2)
        self.running = True
        self.thread = threading.Thread(target=self._reader, daemon=True)
        self.thread.start()

    def disconnect(self):
        self.running = False
        if self.ser is not None:
            try:
                self.ser.close()
            except Exception:
                pass
        self.ser = None

    def send(self, line):
        if self.ser is not None and self.ser.is_open:
            self.ser.write((line.strip() + "\n").encode("ascii", errors="ignore"))

    def _reader(self):
        while self.running and self.ser is not None and self.ser.is_open:
            try:
                line = self.ser.readline().decode("utf-8", errors="replace").strip()
                if line:
                    self.rx_queue.put(line)
            except Exception as exc:
                self.rx_queue.put("@LOCALERR," + str(exc))
                break


class MFSApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title(APP_TITLE)
        self.geometry("1080x720")
        self.minsize(920, 620)

        self.rx_queue = queue.Queue()
        self.link = SerialWorker(self.rx_queue)

        self.status = tk.StringVar(value="DISCONNECTED")
        self.uptime = tk.StringVar(value="0 ms")
        self.test_state = tk.StringVar(value="IDLE")
        self.pot = tk.IntVar(value=0)
        self.pot_min = None
        self.pot_max = None
        self.pot_stats = tk.StringVar(value="min --   max --")
        self.button_state = [tk.StringVar(value="RELEASED") for _ in range(3)]
        self.button_count = [tk.IntVar(value=0) for _ in range(3)]
        self.led_state = [tk.BooleanVar(value=False) for _ in range(4)]
        self.buzzer_state = tk.StringVar(value="OFF")
        self.display_state = tk.StringVar(value="0000")
        self.log_telemetry = tk.BooleanVar(value=False)

        self._build_header()
        self._build_tabs()
        self.refresh_ports()
        self.after(50, self._poll_rx)
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    def _build_header(self):
        bar = ttk.Frame(self, padding=8)
        bar.pack(fill="x")

        ttk.Label(bar, text="COM port:").pack(side="left")
        self.port = ttk.Combobox(bar, width=14, state="readonly")
        self.port.pack(side="left", padx=4)
        ttk.Button(bar, text="Refresh", command=self.refresh_ports).pack(side="left", padx=3)
        ttk.Button(bar, text="Connect", command=self.connect).pack(side="left", padx=3)
        ttk.Button(bar, text="Disconnect", command=self.disconnect).pack(side="left", padx=3)

        ttk.Separator(bar, orient="vertical").pack(side="left", fill="y", padx=10)
        ttk.Label(bar, textvariable=self.status).pack(side="left")
        ttk.Button(bar, text="RUN FULL TEST", command=lambda: self.command("TEST,ALL")).pack(side="right")
        ttk.Button(bar, text="STOP TEST", command=lambda: self.command("TEST,STOP")).pack(side="right", padx=5)

    def _build_tabs(self):
        notebook = ttk.Notebook(self)
        notebook.pack(fill="both", expand=True, padx=8, pady=(0, 8))

        self.tab_overview = ttk.Frame(notebook, padding=14)
        self.tab_leds = ttk.Frame(notebook, padding=14)
        self.tab_buttons = ttk.Frame(notebook, padding=14)
        self.tab_pot = ttk.Frame(notebook, padding=14)
        self.tab_display = ttk.Frame(notebook, padding=14)
        self.tab_buzzer = ttk.Frame(notebook, padding=14)
        self.tab_system = ttk.Frame(notebook, padding=14)

        for tab, title in [
            (self.tab_overview, "OVERVIEW"),
            (self.tab_leds, "LEDs"),
            (self.tab_buttons, "BUTTONS"),
            (self.tab_pot, "POT A0"),
            (self.tab_display, "DISPLAY"),
            (self.tab_buzzer, "BUZZER"),
            (self.tab_system, "SYSTEM"),
        ]:
            notebook.add(tab, text=title)

        self._build_overview()
        self._build_leds()
        self._build_buttons()
        self._build_pot()
        self._build_display()
        self._build_buzzer()
        self._build_system()

    def _build_overview(self):
        ttk.Label(
            self.tab_overview,
            text="Arduino UNO + Multi-Function Shield",
            font=("Segoe UI", 18, "bold"),
        ).pack(pady=(4, 14))

        grid = ttk.Frame(self.tab_overview)
        grid.pack(fill="both", expand=True)
        grid.columnconfigure(0, weight=1)
        grid.columnconfigure(1, weight=1)

        buttons = ttk.LabelFrame(grid, text="Physical buttons", padding=12)
        buttons.grid(row=0, column=0, padx=8, pady=8, sticky="nsew")
        for i in range(3):
            ttk.Label(buttons, text=f"S{i + 1} / A{i + 1}", font=("Segoe UI", 11, "bold")).grid(
                row=0, column=i, padx=12
            )
            ttk.Label(buttons, textvariable=self.button_state[i], font=("Consolas", 12)).grid(
                row=1, column=i, padx=12, pady=(5, 0)
            )
            ttk.Label(buttons, textvariable=self.button_count[i]).grid(row=2, column=i, padx=12)

        pot = ttk.LabelFrame(grid, text="Potentiometer A0", padding=12)
        pot.grid(row=0, column=1, padx=8, pady=8, sticky="nsew")
        ttk.Label(pot, textvariable=self.pot, font=("Consolas", 24, "bold")).pack()
        ttk.Progressbar(pot, maximum=1023, variable=self.pot, length=320).pack(pady=8)
        ttk.Label(pot, textvariable=self.pot_stats).pack()

        leds = ttk.LabelFrame(grid, text="LED D1..D4", padding=12)
        leds.grid(row=1, column=0, padx=8, pady=8, sticky="nsew")
        self.overview_led_labels = []
        for i in range(4):
            label = ttk.Label(leds, text=f"D{i + 1}: OFF", font=("Segoe UI", 12, "bold"))
            label.grid(row=0, column=i, padx=10, pady=12)
            self.overview_led_labels.append(label)

        outputs = ttk.LabelFrame(grid, text="Outputs / test", padding=12)
        outputs.grid(row=1, column=1, padx=8, pady=8, sticky="nsew")
        ttk.Label(outputs, text="Display").grid(row=0, column=0, sticky="w")
        ttk.Label(outputs, textvariable=self.display_state, font=("Consolas", 28, "bold")).grid(
            row=1, column=0, padx=10
        )
        ttk.Label(outputs, text="Buzzer").grid(row=0, column=1, sticky="w")
        ttk.Label(outputs, textvariable=self.buzzer_state, font=("Consolas", 14, "bold")).grid(
            row=1, column=1, padx=10
        )
        ttk.Label(outputs, text="Auto test").grid(row=2, column=0, sticky="w", pady=(16, 0))
        ttk.Label(outputs, textvariable=self.test_state, font=("Consolas", 12, "bold")).grid(
            row=3, column=0, columnspan=2, sticky="w"
        )

    def _build_leds(self):
        ttk.Label(
            self.tab_leds,
            text="Manual LED control (shield LEDs are active LOW)",
            font=("Segoe UI", 14, "bold"),
        ).pack(pady=8)
        box = ttk.Frame(self.tab_leds)
        box.pack(pady=24)
        uno_pins = [13, 12, 11, 10]
        for i in range(4):
            ttk.Checkbutton(
                box,
                text=f"D{i + 1} / UNO D{uno_pins[i]}",
                variable=self.led_state[i],
                command=lambda n=i: self.set_led(n),
            ).grid(row=0, column=i, padx=18)
        ttk.Button(self.tab_leds, text="ALL ON", command=lambda: self.command("LED,ALL,ON")).pack(pady=4)
        ttk.Button(self.tab_leds, text="ALL OFF", command=lambda: self.command("LED,ALL,OFF")).pack(pady=4)

    def _build_buttons(self):
        ttk.Label(self.tab_buttons, text="Live button state and press counters", font=("Segoe UI", 14, "bold")).pack(
            pady=8
        )
        box = ttk.Frame(self.tab_buttons)
        box.pack(pady=30)
        for i in range(3):
            ttk.Label(box, text=f"S{i + 1} / A{i + 1}", font=("Segoe UI", 16, "bold")).grid(
                row=0, column=i, padx=35
            )
            ttk.Label(box, textvariable=self.button_state[i], font=("Consolas", 18)).grid(
                row=1, column=i, padx=35, pady=8
            )
            ttk.Label(box, text="presses").grid(row=2, column=i)
            ttk.Label(box, textvariable=self.button_count[i], font=("Consolas", 16, "bold")).grid(row=3, column=i)

    def _build_pot(self):
        ttk.Label(self.tab_pot, text="Potentiometer A0", font=("Segoe UI", 14, "bold")).pack(pady=8)
        ttk.Label(self.tab_pot, textvariable=self.pot, font=("Consolas", 52, "bold")).pack(pady=18)
        ttk.Progressbar(self.tab_pot, maximum=1023, variable=self.pot, length=650).pack(pady=12)
        self.pot_percent = ttk.Label(self.tab_pot, text="0.0 %", font=("Segoe UI", 16))
        self.pot_percent.pack()
        ttk.Label(self.tab_pot, textvariable=self.pot_stats, font=("Consolas", 13)).pack(pady=10)
        ttk.Button(self.tab_pot, text="Reset min/max", command=self.reset_pot_stats).pack()

    def _build_display(self):
        ttk.Label(self.tab_display, text="4-digit display / 2 x 74HC595", font=("Segoe UI", 14, "bold")).pack(
            pady=8
        )
        line = ttk.Frame(self.tab_display)
        line.pack(pady=18)
        self.display_entry = ttk.Entry(line, width=8, font=("Consolas", 28), justify="center")
        self.display_entry.insert(0, "2026")
        self.display_entry.pack(side="left", padx=8)
        ttk.Button(line, text="SEND", command=self.send_display).pack(side="left", padx=8)

        quick = ttk.Frame(self.tab_display)
        quick.pack(pady=8)
        for text in ["0000", "1234", "8888"]:
            ttk.Button(quick, text=text, command=lambda value=text: self.command("DISP," + value)).pack(
                side="left", padx=5
            )

        cfg = ttk.LabelFrame(self.tab_display, text="Digit-select polarity (clone diagnostic)", padding=10)
        cfg.pack(pady=18)
        ttk.Button(cfg, text="STANDARD", command=lambda: self.command("CFG,DIGITSEL,STD")).pack(side="left", padx=8)
        ttk.Button(cfg, text="INVERTED", command=lambda: self.command("CFG,DIGITSEL,INV")).pack(side="left", padx=8)
        ttk.Label(cfg, text="Use only if digit selection is wrong on this physical clone.").pack(side="left", padx=12)

    def _build_buzzer(self):
        ttk.Label(
            self.tab_buzzer,
            text="Buzzer D3 — active buzzer primary control + PWM diagnostic",
            font=("Segoe UI", 14, "bold"),
        ).pack(pady=8)

        active = ttk.LabelFrame(self.tab_buzzer, text="Active buzzer (primary)", padding=12)
        active.pack(pady=12)
        ttk.Button(active, text="ON", command=lambda: self.command("BUZ,ON")).pack(side="left", padx=5)
        ttk.Button(active, text="OFF", command=lambda: self.command("BUZ,OFF")).pack(side="left", padx=5)
        ttk.Button(active, text="BEEP 200 ms", command=lambda: self.command("BEEP,200")).pack(side="left", padx=5)

        passive = ttk.LabelFrame(self.tab_buzzer, text="PWM / tone modulation diagnostic", padding=12)
        passive.pack(pady=12)
        ttk.Label(passive, text="Hz:").pack(side="left")
        self.tone_freq = ttk.Entry(passive, width=8)
        self.tone_freq.insert(0, "1000")
        self.tone_freq.pack(side="left", padx=6)
        ttk.Button(passive, text="TONE ON", command=self.tone_on).pack(side="left", padx=5)
        ttk.Button(passive, text="TONE OFF", command=lambda: self.command("TONE,OFF")).pack(side="left", padx=5)

    def _build_system(self):
        info = ttk.Frame(self.tab_system)
        info.pack(fill="x")
        ttk.Label(info, text="Uptime:").pack(side="left")
        ttk.Label(info, textvariable=self.uptime).pack(side="left", padx=(4, 14))
        ttk.Checkbutton(info, text="Log @STATE telemetry", variable=self.log_telemetry).pack(side="left")
        ttk.Button(info, text="INFO", command=lambda: self.command("INFO")).pack(side="right")
        ttk.Button(info, text="STATE?", command=lambda: self.command("STATE?")).pack(side="right", padx=5)

        self.log = tk.Text(self.tab_system, height=25, font=("Consolas", 10))
        self.log.pack(fill="both", expand=True, pady=8)

        raw = ttk.Frame(self.tab_system)
        raw.pack(fill="x")
        self.raw_entry = ttk.Entry(raw)
        self.raw_entry.pack(side="left", fill="x", expand=True)
        self.raw_entry.bind("<Return>", lambda _event: self.send_raw())
        ttk.Button(raw, text="SEND", command=self.send_raw).pack(side="left", padx=6)

    def refresh_ports(self):
        ports = self.link.ports()
        self.port["values"] = ports
        if ports and self.port.get() not in ports:
            self.port.set(ports[0])

    def connect(self):
        if serial is None:
            messagebox.showerror("pyserial missing", "Install it with: py -m pip install pyserial")
            return
        port = self.port.get()
        if not port:
            messagebox.showwarning("COM port", "Select a COM port first.")
            return
        try:
            self.link.connect(port)
            self.status.set(f"CONNECTED {port} @ {BAUD}")
            self._add_log(f"# connected to {port} @ {BAUD}")
            self.after(1800, lambda: self.command("INFO"))
        except Exception as exc:
            messagebox.showerror("Connection error", str(exc))

    def disconnect(self):
        self.link.disconnect()
        self.status.set("DISCONNECTED")
        self._add_log("# disconnected")

    def command(self, line):
        self.link.send(line)
        self._add_log("> " + line)

    def set_led(self, index):
        self.command(f"LED,{index + 1},{'ON' if self.led_state[index].get() else 'OFF'}")

    def send_display(self):
        value = self.display_entry.get().strip()[:4]
        if not value or any(ch not in "0123456789-" for ch in value):
            messagebox.showwarning("Display", "Use up to four digits (or '-').")
            return
        self.command("DISP," + value)

    def tone_on(self):
        try:
            hz = max(30, min(5000, int(self.tone_freq.get())))
        except ValueError:
            hz = 1000
        self.command(f"TONE,{hz}")

    def send_raw(self):
        line = self.raw_entry.get().strip()
        if line:
            self.command(line)
            self.raw_entry.delete(0, "end")

    def reset_pot_stats(self):
        self.pot_min = None
        self.pot_max = None
        self.pot_stats.set("min --   max --")

    def _update_pot_stats(self, value):
        self.pot_min = value if self.pot_min is None else min(self.pot_min, value)
        self.pot_max = value if self.pot_max is None else max(self.pot_max, value)
        self.pot_stats.set(f"min {self.pot_min}   max {self.pot_max}")

    def _parse_state(self, line):
        # @STATE,uptime,pot,b1,b2,b3,c1,c2,c3,l1,l2,l3,l4,buzmode,buzhz,display,test
        parts = line.split(",", 16)
        if len(parts) != 17:
            return
        try:
            self.uptime.set(parts[1] + " ms")
            pot = int(parts[2])
            self.pot.set(pot)
            self.pot_percent.configure(text=f"{pot * 100.0 / 1023.0:.1f} %")
            self._update_pot_stats(pot)

            for i in range(3):
                pressed = bool(int(parts[3 + i]))
                self.button_state[i].set("PRESSED" if pressed else "RELEASED")
                self.button_count[i].set(int(parts[6 + i]))

            for i in range(4):
                on = bool(int(parts[9 + i]))
                self.led_state[i].set(on)
                self.overview_led_labels[i].configure(text=f"D{i + 1}: {'ON' if on else 'OFF'}")

            buz_mode = parts[13]
            buz_hz = int(parts[14])
            self.buzzer_state.set(f"{buz_mode} {buz_hz} Hz" if buz_hz else buz_mode)
            self.display_state.set(parts[15])
            self.test_state.set(parts[16])
        except (ValueError, IndexError):
            return

    def _add_log(self, line):
        if not hasattr(self, "log"):
            return
        self.log.insert("end", line + "\n")
        self.log.see("end")

    def _poll_rx(self):
        try:
            while True:
                line = self.rx_queue.get_nowait()
                if line.startswith("@STATE,"):
                    self._parse_state(line)
                    if self.log_telemetry.get():
                        self._add_log("< " + line)
                else:
                    self._add_log("< " + line)
                if line.startswith("@LOCALERR,"):
                    self.status.set("SERIAL ERROR")
        except queue.Empty:
            pass
        self.after(50, self._poll_rx)

    def _on_close(self):
        self.link.disconnect()
        self.destroy()


if __name__ == "__main__":
    app = MFSApp()
    app.mainloop()
