# Generate a table of RSSI values according to cc2500 datasheet specifications
# More information can be found in section 17.3 of the datasheet
# RSSI values are in dBm with 0.5 dBm resolution
# Encoded using 2's comlpement numbers
from numpy import *

# Convert rssi value from dBm to watts
def dbm_to_watt( p ):
  return (10.0**(p/10.0))/1000.0
  
def watt_to_dbm( p ):
  return 10.0 * log10(1000.0 * p)

target_rssi = dbm_to_watt(-80)
tx_power = dbm_to_watt(1)

s = 'energy_t rssi_table[255] = { '
w = zeros(256)
index = 0;
for m in range(0,256):
  if m >= 128:
    s += repr((m-256)/2.0-72.0)+ ','
    w[index] = ((m-256)/2.0-72.0)
  else:
    s += repr((m)/2.0-72.0) + ','
    w[index] = ((m)/2.0-72.0)
  if (m % 10) == 0 and m > 0:
    s += '\n'
  index += 1
s += ' };'

#print s

a = dbm_to_watt(w)/tx_power
#print a
l = (target_rssi/a)
#print l
print 'rssi(dBm),rssi(W),alpha,tx_power(dBm),tx_power(W)'
for n in range(0,256):
  print w[n],',', dbm_to_watt(w[n]), ',', a[n] ,',' , round(watt_to_dbm(l[n]),1),',', (l[n])
