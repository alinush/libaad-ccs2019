#!/usr/bin/env python2.7

import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter
from matplotlib.dates import MonthLocator, DateFormatter, DayLocator, epoch2num, num2date
import pandas
import sys
import time

c = 1

plotEverything = False
if len(sys.argv) < 3:
    print "Usage:", sys.argv[0], "<output-png-file> <csv-file> [<csv-file>] ..."
    sys.exit(0)

del sys.argv[0] # that's the program's name

out_png_file = sys.argv[0]
del sys.argv[0]
if not out_png_file.endswith('.png'):
    print "ERROR: Expected .png file as first argument"
    sys.exit(1)

print sys.argv

data_files = [f for f in sys.argv]

print "Reading CSV files:", data_files, "..."

csv_data = pandas.concat((pandas.read_csv(f) for f in data_files))
csv_data_by_batch_size = {}
batch_sizes = csv_data.batchSize.unique()
batch_sizes.sort()
for bs in batch_sizes:
    csv_batch = csv_data[csv_data.batchSize == bs]
    csv_batch['cumulativeAvgTime'] = csv_batch.appendTimeMillisec.expanding().mean()
    csv_batch.cumulativeAvgTime /= 1000;
    csv_data_by_batch_size[bs] = csv_batch

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
    fig, ax1= plt.subplots(figsize=(12,7)) 
    #ax1.set_title('AAD cumulative average append time') #, fontsize=fontsize)
    #ax1.set_xlabel("Number of key-value pairs"); #, fontsize=fontsize)
    ax1.set_ylabel("Cumulative average append time (secs)") #, fontsize=fontsize)
    ax1.set_xscale("log", basex=2);

    plots = []
    names = []
    
    for bs in batch_sizes:
        #plt.xticks(x, x, rotation=30)
        print "Plotting batch size", str(bs), "..."
        col1 = data[bs].cumulativeAvgTime
        x = data[bs].dictSize.unique()    # x-axis is the AAD size
        #print col1
        #print

        assert len(x) == len(col1)

        plt1, = ax1.plot(x, col1)
        plots.append(plt1)
        names.append('batch size ' + str(bs))

    #ax1.set_xticks(x_ticks)
    #ax1.set_xticklabels(x, rotation=30)
    ax1.legend(plots,
               names,
               loc='upper left')#, fontsize=fontsize

    plt.tight_layout()
    #date = time.strftime("%Y-%m-%d-%H-%M-%S")
    #out_png_file = 'append-time-' + date + '.png'
    plt.savefig(out_png_file, bbox_inches='tight')
    plt.close()
    print
    print "All done! See", out_png_file
    print

plotNumbers(csv_data_by_batch_size)


#plt.xticks(fontsize=fontsize)
#plt.yticks(fontsize=fontsize)
plt.show()
