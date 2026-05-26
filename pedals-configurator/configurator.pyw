import tkinter as tk
from tkinter import ttk, messagebox
import serial
import serial.tools.list_ports
import threading
import time
import queue

# Try to import ttkbootstrap for modern look
try:
    import ttkbootstrap as tb
    from ttkbootstrap.constants import *
    ROOT_CLASS = tb.Window
    THEME = "darkly"
except ImportError:
    ROOT_CLASS = tk.Tk
    THEME = None

class ConfiguratorApp(ROOT_CLASS):
    def __init__(self):
        if THEME:
            super().__init__(themename=THEME, title="Tester Shiftera / Button Box")
        else:
            super().__init__()
            self.title("Tester Shiftera / Button Box")

        self.geometry("600x400")

        self.serial_port = None
        self.is_connected = False
        self.msg_queue = queue.Queue()
        self.lock = threading.Lock()

        # UI State
        self.button_vars = [] # List of IntVars for buttons (0/1)

        self.create_ui()

        # Start background threads/tasks
        self.after(50, self.process_queue)
        self.after(100, self.loop_read_data)

        self.reader_thread = threading.Thread(target=self.serial_reader_loop, daemon=True)
        self.reader_thread.start()

    def create_ui(self):
        # --- Connection Frame ---
        conn_frame = ttk.Labelframe(self, text="Połączenie", padding=15)
        conn_frame.pack(fill='x', padx=20, pady=10)

        ttk.Label(conn_frame, text="Port COM:", font=('Helvetica', 10)).pack(side='left', padx=10)
        self.port_var = tk.StringVar()
        self.cbo_ports = ttk.Combobox(conn_frame, textvariable=self.port_var, width=25, state="readonly")
        self.cbo_ports.pack(side='left', padx=5)
        self.refresh_ports()

        ttk.Button(conn_frame, text="⟳ Odśwież", command=self.refresh_ports, style='info.Outline.TButton' if THEME else None).pack(side='left', padx=10)
        self.btn_connect = ttk.Button(conn_frame, text="Połącz", command=self.toggle_connect, style='primary.TButton' if THEME else None)
        self.btn_connect.pack(side='left', padx=5)

        self.lbl_status = ttk.Label(conn_frame, text="Rozłączono", font=('Helvetica', 10, 'bold'), foreground="red")
        self.lbl_status.pack(side='right', padx=20)


        # --- Buttons Frame (Shifter) ---
        btns_frame = ttk.Labelframe(self, text="Przyciski / Shifter (GPIO 4-11)", padding=15)
        btns_frame.pack(fill='both', expand=True, padx=20, pady=10)

        btn_grid = ttk.Frame(btns_frame)
        btn_grid.pack(anchor='center', expand=True)

        self.btn_widgets = []
        for i in range(8):
            lbl = ttk.Label(btn_grid, text=f"{i+1}", width=4, anchor='center', font=('Helvetica', 16, 'bold'), relief="raised", borderwidth=2)
            lbl.grid(row=0, column=i, padx=10, pady=10)
            self.btn_widgets.append(lbl)

            ttk.Label(btn_grid, text=f"GPIO {i+4}", font=('Arial', 10)).grid(row=1, column=i, padx=5)

    def refresh_ports(self):
        ports = sorted([p.device for p in serial.tools.list_ports.comports()])
        self.cbo_ports['values'] = ports
        if ports:
            self.cbo_ports.current(0)

    def toggle_connect(self):
        if not self.is_connected:
            try:
                port = self.port_var.get()
                if not port:
                    messagebox.showwarning("Błąd", "Wybierz port COM!")
                    return
                self.serial_port = serial.Serial(port, 115200, timeout=1)
                self.is_connected = True
                self.btn_connect.config(text="Rozłącz", style='danger.TButton' if THEME else None)
                self.lbl_status.config(text="Połączono", foreground="#00bc8c" if THEME else "green")
            except Exception as e:
                messagebox.showerror("Błąd Połączenia", str(e))
        else:
            self.disconnect()

    def disconnect(self):
        self.is_connected = False
        if self.serial_port:
            try:
                self.serial_port.close()
            except:
                pass
            self.serial_port = None
        self.btn_connect.config(text="Połącz", style='primary.TButton' if THEME else None)
        self.lbl_status.config(text="Rozłączono", foreground="red")

    def serial_reader_loop(self):
        while True:
            if self.is_connected and self.serial_port:
                try:
                    if self.serial_port.in_waiting:
                        line = self.serial_port.readline().decode('utf-8', errors='ignore').strip()
                        if line:
                            self.msg_queue.put(line)
                    else:
                        time.sleep(0.01)
                except Exception:
                    self.disconnect()
            else:
                time.sleep(0.1)

    def loop_read_data(self):
        if self.is_connected:
            self.send_command("READ")
        self.after(50, self.loop_read_data) # Poll faster for buttons (50ms)

    def send_command(self, cmd):
        if self.is_connected and self.serial_port:
            try:
                with self.lock:
                    self.serial_port.write((cmd + "\n").encode('utf-8'))
            except Exception:
                self.disconnect()

    def update_buttons_ui(self, buttons_byte):
        for i in range(8):
            is_pressed = (buttons_byte >> i) & 1
            if is_pressed:
                self.btn_widgets[i].config(background="#00bc8c", foreground="white") # Green active
            else:
                self.btn_widgets[i].config(background="#303030", foreground="white") # Dark inactive

    def process_queue(self):
        while not self.msg_queue.empty():
            msg = self.msg_queue.get()
            if msg.startswith("BTN:"):
                # BTN:buttons
                try:
                    buttons = int(msg[4:])
                    self.update_buttons_ui(buttons)
                except ValueError:
                    pass

        self.after(50, self.process_queue)

if __name__ == "__main__":
    app = ConfiguratorApp()
    app.mainloop()
