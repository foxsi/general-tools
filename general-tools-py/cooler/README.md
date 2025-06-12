# LN2 Cooler Interface Script
You can use this to control the FOXSI LN2 cooling system over Ethernet.

## Setup
You should connect your computer to the cooler control box via Ethernet cable. 

The default IP address of the cooler is `192.168.3.1`. You should configure your network settings so that your machine is in the same subnetwork. 

For example, if your subnetwork mask is `255.255.255.0`, you should set your computer's IP address to be `192.168.3.xxx` where `xxx` is not `1` and is less than `254`.

To verify your physical connection to the cooler, try this command:
```bash
ping 192.168.3.1
```

If you are connected, you should see responses that end in a `time=` value, and don't say "request timed out".

## Usage
>[!INFO] The first time you run this, create a directory `log/` in this folder to save log files to.

You can start running using all the default options like this:
```bash
python cooler.py
```

If it works well, you will see printout in the terminal like this:
```
connecting to   192.168.3.1:9760
from            192.168.3.118:9760
connected!
logging to      log/log_cooling_2025-06-12_12-15-04.log
```

If it cannot connect, or if you see errors about opening the file, you can use the following command line arguments to modify the behavior:
```
--help, -h      show a help message and exit.
--cooler-ip     IP address of the cooler (default 192.168.3.1)
--cooler-port   Port number of the cooler (default 9760)
--local-ip      IP address of this computer (default 192.168.3.118)
--local-port    Port number of this computer (default 9760)
--log           Log file path to save logs to
```

The default log file location is `log/log_cooling_YYYY-MM-DD_HH-MM-SS.log`, with the date and time set by the time the program is launched.

The program will query the cooler with the `ptc` command once every five seconds, and text input by the user to the terminal will be sent as commands to the cooler after the `ENTER` key is pressed.

To exit, use `ctrl-C`. You may need to press it a few times because the script is multithreaded.