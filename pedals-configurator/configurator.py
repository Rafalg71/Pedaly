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
    ROOT_CLASS = tb.Window
    THEME = "darkly"
except ImportError:
    ROOT_CLASS = tk.Tk
    THEME = None

class ConfiguratorApp(ROOT_CLASS):
    def __init__(self):
        if THEME:
            super().__init__(themename=THEME)
        else:
            super().__init__()

        self.title("SimRacing Pedals Configurator")
        self.geometry("900x600")

        self.serial_port = None
        self.is_connected = False
        self.msg_queue = queue.Queue()
        self.lock = threading.Lock()

        # UI State
        self.pedal_vars = [] # Holds dict of vars for each pedal

        self.create_ui()

        # Start background threads/tasks
        self.after(50, self.process_queue)
        self.after(100, self.loop_read_data)

        self.reader_thread = threading.Thread(target=self.serial_reader_loop, daemon=True)
        self.reader_thread.start()

    def create_ui(self):
        # Top Frame: Connection
        conn_frame = ttk.Frame(self, padding=10)
        conn_frame.pack(fill='x')

        ttk.Label(conn_frame, text="Serial Port:").pack(side='left', padx=5)
        self.port_var = tk.StringVar()
        self.cbo_ports = ttk.Combobox(conn_frame, textvariable=self.port_var, width=20)
        self.cbo_ports.pack(side='left', padx=5)
        self.refresh_ports()

        ttk.Button(conn_frame, text="Refresh", command=self.refresh_ports).pack(side='left', padx=5)
        self.btn_connect = ttk.Button(conn_frame, text="Connect", command=self.toggle_connect)
        self.btn_connect.pack(side='left', padx=5)

        self.lbl_status = ttk.Label(conn_frame, text="Disconnected", foreground="red")
        self.lbl_status.pack(side='left', padx=10)

        # Main Frame: Pedals
        main_frame = ttk.LabelFrame(self, text="Pedal Calibration", padding=10)
        main_frame.pack(fill='both', expand=True, padx=10, pady=10)

        pedal_names = ["Throttle", "Brake", "Clutch"]

        for idx, name in enumerate(pedal_names):
            row_frame = ttk.Frame(main_frame, padding=5, borderwidth=1, relief="solid")
            row_frame.pack(fill='x', pady=5)

            # Variables
            raw_var = tk.IntVar(value=0)
            min_var = tk.StringVar(value="0")
            max_var = tk.StringVar(value="4095")
            dz_start_var = tk.StringVar(value="0")
            dz_end_var = tk.StringVar(value="0")

            self.pedal_vars.append({
                "raw": raw_var,
                "min": min_var,
                "max": max_var,
                "dz_start": dz_start_var,
                "dz_end": dz_end_var,
                "name": name,
                "idx": idx
            })

            # Row Layout
            # Header
            ttk.Label(row_frame, text=name, width=10, font=('Segoe UI', 12, 'bold')).grid(row=0, column=0, rowspan=2, padx=5)

            # Progress Bar
            pb = ttk.Progressbar(row_frame, orient='horizontal', length=400, mode='determinate', maximum=4095, variable=raw_var)
            pb.grid(row=0, column=1, columnspan=6, padx=5, pady=5, sticky='ew')
            ttk.Label(row_frame, textvariable=raw_var, width=5).grid(row=0, column=7, padx=5)

            # Controls Row
            # Min
            ttk.Label(row_frame, text="Min:").grid(row=1, column=1, sticky='e')
            ttk.Entry(row_frame, textvariable=min_var, width=6).grid(row=1, column=2)
            ttk.Button(row_frame, text="Set", command=lambda i=idx: self.set_current_as_min(i), width=4).grid(row=1, column=3, padx=2)

            # Max
            ttk.Label(row_frame, text="Max:").grid(row=1, column=4, sticky='e')
            ttk.Entry(row_frame, textvariable=max_var, width=6).grid(row=1, column=5)
            ttk.Button(row_frame, text="Set", command=lambda i=idx: self.set_current_as_max(i), width=4).grid(row=1, column=6, padx=2)

            # Deadzones
            dz_frame = ttk.Frame(row_frame)
            dz_frame.grid(row=1, column=7, columnspan=2, padx=10)

            ttk.Label(dz_frame, text="DZ Start (%):").pack(side='left')
            ttk.Entry(dz_frame, textvariable=dz_start_var, width=4).pack(side='left', padx=2)

            ttk.Label(dz_frame, text="End (%):").pack(side='left', padx=(5,0))
            ttk.Entry(dz_frame, textvariable=dz_end_var, width=4).pack(side='left', padx=2)

        # Bottom Frame: Actions
        action_frame = ttk.Frame(self, padding=10)
        action_frame.pack(fill='x')

        ttk.Button(action_frame, text="Read Config", command=self.send_get_config).pack(side='left', padx=10)
        ttk.Button(action_frame, text="Save Calibration", command=self.save_config, style='success.TButton' if THEME else None).pack(side='right', padx=10)

        self.lbl_save_status = ttk.Label(action_frame, text="", foreground="green")
        self.lbl_save_status.pack(side='right', padx=10)

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
                    return
                self.serial_port = serial.Serial(port, 115200, timeout=1)
                self.is_connected = True
                self.btn_connect.config(text="Disconnect", style='danger.TButton' if THEME else None)
                self.lbl_status.config(text="Connected", foreground="green")
                # Request config immediately
                self.send_get_config()
            except Exception as e:
                messagebox.showerror("Connection Error", str(e))
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
        self.btn_connect.config(text="Connect", style='success.TButton' if THEME else None)
        self.lbl_status.config(text="Disconnected", foreground="red")

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
                    # Connection lost or error
                    self.disconnect()
            else:
                time.sleep(0.1)

    def loop_read_data(self):
        if self.is_connected:
            self.send_command("READ")
        self.after(100, self.loop_read_data) # Slower poll

    def send_command(self, cmd):
        if self.is_connected and self.serial_port:
            try:
                with self.lock:
                    self.serial_port.write((cmd + "\n").encode('utf-8'))
            except Exception:
                self.disconnect()

    def send_get_config(self):
        self.send_command("GET_CONFIG")

    def save_config(self):
        # Validate inputs
        for p in self.pedal_vars:
            try:
                mn = int(p['min'].get())
                mx = int(p['max'].get())
                dzs = int(p['dz_start'].get())
                dze = int(p['dz_end'].get())

                if mn == mx:
                    messagebox.showerror("Validation Error", f"Pedal {p['name']}: Min cannot equal Max.")
                    return
                if dzs < 0 or dzs > 100 or dze < 0 or dze > 100:
                     messagebox.showerror("Validation Error", f"Pedal {p['name']}: Deadzone must be 0-100%.")
                     return
                if dzs + dze >= 100:
                     messagebox.showerror("Validation Error", f"Pedal {p['name']}: Total deadzone cannot exceed 100%.")
                     return

            except ValueError:
                messagebox.showerror("Validation Error", f"Invalid number format for {p['name']}.")
                return

        # Send SET commands for each pedal
        # SET idx min max dz_start dz_end
        for p in self.pedal_vars:
            idx = p['idx']
            mn = p['min'].get()
            mx = p['max'].get()
            dzs = p['dz_start'].get()
            dze = p['dz_end'].get()
            self.send_command(f"SET {idx} {mn} {mx} {dzs} {dze}")
            time.sleep(0.05)

        # Send SAVE
        self.send_command("SAVE")
        self.lbl_save_status.config(text="Saving...")
        self.after(2000, lambda: self.lbl_save_status.config(text=""))

    def set_current_as_min(self, idx):
        val = self.pedal_vars[idx]['raw'].get()
        self.pedal_vars[idx]['min'].set(str(val))

    def set_current_as_max(self, idx):
        val = self.pedal_vars[idx]['raw'].get()
        self.pedal_vars[idx]['max'].set(str(val))

    def process_queue(self):
        while not self.msg_queue.empty():
            msg = self.msg_queue.get()
            if msg.startswith("RAW:"):
                try:
                    parts = msg[4:].split(',')
                    if len(parts) == 3:
                        for i in range(3):
                            self.pedal_vars[i]['raw'].set(int(parts[i]))
                except ValueError:
                    pass
            elif msg.startswith("CONF:"):
                # CONF:min:max:dzs:dze, ...
                try:
                    parts = msg[5:].split(',')
                    if len(parts) == 3:
                        for i in range(3):
                            # min:max:dzs:dze
                            p_parts = parts[i].split(':')
                            if len(p_parts) == 4:
                                self.pedal_vars[i]['min'].set(p_parts[0])
                                self.pedal_vars[i]['max'].set(p_parts[1])
                                self.pedal_vars[i]['dz_start'].set(p_parts[2])
                                self.pedal_vars[i]['dz_end'].set(p_parts[3])
                except ValueError:
                    pass
            elif msg == "SAVED":
                self.lbl_save_status.config(text="Saved ✓")

        self.after(50, self.process_queue)

if __name__ == "__main__":
    app = ConfiguratorApp()
    app.mainloop()
