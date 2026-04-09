import os
import subprocess

CONFIG_FILE = "config/server.conf"
SERVER_EXEC = "./server"

server_process = None

def is_valid_ip(ip_list):
    for i in ip_list:
        if int(i) < 0 and int(i) >255:
            return False
    return True
    
def write_config():
    print("\nEnter DHCP Configuration:")

    pool_start = input("POOL_START: ")
    pool_end = input("POOL_END: ")
    
    if not is_valid_ip(pool_start):
        print("Enter valid ip")
        return
    if not is_valid_ip(pool_end):
        print("Enter valid ip")
        return
        
    subnet = input("SUBNET_MASK: ")
    gateway = input("GATEWAY: ")
    lease = input("LEASE_TIME (seconds): ")

    with open(CONFIG_FILE, "w") as f:
        f.write(f"POOL_START={pool_start}\n")
        f.write(f"POOL_END={pool_end}\n")
        f.write(f"SUBNET_MASK={subnet}\n")
        f.write(f"GATEWAY={gateway}\n")
        f.write(f"LEASE_TIME={lease}\n")

    print("Configuration saved.\n")



def start_server():
    global server_process

    if server_process is not None:
        print(" Server already running")
        return

    server_process = subprocess.Popen([SERVER_EXEC])
    print(" Server started\n")



def stop_server():
    global server_process

    if server_process is None:
        print(" Server not running")
        return

    server_process.terminate()
    server_process = None
    print(" Server stopped\n")



def show_leases():
    print("\n Current Leases:\n")

    try:
        with open("logs/dhcp.log", "r") as f:
            lines = f.readlines()


        for line in lines:
            if "Expiry time" in line:
                print(line.strip())

    except FileNotFoundError:
        print("No logs found\n")



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
