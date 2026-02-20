import tkinter as tk
from tkinter import ttk, messagebox
import serial
import serial.tools.list_ports
import threading
import time
import queue
import sys

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
            super().__init__(themename=THEME, title="Konfigurator Pedałów SimRacing", iconphoto=None)
        else:
            super().__init__()
            self.title("Konfigurator Pedałów SimRacing")

        self.geometry("950x650")

        self.serial_port = None
        self.is_connected = False
        self.msg_queue = queue.Queue()
        self.lock = threading.Lock()

        # UI State
        self.pedal_vars = []

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

        # --- Pedals Frame ---
        main_frame = ttk.Labelframe(self, text="Kalibracja Pedałów", padding=15)
        main_frame.pack(fill='both', expand=True, padx=20, pady=10)

        pedal_names = ["Gaz (Throttle)", "Hamulec (Brake)", "Sprzęgło (Clutch)"]

        for idx, name in enumerate(pedal_names):
            row_frame = ttk.Frame(main_frame, padding=10)
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

            # -- Row Layout --

            # Name & Current Value
            info_frame = ttk.Frame(row_frame, width=200)
            info_frame.pack(side='left', padx=(0, 20))
            ttk.Label(info_frame, text=name, font=('Helvetica', 12, 'bold')).pack(anchor='w')
            val_lbl = ttk.Label(info_frame, textvariable=raw_var, font=('Consolas', 14), foreground="#00bc8c" if THEME else "black")
            val_lbl.pack(anchor='w')

            # Controls
            ctrl_frame = ttk.Frame(row_frame)
            ctrl_frame.pack(side='left', fill='x', expand=True)

            # Progress Bar
            pb = ttk.Progressbar(ctrl_frame, orient='horizontal', mode='determinate', maximum=4095, variable=raw_var, style='success.Striped.Horizontal.TProgressbar' if THEME else None)
            pb.pack(fill='x', pady=(0, 10))

            # Settings Grid
            settings_grid = ttk.Frame(ctrl_frame)
            settings_grid.pack(fill='x')

            # Min
            ttk.Label(settings_grid, text="Min (Puszczony):").grid(row=0, column=0, padx=5, sticky='e')
            ttk.Entry(settings_grid, textvariable=min_var, width=8).grid(row=0, column=1, padx=2)
            ttk.Button(settings_grid, text="Ustaw", command=lambda i=idx: self.set_current_as_min(i), style='secondary.Outline.TButton' if THEME else None, width=6).grid(row=0, column=2, padx=5)

            # Max
            ttk.Label(settings_grid, text="Max (Wciśnięty):").grid(row=0, column=3, padx=(20, 5), sticky='e')
            ttk.Entry(settings_grid, textvariable=max_var, width=8).grid(row=0, column=4, padx=2)
            ttk.Button(settings_grid, text="Ustaw", command=lambda i=idx: self.set_current_as_max(i), style='secondary.Outline.TButton' if THEME else None, width=6).grid(row=0, column=5, padx=5)

            # Deadzones
            ttk.Label(settings_grid, text="Martwa strefa Start (%):").grid(row=0, column=6, padx=(20, 5), sticky='e')
            ttk.Spinbox(settings_grid, from_=0, to=50, textvariable=dz_start_var, width=4).grid(row=0, column=7, padx=2)

            ttk.Label(settings_grid, text="Koniec (%):").grid(row=0, column=8, padx=5, sticky='e')
            ttk.Spinbox(settings_grid, from_=0, to=50, textvariable=dz_end_var, width=4).grid(row=0, column=9, padx=2)

            # Separator
            ttk.Separator(main_frame, orient='horizontal').pack(fill='x', pady=5)

        # --- Footer Frame ---
        footer = ttk.Frame(self, padding=15)
        footer.pack(fill='x', side='bottom')

        ttk.Button(footer, text="📥 Odczytaj Ustawienia", command=self.send_get_config, style='info.TButton' if THEME else None).pack(side='left', padx=10)

        save_btn = ttk.Button(footer, text="💾 Zapisz Kalibrację w Urządzeniu", command=self.save_config, style='success.TButton' if THEME else None)
        save_btn.pack(side='right', padx=10)

        self.lbl_save_status = ttk.Label(footer, text="", font=('Helvetica', 10, 'italic'), foreground="#00bc8c")
        self.lbl_save_status.pack(side='right', padx=20)

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
                # Request config immediately
                self.send_get_config()
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
                    messagebox.showerror("Błąd Walidacji", f"Pedał {p['name']}: Min nie może być równe Max.")
                    return
                if dzs < 0 or dzs > 100 or dze < 0 or dze > 100:
                     messagebox.showerror("Błąd Walidacji", f"Pedał {p['name']}: Martwa strefa musi być 0-100%.")
                     return
                if dzs + dze >= 100:
                     messagebox.showerror("Błąd Walidacji", f"Pedał {p['name']}: Suma stref nie może przekraczać 100%.")
                     return

            except ValueError:
                messagebox.showerror("Błąd Walidacji", f"Nieprawidłowy format liczby dla {p['name']}.")
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
        self.lbl_save_status.config(text="Zapisywanie...")
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
                self.lbl_save_status.config(text="Zapisano Pomyślnie ✓")

        self.after(50, self.process_queue)

if __name__ == "__main__":
    app = ConfiguratorApp()
    app.mainloop()
