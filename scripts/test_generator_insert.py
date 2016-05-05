import random

n = 100000
count = 0
difference = 0
output = ""
keys = range(n)
random.shuffle(keys)
for i in xrange(n):
  # if (random.random() < 0):
  #   random.shuffle(keys)
  #   for j in xrange(random.randint(0,len(keys))):
  #     key = keys.pop()
  #     output += "remove %d %d\n" % (key, len(keys))
  # else:
  # while (chosen_key in keys):
  #   chosen_key = random.randint(1, n)
  output +=  "insert %d %d\n" % (keys[i], keys[i])

print output