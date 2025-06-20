
import asyncio
from datetime import datetime, timedelta

import matplotlib.dates as mdates
import matplotlib.pyplot as plt
import numpy as np

from readBackwards import BackwardsReader

CONT_PLOT = True

async def plot_temps(file_name, read_size=0):
    await asyncio.sleep(2) # let the temepratures to be read a bit first
    plt.ion()
    fig = plt.figure(figsize=(10,6), num="LN2 Cooling plot")
    fig.canvas.mpl_connect('close_event', on_close)
    orig = plt.gca()
    twin = orig.twinx()
    my_fmt = mdates.DateFormatter('%H:%M:%S')
    plt.title(f"Log File: {file_name}")
    
    while CONT_PLOT:
        print(CONT_PLOT)
        await asyncio.sleep(1)
        # buffer size of about 12,500 is about 10 mins worth of measurements with current sample rate (1 per second)
        with BackwardsReader(file=file_name, blksize=read_size, forward=True) as f:
            data = f.readlines()
        result = data_parser(data)
        print(result)
        if result is None:
            continue
        t1, s1, c1, t2, misc = result

        t1_plot = True if np.ndim(t1)==2 else False
        s1_plot = True if np.ndim(s1)==2 else False
        c1_plot = True if np.ndim(c1)==2 else False
        _t2_plot = True if np.ndim(t2)==2 else False
        _misc_plot = True if np.ndim(misc)==2 else False

        _temps = tuple([ts[:,1] for ts in (t1, s1, c1) if np.ndim(ts)==2])
        _times = tuple([ts[:,0] for ts in (t1, s1, c1) if np.ndim(ts)==2])

        _all_temps = np.concatenate(_temps)
        _all_times = np.concatenate(_times)
       
        orig.clear()
        #twin.clear()
        
        if t1_plot:
            orig.plot(t1[:,0], t1[:,1], color="c", lw=3, label="T1")
        else:
            orig.plot(t1, t1, color="c", lw=3, label="T1")
        if s1_plot:
            orig.plot(s1[:,0], s1[:,1], color="r", ls="--", lw=3, label="S1")
        else:
            orig.plot(s1, s1, color="r", ls="--", lw=3, label="S1")
        if c1_plot:
            orig.plot(c1[:,0], c1[:,1], color="m", ls=":", lw=3, label="C1")
        else:
            orig.plot(c1, c1, color="m", ls=":", lw=3, label="C1")

        # orig.plot(t2[:,0], t2[:,1], color="m", lw=3, label="T2")
        orig.legend(loc="upper right")

        _min_c, _max_c = np.nanmin(_all_temps), np.nanmax(_all_temps)
        min_c = _min_c*0.95 if _min_c>=0 else _min_c*1.05 # if <0 then need the mag. of min. to get larger not smaller
        max_c = _max_c*1.05 if _max_c>=0 else _max_c*0.95 # if <0 then need the mag. of max. to get smaller not larger
        orig.set_ylim([min_c, max_c])
        twin.set_ylim([c2f(min_c), c2f(max_c)])
        
        orig.set_ylabel(u"\u2103")
        twin.set_ylabel(u"\u2109")
        orig.set_xlabel(f"Time ({_all_times[0].strftime('%Y-%m-%d')})")
        
        orig.xaxis.set_major_formatter(my_fmt)
        orig.tick_params(axis='x', labelrotation=45)
        orig.set_xlim([_all_times[0], _all_times[-1]])
        if len(t1)>1:
            orig.annotate(get_temp_metric_string(t1[:,0], t1[:,1]), (0.01, 0.01), xycoords="axes fraction", ha="left", va="bottom", backgroundcolor=(1,1,1,0.6))
        plt.tight_layout()
        plt.draw()
        plt.pause(0.1)

def on_close(event):
    """If the figure is closed, then stop the program"""
    print("Closing figure.")
    globals()["CONT_PLOT"] = False

def data_parser(data):
    if len(data)<3:
        return np.array([]), np.array([]), np.array([]), np.array([]), np.array([])
    
    data = data[1:]
    # get rid of end characters
    trimmed_lines = [d.decode("utf-8").replace("[", "").replace("\n", "") for d in data]
    # split the time from the command
    times_and_comms = np.array([d.split("]  ") for d in trimmed_lines])
    times = np.array([datetime.strptime(d, "%Y-%m-%d_%H-%M-%S") for d in times_and_comms[:,0]])
    commands = times_and_comms[:,1]
    _ptc = np.nonzero(commands!="ptc")[0]
    
    times = times[_ptc]
    commands = commands[_ptc]
    
    # I'd imagine I could be fancy but I'll just loop normally
    t1 = []
    s1 = []
    c1 = []
    t2 = []
    misc = []
    for command, time in zip(commands, times):
        if command.startswith("T1 "):
            t1.append([time, float(command.replace("T1 ", ""))])
        elif command.startswith("S1 "):
            s1.append([time, float(command.replace("S1 ", ""))])
        elif command.startswith("C1 "):
            c1.append([time, float(command.replace("C1 ", ""))])
        elif command.startswith("T2 "):
            t2.append([time, float(command.replace("T2 ", ""))])
        else:
            misc.append([time, command])

    return np.array(t1), np.array(s1), np.array(c1), np.array(t2), np.array(misc)

def c2f(celsius):
    return celsius * 9.0 / 5.0 + 32.0

def get_temp_metric_string(times, temp_c, unit=u"\u2103"):
    """Default unit is Celsius."""
    
    # current info
    temp_sign = "+" if temp_c[-1]>=0 else "-"
    current_info = f"Current Temp: {temp_sign}{round(abs(temp_c[-1]),1)}{unit}"
    
    # instantaneous info
    irate_sign = ""
    irate = np.nan
    if len(times)>1:
        t_delta = (times[-1]-times[-2]).total_seconds()/60 # in minutes
        temp_delta = temp_c[-1]-temp_c[-2]
        irate_sign = "+" if temp_delta>=0 else "-"
        irate = abs(temp_delta)/t_delta
    irate_info = f"Inst. Rate: {irate_sign}{round(irate,1)}{unit}/min"
    
    # last 1 min info
    rate1_sign, rate1, std1 = info_over_time(times, temp_c, t_delta_minutes=1)
    min1_info = f"Rate & STD (1 min): {rate1_sign}{round(rate1,1)}{unit}/min, {round(std1,1)}{unit}"
    
    # last 5 min info
    rate5_sign, rate5, std5 = info_over_time(times, temp_c, t_delta_minutes=5)
    min5_info = f"Rate & STD (5 min): {rate5_sign}{round(rate5,1)}{unit}/min, {round(std5,1)}{unit}"
    
    # last 10 min info
    rate10_sign, rate10, std10 = info_over_time(times, temp_c, t_delta_minutes=10)
    min10_info = f"Rate & STD (10 min): {rate10_sign}{round(rate10,1)}{unit}/min, {round(std10,1)}{unit}"
    
    return f"{current_info}\n{irate_info}\n{min1_info}\n{min5_info}\n{min10_info}" 

def info_over_time(times, temp_c, t_delta_minutes):
    rate, std = np.nan, np.nan
    rate_sign = ""
    times = np.array(times)
    excess_data = times<(times[-1] - timedelta(minutes=t_delta_minutes))
    # if there is more than 1 min of data
    if np.sum(excess_data)>0:
        times = times[~excess_data]
        temp_c = np.array(temp_c)[~excess_data]
        t_in_mins = [(t-times[0]).total_seconds()/60 for t in times] 
        rate = get_grad(t_in_mins, temp_c)
        rate_sign = "+" if rate>=0 else "-"
        rate, std = abs(rate), np.std(temp_c)
    return rate_sign, rate, std
    
def get_grad(x, y):
    """Get the gradient by fitting x and y with a straight line."""
    x, y = np.array(x), np.array(y) 
    n = len(x)
    s_xy = np.sum(x*y)
    s_x, s_y = np.sum(x), np.sum(y)
    s_xx = np.sum(x*x)
    return (n*s_xy - s_x*s_y)/(n*s_xx - s_x**2)

if __name__=="__main__":
    async def main(file_name, read_size=0):
        await asyncio.gather(plot_temps(file_name, read_size=read_size))
    
    asyncio.run(main("/Users/kris/Downloads/tempy/log_cooling_2025-06-13_11-30-43.log", read_size=0))