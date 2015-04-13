#!/usr/bin/env python

from openpyxl import load_workbook

wb = load_workbook('opcodes.xlsx')

class Opcode(object):
    def __init__(self, opcode, name, mode, bytes, cycles, crossing):
        pass

ws = wb.active
ops = []

modes = {
        None:"UNKNOWN", 'Implied':'IMP', '(Indirect,X)':'INDX',
        '(Indirect),Y':'INDY', 'Accumulator':'ACC', 'Indirect':'IND',
        'Absolute':'ABS', 'Absolute,X':'ABSX','Absolute,Y':'ABSY',
        'Immediate':'IMD', 'Relative':'REL', 'Zero Page':'ZP',
        'Zero Page,X':'ZPX', 'Zero Page,Y':'ZPY'
        }

for row in ws.rows[1:]:
    op = {}
    op['code'] = "0x%X" % int(row[0].value)
    op['name'] = row[2].value
    op['mode'] = modes[row[3].value]
    cycles = row[5].value
    op['cycles'] = int(0 if not cycles else cycles)
    bytes = row[4].value
    op['bytes'] = int(0 if not bytes else bytes)
    op['value'] = int(row[0].value)
    op['modify'] = int(0 if not row[9].value else 1)
    ops.append(op)
    
print 'enum mos6502_modes {'''
c_modes = modes.values()
c_modes.sort()
for i in c_modes:
    print '\tMODE_%s,' % i
print '};'
print '''struct op {
\tchar *name;
\tint mode;
\tint bytes;
\tint modify;
} ops[] = {'''

for op in ops:
    print '\t{"%s", MODE_%s, %i, %i}, //%s, %i' % (op['name'], op['mode'], op['bytes'], op['modify'], op['code'], op['value'])

print '};';
