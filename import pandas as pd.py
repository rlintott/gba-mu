import re
import sys

string = ""

for line in sys.stdin:
    string += line

reg = re.compile(r'<p class="css-axufdj evys1bk0">(.*?)</p>')

story = reg.findall(string)

for para in story:
    print(para)
    print(" ")