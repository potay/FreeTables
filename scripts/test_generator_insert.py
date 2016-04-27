import random

n = 1000000
count = 0
difference = 0
keys = []
output = ""
for i in xrange(n):
  if (random.random() < 0):
    random.shuffle(keys)
    for j in xrange(random.randint(0,len(keys))):
      key = keys.pop()
      output += "remove %d %d\n" % (key, len(keys))
  else:
    chosen_key = random.randint(1, n)
    while (chosen_key in keys):
      chosen_key = random.randint(1, n)
    keys.append(chosen_key)
    output +=  "insert %d %d %d\n" % (chosen_key, i, len(keys))

print output