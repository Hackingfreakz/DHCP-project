import os
import subprocess
import signal

CONFIG_FILE = "config/server.conf"
SERVER_EXEC = "./server"
PID_FILE = "server.pid"

def is_valid_ip(ip_list):
    try:
        for i in ip_list:
            val = int(i)
            if val < 0 or val > 255:
                return False
        return len(ip_list) == 4
    except:
        return False

def write_config():
    print("\nEnter DHCP Configuration:")

    pool_start = input("POOL_START: ")
    pool_end = input("POOL_END: ")
    subnet = input("SUBNET_MASK: ")
    gateway = input("GATEWAY: ")
    lease = input("LEASE_TIME (seconds): ")

    if not is_valid_ip(pool_start.split(".")):
        print("Invalid POOL_START")
        return
    if not is_valid_ip(pool_end.split(".")):
        print("Invalid POOL_END")
        return
    if not is_valid_ip(subnet.split(".")):
        print("Invalid SUBNET_MASK")
        return
    if not is_valid_ip(gateway.split(".")):
        print("Invalid GATEWAY")
        return

    with open(CONFIG_FILE, "w") as f:
        f.write(f"POOL_START={pool_start}\n")
        f.write(f"POOL_END={pool_end}\n")
        f.write(f"SUBNET_MASK={subnet}\n")
        f.write(f"GATEWAY={gateway}\n")
        f.write(f"LEASE_TIME={lease}\n")

    print("Configuration saved.\n")

def start_server():
    if os.path.exists(PID_FILE):
        print("Server already running")
        return

    proc = subprocess.Popen(["gnome-terminal","--",SERVER_EXEC])
    with open(PID_FILE, "w") as f:
        f.write(str(proc.pid))

    print(f"Server started (PID: {proc.pid})\n")

def stop_server():
    if not os.path.exists(PID_FILE):
        print("Server not running")
        return

    try:
        subprocess.run(["pkill","server"])
        print("Server stopped\n")

    except ProcessLookupError:
        print("Process already stopped")
    except Exception as e:
        print("Error stopping server:", e)

def show_leases():
    print("\nCurrent Leases:\n")

    try:
        with open("leases.txt", "r") as f:
            lines = f.readlines()

        if not lines:
            print("No active leases\n")
            return

        for line in lines:
            client, ip, expiry_date,expiry_time = line.strip().split()

            print(f"Client: {client} | IP: {ip} | Expiry: {expiry_date} {expiry_time}")

        print()

    except FileNotFoundError:
        print("No lease file found\n")

def main():
    while True:
        print("\nCommands:")
        print("1. config")
        print("2. start")
        print("3. stop")
        print("4. show leases")
        print("5. exit")

        cmd = input("\nEnter command: ").strip().lower()

        if cmd == "config":
            write_config()

        elif cmd == "start":
            start_server()

        elif cmd == "stop":
            stop_server()

        elif cmd == "show leases":
            show_leases()

        elif cmd == "exit":
            stop_server()
            break

        else:
            print("Invalid command")

if __name__ == "__main__":
    main()
