import re
import string
import random
import copy

with open("./obama.txt", "r") as f:
  text = f.read()

regex = re.compile('[,\.!?:()-;"\']')
text = regex.sub('', text)
regex = re.compile('[\n]')
text = regex.sub(' ', text)
regex = re.compile('[\ {2}]')
text = regex.sub(' ', text)
text = text.replace("  ", " ")
text = text.split()
text = dict(enumerate(text))
currtext = text.keys()
inserted_temp = []
inserted = []

prob_delete = 0.15
prob_search = 0.10
n = 10000
string = ""

for i in xrange(n):
  if len(inserted_temp) > 10:
    movekey = random.choice(inserted_temp)
    inserted.append(movekey)
    inserted_temp.remove(movekey)

  toss = random.random()

  if (len(inserted) > 0 and toss < prob_delete+prob_search) or (len(currtext) == 0):
    if (toss < prob_delete):
      key = random.choice(inserted)
      currtext.append(key)
      string += "remove " + str(key) + "\n"
      inserted.remove(key)
    else:
      key = random.choice(inserted)
      string += "search " + str(key) + " 1\n"
  else:
    key = random.choice(currtext)
    inserted_temp.append(key)
    string += "insert " + str(key) + " " + str(text[key]) + "\n"
    currtext.remove(key)

print string