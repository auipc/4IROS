import subprocess
import os
path = os.path.dirname(os.path.realpath(__file__))
# I'm not parsing dwarf eff u
proc = subprocess.Popen(['readelf', '--symbols', '-W', os.path.join(path,'build/kernel/4IROS')], stdout=subprocess.PIPE)
lines = []
line = proc.stdout.readline()
while line:
    lines.append(line)
    line = proc.stdout.readline()

clean = [x.decode().rstrip() for x in lines if b'FUNC' in x]
for v in clean:
    lol = [x for x in v.split(' ') if len(x) > 0]
    lol = lol[1:]
    newlol = lol[0]
    for x in range(len(lol[0])):
        if lol[0][x] == '0':
            newlol = lol[0][x+1:]
            continue
        break
    lol[0] = newlol
    del lol[2]
    del lol[2]
    del lol[2]
    del lol[2]

    print(','.join(lol))
