import sys
import re
from datetime import datetime
import os
import socket
import argparse
import asyncio

from plotting import plot_temps

# default network settings for cooler:
default_cooler_ip = "192.168.3.1"
default_cooler_port = 9760

# timestamping for display and logfile:
default_time = datetime.now().strftime('%Y-%m-%d_%H-%M-%S')
default_filename = os.path.join("log", "log_cooling_" + default_time + ".log")

default_delay = 5 # seconds to wait between ptc queries

# command line argument setup
parser = argparse.ArgumentParser("cooler.py")
parser.add_argument("--cooler-ip", help="IP address of the cooler (default " + default_cooler_ip + ")", type=str, default="192.168.3.1")
parser.add_argument("--cooler-port", help="Port number of the cooler (default " + str(default_cooler_port) + ")", type=int, default=9760)
parser.add_argument("--local-ip", help="IP address of this computer", type=str, default="192.168.3.118")
parser.add_argument("--local-port", help="Port number of this computer", type=int, default=9760)
parser.add_argument("--log", help="Log file path to save to", type=str, default=default_filename)

def write_log(text, file):
    """Write the `text` to the `file` and flush."""
    file.write(text)
    file.flush()

def cooler_command(text):
    """Format the `text` command for the cooler.
    
    The cooler expects commands to be ASCII-encoded, and terminated with a carriage return and newline.
    """
    return bytes(text + '\r\n', encoding='ascii')

def timedstr(text):
    """Prepend `text` with the current date and time."""
    return '['+datetime.now().strftime('%Y-%m-%d_%H-%M-%S')+']  ' + text

def timedprintlog(text, file, end=None):
    """Print `text` to the terminal and write it to the log `file`."""
    outstr = timedstr(text)
    print(outstr, end=end)
    write_log(outstr + "\n", file)

async def ainput(string: str) -> str:
    """Asychnronously get user input from the command line. 
    
    The argument `string` is a prompt, can be the empty string.
    """
    await asyncio.to_thread(sys.stdout.write, f'{string}')
    return await asyncio.to_thread(sys.stdin.readline)

async def cooler_transaction(socket, file):
    """Asynchronously perform cooler status requests by sending it the `ptc` command.

    Responses to the request will be displayed and saved in the log file.
    """
    while True:
        await asyncio.sleep(default_delay)
        try:
            timedprintlog('ptc', file)
            socket.send(cooler_command('PTC'))
            try:
                response = socket.recv(512)
                cleaned = re.sub(r'\n\s*\n', '\n', str(response, encoding='ascii')).splitlines()
                [timedprintlog(s, file) for s in cleaned]

            except TimeoutError:
                return
            
        except KeyboardInterrupt:
            print("closing!")
            socket.close()
            socket.shutdown()
            file.close()
            sys.exit(0)

async def handle_user(socket, file):
    """Asynchronously wait for user input commands.
    
    User input commands will be sent to the cooler, and also timestamped and logged in the log file.
    """
    while True:
        try:
            command = await ainput('')
            socket.send(cooler_command(command))
            write_log(timedstr(command), file)
        except KeyboardInterrupt:
            print("closing!")
            socket.close()
            socket.shutdown()
            file.close()
            sys.exit(0)

async def repl(socket, file):
    """Wait for any user input while querying the cooler for status."""
    await asyncio.gather(cooler_transaction(socket, file), handle_user(socket, file), plot_temps(file, 0))
        


if __name__ == "__main__":
    args = parser.parse_args()
    print("\nconnecting to  ", args.cooler_ip + ":" + str(args.cooler_port))
    print("from           ", args.local_ip + ":" + str(args.local_port))

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
    sock.settimeout(2)
    sock.bind((args.local_ip, args.local_port))
    try:
        sock.connect((args.cooler_ip, args.cooler_port))
    except TimeoutError:
        print("\033[91mConnection timed out! Check IP addresses and connection.\033[0m")
        sys.exit(1)

    print("\033[92mconnected!\033[0m")
    try:
        file = open(args.log, 'w+')
    except IOError:
        print("\033[91mCannot open log file: ", args.log, "\033[0m")
        sys.exit(1)
    print("logging to     ", args.log)

    asyncio.run(repl(sock, file))