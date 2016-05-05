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

string = ""

for i in xrange(len(text)):
  string += "insert " + text[i] + " 1\n"

print string