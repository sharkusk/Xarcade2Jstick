import re
f = open('/usr/include/linux/input-event-codes.h')
out = open('./input_event_code_array.c', 'w')

text = f.read()
f.close()

p = re.compile(r'(KEY_\w+)\s+(\d\w*)')
q = re.compile(r'(BTN_\w+)\s+(\d\w*)')
r = re.compile(r'(EV_\w+)\s+(\d\w*)')

keys = p.findall(text)
buttons = q.findall(text)
events = r.findall(text)

out.write('/* autogenerated file */\n')
out.write('#include <linux/input.h>\n')
out.write('#include "input_event_code_array.h"\n')
out.write('struct input_mapping ieca[] = {\n')

for key in keys:
    out.write(f'    {{.name = "{key[0]}", .code = {key[0]}}}, /* {key[1]} */\n')

for button in buttons:
    out.write(f'    {{.name = "{button[0]}", .code = {button[0]}}}, /* {button[1]} */\n')

for event in events:
    out.write(f'    {{.name = "{event[0]}", .code = {event[0]}}}, /* {event[1]} */\n')

out.write('    {.name = "_END_", .code = 0xFFFF}\n')

out.write('};\n')

out.close()

