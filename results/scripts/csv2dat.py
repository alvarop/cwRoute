#!/usr/bin/env python
# This script takes a list of files as an input (it only takes    
# files named 'energies.csv') Change if needed...
# It replaces commas with spaces and ads a time as the first column
# assuming a 5 Hz sampling rate
# sample usage: find . | ./csv2dat.py

import csv
import fileinput
for line in fileinput.input():
    if '.csv' in line:
      line = line.replace('\n','')
      print "Processing ", line
      #open output file
      outfile = open(line.replace('csv','dat'),'w')

      #open csv file to read
      with open(line, 'rb') as f:
        reader = csv.reader(f)
        x = 0
        for row in reader:
          outfile.write(str(x*.2))
          outfile.write(' ')
          for item in row:
            outfile.write(item)
            outfile.write(' ')
          x = x + 1
          outfile.write('\n')
      outfile.close()

