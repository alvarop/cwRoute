#!/usr/bin/env python
# This script takes a list of files as an input (it only takes    
# files named 'energies.csv') Change if needed...
# finds all energies.csv files in the data directory and plots in svg and png
# sample usage: find data/ | ./plotcsv.py
#

import csv
import fileinput
import string
import os
max_devices = 8
sample_rate = 5 # in Hz
for line in fileinput.input():
    if 'energies.csv' in line:      
      line = line.replace('\n','')
      dirs = line.rsplit('/')
      #print(dirs[len(dirs)-2])
      
      datafilename = 'tmp/'+ dirs[len(dirs)-2] + '.dat'
      scriptfilename = 'tmp/'+ dirs[len(dirs)-2] + '.scr'
      
      # generate data file in gnuplot format
      datafile = open(datafilename,'w')

      #open csv file to read
      with open(line, 'rb') as f:
        reader = csv.reader(f)
        x = 0
        for row in reader:
          datafile.write(str(x/sample_rate))
          datafile.write(' ')
          for item in row:
            datafile.write(item)
            datafile.write(' ')
          x = x + 1
          datafile.write('\n')
      datafile.close()
      
      # generate gnuplot script to process file
      print "Processing ", line
      scriptfile = open( scriptfilename,'w')
      
      scriptfile.write("# gnuplot script for '" + datafilename + "'\n")  
      scriptfile.write("set title '" + dirs[len(dirs)-2] +  "'\n")
      scriptfile.write("set xlabel 'Time (s)'\n")
      scriptfile.write("set ylabel 'Power Used (mW)'\n")
      scriptfile.write("plot '" + datafilename + "' using 1:2 with lines title 'Average'\n")
      for ed in range(1,max_devices+1):
        scriptfile.write("replot '" + datafilename + "' using 1:" + str(ed + 2) + " with lines title 'ED" + str(ed) + "'\n")
      scriptfile.write("set terminal svg size 640,480\n")
      scriptfile.write("set output 'plots/svg/" + dirs[len(dirs)-2] + ".svg'\n")
      scriptfile.write("replot\n")
      scriptfile.write("set terminal png size 640,480\n")
      scriptfile.write("set output 'plots/png/" + dirs[len(dirs)-2] + ".png'\n")
      scriptfile.write("replot\n")
      
      scriptfile.close()
      
      os.system("gnuplot " + scriptfilename)
      
      #os.remove(datafilename)
      #os.remove(scriptfilename)
