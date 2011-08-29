# Generate a table of RSSI values according to cc2500 datasheet specifications
# More information can be found in section 17.3 of the datasheet
# RSSI values are in dBm with 0.5 dBm resolution
# Encoded using 2's comlpement numbers
p = range(0,256)
s = 'energy_t rssi_table[255] = { '
for m in p:
  if m >= 128:
    s += repr((m-256)/2.0-72.0)+ ','
  else:
    s += repr((m)/2.0-72.0) + ','
  if (m % 10) == 0 and m > 0:
    s += '\n'
s += ' };'

print s

