import specio3
import os
import csv

if __name__ == '__main__':
    data = specio3.read_spc('/Users/william/PycharmProjects/specio3/tests/data/cts0829b.spc')[0]
    with open('/Users/william/git/amazon-bedrock-workshop/output.csv', mode = 'w', newline = '') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['x', 'y'])  # Header
        zipped = zip(data[0], data[1])
        for x, y in zipped:
            writer.writerow([x, y])
