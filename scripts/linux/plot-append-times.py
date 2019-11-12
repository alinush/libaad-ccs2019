#!/usr/bin/env python

import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter
from matplotlib.dates import MonthLocator, DateFormatter, DayLocator, epoch2num, num2date
import pandas
import sys
import time

c = 1

plotEverything = False
if len(sys.argv) < 4:
    print "Usage:", sys.argv[0], "[--all] <output-png-file> <batch-size> <csv-file> [<csv-file>] ..."
    print
    print "If --all is passed, then also plots individual append times."
    sys.exit(0)

del sys.argv[0] # that's the program's name

if sys.argv[0] == "--all":
    print "Plotting append times for each individual append..."
    plotEverything = True
    del sys.argv[0]

out_png_file = sys.argv[0]
del sys.argv[0]
if not out_png_file.endswith('.png'):
    print "ERROR: Expected .png file as first argument"
    sys.exit(1)

batch_size = sys.argv[0]
del sys.argv[0]

print sys.argv

data_files = [f for f in sys.argv]

print "Reading CSV files:", data_files, "..."

csv_data = pandas.concat((pandas.read_csv(f) for f in data_files))
csv_data['cumulativeAvgTime'] = csv_data.appendTimeMillisec.expanding().mean()
csv_data.cumulativeAvgTime /= 1000;
csv_data.appendTimeMillisec /= 1000;


#print "Raw:"
#print csv_data.to_string()
#print csv_data.columns
#print csv_data['dictSize'].values

#print "Averaged:"
#print csv_data[csv_data.dictSize == 1023]; # filter results by dictionary size

#print csv_data.to_string()  # print all data

SMALL_SIZE = 10
MEDIUM_SIZE = 20
BIGGER_SIZE = 25

plt.rc('font', size=BIGGER_SIZE)          # controls default text sizes
plt.rc('axes', titlesize=BIGGER_SIZE)     # fontsize of the axes title
plt.rc('axes', labelsize=MEDIUM_SIZE)    # fontsize of the x and y labels
plt.rc('xtick', labelsize=MEDIUM_SIZE)    # fontsize of the tick labels
plt.rc('ytick', labelsize=MEDIUM_SIZE)    # fontsize of the tick labels
plt.rc('legend', fontsize=MEDIUM_SIZE)    # legend fontsize
plt.rc('figure', titlesize=BIGGER_SIZE)  # fontsize of the figure title

# adjust the size of the plot here: (20, 10) is bigger than (10, 5) in the
# sense that fonts will look smaller on (20, 10)
#fig, ax1 = plt.subplots(figsize=(15, 10))

def plotNumbers(data):
    x = csv_data.dictSize    #.unique() # x-axis is the AAD size
    #print sizes
    #x_ticks = [ int(n) for n in x.values if int(n) % int(batch_size) == 0 ]
    #x_ticks.insert(0, 1)    # add 2^0 = 1 as an X tick as well

    fig, ax1= plt.subplots(figsize=(12,9)) 
    if plotEverything:
        ax1.set_title('AAD append time (batch size = ' + batch_size + ')') #, fontsize=fontsize)
    else:
        ax1.set_title('AAD cumulative average append time (batch size = ' + batch_size + ')') #, fontsize=fontsize)
    ax1.set_xlabel("Number of key-value pairs"); #, fontsize=fontsize)
    ax1.set_ylabel("Append time (secs)") #, fontsize=fontsize)

    plots = []
    names = []
    
    #plt.xticks(x, x, rotation=30)
    col1 = data.appendTimeMillisec.values
    #print col1
    #print

    assert len(x) == len(col1)

    plt1, = ax1.plot(x, csv_data.cumulativeAvgTime)
    plots.append(plt1)
    names.append("Cumulative average")

    if plotEverything:
        plt2, = ax1.plot(x, col1)
        plots.append(plt2)
        names.append("Last append")

        #ax1.set_xticks(x_ticks)
        #ax1.set_xticklabels(x, rotation=30)
        ax1.legend(plots,
                   names,
                   loc='best')#, fontsize=fontsize

    plt.tight_layout()
    #date = time.strftime("%Y-%m-%d-%H-%M-%S")
    #out_png_file = 'append-time-' + date + '.png'
    plt.savefig(out_png_file, bbox_inches='tight')
    plt.close()
    print
    print "All done! See", out_png_file
    print

plotNumbers(csv_data)


#plt.xticks(fontsize=fontsize)
#plt.yticks(fontsize=fontsize)
plt.show()
