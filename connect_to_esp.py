import tkinter as tk
from tkinter import ttk, messagebox
import tkinter.font as tkFont
import requests


esp_ip = "192.168.138.26"


class ESPControlApp:
    def __init__(self, master):
        self.master = master
        master.geometry("450x250")

        # Make window resizable
        master.resizable(True, True)

        # Set minimum window size
        master.minsize(400, 200)

        # Define default font
        self.default_font = tkFont.Font(family="Helvetica", size=12)

        # Configure ttk style
        self.style = ttk.Style()
        self.style.theme_use('clam')
        self.style.configure('Open.TButton', font=self.default_font, background='green', foreground='white', padding=5)
        self.style.configure('Close.TButton', font=self.default_font, background='red', foreground='white', padding=5)
        self.style.configure('Status.TButton', font=self.default_font, background='blue', foreground='white', padding=5)
        self.style.configure('NormalStatus.TLabel', font=self.default_font, foreground='black')
        self.style.configure('WaterStatus.TLabel', font=self.default_font, foreground='red')

        # Top frame for IP input using grid layout
        self.top_frame = ttk.Frame(master, padding="10 10 10 10")
        self.top_frame.pack(fill=tk.X)

        ttk.Label(self.top_frame, text="ESP32 IP:", font=self.default_font)\
            .grid(row=0, column=0, padx=5, pady=5, sticky="e")

        self.ip_var = tk.StringVar(value=esp_ip)

        ttk.Entry(self.top_frame, textvariable=self.ip_var, width=20, font=self.default_font)\
            .grid(row=0, column=1, padx=5, pady=5, sticky="w")

        # Middle frame for buttons using grid layout
        self.button_frame = ttk.Frame(master, padding="10 10 10 10")
        self.button_frame.pack(fill=tk.X)

        btn_open = ttk.Button(self.button_frame, text="Open Lid", style='Open.TButton',
                              command=lambda: self.send("open"))
        btn_open.grid(row=0, column=0, padx=10, pady=10, sticky="ew")

        btn_close = ttk.Button(self.button_frame, text="Close Lid", style='Close.TButton',
                               command=lambda: self.send("close"))
        btn_close.grid(row=0, column=1, padx=10, pady=10, sticky="ew")

        btn_status = ttk.Button(self.button_frame, text="Check Status", style='Status.TButton',
                                command=self.check_status)
        btn_status.grid(row=0, column=2, padx=10, pady=10, sticky="ew")

        # Configure grid columns to expand equally
        self.button_frame.columnconfigure(0, weight=1)
        self.button_frame.columnconfigure(1, weight=1)
        self.button_frame.columnconfigure(2, weight=1)

        # Bottom frame for status
        self.status_frame = ttk.Frame(master, padding="10 10 10 10")
        self.status_frame.pack(fill=tk.BOTH, expand=True)

        self.status_var = tk.StringVar(value="Status: Not checked")
        self.status_label = ttk.Label(self.status_frame, textvariable=self.status_var, style='NormalStatus.TLabel')
        self.status_label.pack(anchor=tk.W)

        self.last_status = ""
        self.poll_status()


    def send(self, cmd):
        ip = self.ip_var.get().strip()
        if not ip:
            messagebox.showerror("Error", "Please enter ESP32 IP address")
            self.status_var.set("Error: No IP address")
            return

        try:
            r = requests.get(f"http://{ip}/{cmd}", timeout=2)
            self.status_var.set(f"Success: {cmd.upper()} {r.text.strip()}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to send command: {str(e)}")
            self.status_var.set(f"Error: {str(e)}")


    def check_status(self):
        ip = self.ip_var.get().strip()
        if not ip:
            messagebox.showerror("Error", "Please enter ESP32 IP address")
            self.status_var.set("Error: No IP address")
            return

        try:
            r = requests.get(f"http://{ip}/status", timeout=2)
            msg = r.text.strip()
            if msg == "WATER":
                self.status_var.set("Alert: Water Detected!")
                self.status_label.configure(style='WaterStatus.TLabel')
            elif msg:
                self.status_var.set(f"Status: {msg}")
                self.status_label.configure(style='NormalStatus.TLabel')
            else:
                self.status_var.set("Status: No new message")
                self.status_label.configure(style='NormalStatus.TLabel')
        except Exception as e:
            messagebox.showerror("Error", f"Failed to fetch status: {str(e)}")
            self.status_var.set(f"Error: {str(e)}")
            self.status_label.configure(style='NormalStatus.TLabel')


    def poll_status(self):
        ip = self.ip_var.get().strip()
        if ip:
            try:
                r = requests.get(f"http://{ip}/status", timeout=2)
                msg = r.text.strip()
                if msg == "WATER" and self.last_status != "WATER":
                    self.last_status = "WATER"
                    messagebox.showwarning("Water Sensor", "Water detected!")
                    self.status_var.set("Alert: Water Detected!")
                    self.status_label.configure(style='WaterStatus.TLabel')
                elif msg != "WATER":
                    self.last_status = msg
                    self.status_var.set(f"Status: {msg}" if msg else "Status: No new message")
                    self.status_label.configure(style='NormalStatus.TLabel')
            except Exception as e:
                self.status_var.set(f"Error: {str(e)}")
                self.status_label.configure(style='NormalStatus.TLabel')

        self.master.after(2000, self.poll_status)


if __name__ == "__main__":
    root = tk.Tk()
    app = ESPControlApp(root)
    root.mainloop()
