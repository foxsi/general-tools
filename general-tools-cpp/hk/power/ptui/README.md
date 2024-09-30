# ptui

A tiny power monitor for the FOXSI housekeeping board. You can use it to turn systems on/off, and watch current/voltage values. Samples power board at 10 Hz and logs to a CSV file.

## Physical configuration

```mermaid
flowchart LR
HK board (192.168.1.16:7777) --> ptui (192.168.1.XXX:PPPPP)
```

## Building
You'll need an installation of [`boost::asio`](https://www.boost.org/users/download/) to use this. If you use homebrew, you can do:

```bash
$ brew install boost
```

If you don't have `CMake` yet, you can also get it through homebrew:
```bash
$ brew install cmake
```

The other dependency, [`FTXUI`](https://github.com/ArthurSonzogni/FTXUI), will automatically be retrieved when you build.

To build, do this:
```bash
$ cd ptui
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
```

## Operation
Run it like this:
```bash
$ ./bin/ptui ipaddress port
```
providing your local IP address and port for the local connection. Once the UI launches, you can input the IP and port of the Housekeeping board. These are `192.168.1.16` and `7777`:

![image](assets/capture.png)

Click the `Connect...` button to connect to the housekeeping board. After connecting, you can select a system on the left and turn it on or off on the right. Data is sampled at 10 Hz from the board, and displays in the center column. Data is also written to a time-tagged CSV file in `log/`.