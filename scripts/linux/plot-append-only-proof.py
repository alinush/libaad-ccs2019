#!/usr/bin/env python2.7

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
del csv_data['oldDictSize'] # don't need this column for our plots

#print "Raw:"
#print csv_data.to_string()
#print csv_data.columns
#print csv_data['dictSize'].values

#print "Averaged:"
csv_data = csv_data.groupby(['newDictSize'], as_index=False).mean()
#print csv_data[csv_data.dictSize == 1023]; # filter results by dictionary size

csv_data.proofBytes /= 1024    # bytes to KB
csv_data.verifyUsec /= 1000     # usecs to millisecs

print csv_data.to_string()  # print all data

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
    x = csv_data.newDictSize    #.unique() # x-axis is the AAD size
    #print sizes

    logBase = 10
    for size in x:
        if size % 10 != 0:
            print "Switching to log_2"
            logBase = 2
            break
    
    print "Plotting with log base", logBase 

    fig, ax1 = plt.subplots(figsize=(12,7.5))
    ax2 = ax1.twinx()
    ax1.set_xscale("log", basex=logBase);
    ax2.set_xscale("log", basex=logBase);
    #ax1.set_title('AAD append-only proofs') #, fontsize=fontsize)
    ax1.set_ylabel("Proof size (in KB)") #, fontsize=fontsize)
    #ax2.set_title('Append-only proof verification time') #, fontsize=fontsize)
    ax2.set_ylabel("Verification time (in millisecs)") #, fontsize=fontsize)
    #ax1.set_xlabel("Number of key-value pairs")
    
    #plt.xticks(x, x, rotation=30)
    col1 = data.proofBytes.values
    col2 = data.verifyUsec.values
    #print col1
    #print

    assert len(x) == len(col1)
    assert len(x) == len(col2)

    plt1, = ax1.plot(x, col1, linestyle="-", marker="o")
    plt2, = ax2.plot(x, col2, linestyle=":", marker="x")

    ax1.set_xticks(x)
    #ax2.set_xticks(x)
    ax1.set_xticklabels(x, rotation=30)
    #ax2.set_xticklabels(x, rotation=30)

    ax1.legend([plt1, plt2],
               ["Proof size", "Verification time"],
               loc='upper left')#, fontsize=fontsize

    plt.tight_layout()
    #date = time.strftime("%Y-%m-%d-%H-%M-%S")
    #out_png_file = 'append-only-proofs-' + date + '.png'
    plt.savefig(out_png_file, bbox_inches='tight')
    plt.close()

plotNumbers(csv_data)

#plt.xticks(fontsize=fontsize)
#plt.yticks(fontsize=fontsize)
plt.show()
