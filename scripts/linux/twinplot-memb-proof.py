#!/usr/bin/env python

import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter
from matplotlib.dates import MonthLocator, DateFormatter, DayLocator, epoch2num, num2date
import pandas
import sys
import time

c = 1

if len(sys.argv) < 3:
    print "Usage:", sys.argv[0], "<output-png-file> <csv-file> [<csv-file>] ..."
    sys.exit(0)

del sys.argv[0]

out_png_file = sys.argv[0]
del sys.argv[0]

if not out_png_file.endswith('.png'):
    print "ERROR: Expected .png file as first argument"
    sys.exit(1)

data_files = [f for f in sys.argv]

print "Reading CSV files:", data_files, "..."

csv_data = pandas.concat((pandas.read_csv(f) for f in data_files))

#print "Raw:"
#print csv_data.to_string()
#print csv_data.columns
#print csv_data['dictSize'].values

#print "Averaged:"
#print csv_data.groupby(['dictSize', 'numValues']).groups.keys() # prints all (dictSize, numValues) pairs from the data
#csv_data = csv_data.groupby(['dictSize', 'numValues'], as_index=True ).mean()
csv_data = csv_data.groupby(['dictSize', 'numValues'], as_index=False).mean()
#print csv_data[csv_data.dictSize == 1023]; # filter results by dictionary size

csv_data.forestBytes /= 1024;
csv_data.frontierBytes /= 1024;
csv_data.totalBytes /= 1024;
csv_data.verifyUsec /= 1000*1000;

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
    x = csv_data.dictSize.unique() # x-axis is the AAD size
    numValues = csv_data.numValues.unique()
    #print sizes
    #print numValues

    logBase = 10
    for size in x:
        if size % 10 != 0:
            print "Switching to log_2"
            logBase = 2
            break

    print "Plotting with log base", logBase 

    #fig, (ax1, ax2) = plt.subplots(2, figsize=(10.5,12))
    fig, ax1 = plt.subplots(figsize=(15, 9))
    ax1.set_title('AAD complete membership proofs') #, fontsize=fontsize)
    #ax2.set_title('Complete membership proof verification time') #, fontsize=fontsize)

    ax2 = ax1.twinx()
    ax1.set_xscale("log", basex=logBase);
    ax2.set_xscale("log", basex=logBase);
    ax1.set_ylabel("Proof size (in KB)") #, fontsize=fontsize)
    ax2.set_ylabel("Verification time (in secs)") #, fontsize=fontsize)
    ax2.set_xlabel("Number of key-value pairs")

    plots1 = []
    plots2 = []
    names1 = []
    names2 = [] 
    
    #plt.xticks(x, x, rotation=30)
    for nv in numValues:
        if nv not in [0, 1, 4, 8, 32]:
            continue
        filtered = data[data.numValues == nv]
        col1 = filtered.totalBytes.values
        col2 = filtered.verifyUsec.values
        #print col1
        #print

        assert len(x) == len(col1)
        assert len(x) == len(col2)

        # see https://matplotlib.org/2.0.2/api/lines_api.html
        # https://matplotlib.org/api/_as_gen/matplotlib.pyplot.plot.html
        plt1, = ax1.plot(x, col1, linestyle="-") #, marker="o")
        plt2, = ax2.plot(x, col2, linestyle=":", marker="x")
        plots1.append(plt1)
        plots2.append(plt2)
        names1.append(str(nv) + " values")
        names2.append(str(nv) + " values")

    ax1.set_xticks(x)
    ax2.set_xticks(x)
    ax1.set_xticklabels(x, rotation=30)
    ax2.set_xticklabels(x, rotation=30)

    ax1.legend(plots1,
               names1,
               loc='upper left')#, fontsize=fontsize

    ax2.legend(plots2,
               names2,
               loc='lower left')#, fontsize=fontsize

    plt.tight_layout()
    #date = time.strftime("%Y-%m-%d-%H-%M-%S")
    #out_png_file = 'membership-proofs-' + date + '.png'
    plt.savefig(out_png_file)
    plt.close()

plotNumbers(csv_data)

#plt.xticks(fontsize=fontsize)
#plt.yticks(fontsize=fontsize)
plt.show()
