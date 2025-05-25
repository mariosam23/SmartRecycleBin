import tkinter as tk
from tkinter import messagebox
import requests

esp_ip = "192.168.1.133"


class ESPControlApp:
    def __init__(self, master):
        self.master = master
        master.title("ESP32 → Arduino Servo")

        self.ip_var = tk.StringVar()
        tk.Label(master, text="ESP32 IP:").grid(row=0, column=0, padx=5, pady=5)
        tk.Entry(master, textvariable=self.ip_var, width=18).grid(row=0, column=1, padx=5)
        
        # Default IP
        self.ip_var.set("192.168.1.133")

        tk.Button(master, text="Open Lid", width=10, bg="green", fg="white",
                  command=lambda: self.send("open"))\
            .grid(row=1, column=0, padx=5, pady=5)

        tk.Button(master, text="Close Lid", width=10, bg="red", fg="white",
                  command=lambda: self.send("close"))\
            .grid(row=1, column=1, padx=5, pady=5)

        tk.Button(master, text="Check Status", width=22, bg="blue", fg="white",
                  command=self.check_status)\
            .grid(row=2, column=0, columnspan=2, padx=5, pady=5)

        self.last_status = ""
        self.poll_status()

    def send(self, cmd):
        ip = self.ip_var.get().strip()
        if not ip:
            print("Error: Enter ESP32 IP address")
            return
        r = requests.get(f"http://{ip}/{cmd}", timeout=2)
        print(f"Success: {cmd.upper()} → {r.text.strip()}")

    def check_status(self):
        ip = self.ip_var.get().strip()
        if not ip:
            print("Error: Enter ESP32 IP address")
            return
        
        r = requests.get(f"http://{ip}/status", timeout=2)
        msg = r.text.strip()
        if msg == "WATER":
            print("Alert: Water Detected!")
        elif msg:
            print("Status:", msg)
        else:
            print("No new message.")


    def poll_status(self):
        ip = self.ip_var.get().strip()
        if ip:
            r = requests.get(f"http://{ip}/status", timeout=2)
            msg = r.text.strip()
            if msg == "WATER" and self.last_status != "WATER":
                self.last_status = "WATER"
                messagebox.showwarning("Water Sensor", "Water detected!")
            elif msg != "WATER":
                self.last_status = msg
        self.master.after(2000, self.poll_status)

if __name__ == "__main__":
    root = tk.Tk()
    ESPControlApp(root)
    root.mainloop()
