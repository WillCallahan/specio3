import specio3
import os
import csv

if __name__ == '__main__':
    print("Reading SPC file...")
    data = specio3.read_spc('/Users/william/PycharmProjects/specio3/tests/data/103b4anh.spc')[0]

    print(f"X array length: {len(data[0])}")
    print(f"Y array length: {len(data[1])}")
    print(f"First 5 X values: {data[0][:5]}")
    print(f"First 5 Y values: {data[1][:5]}")
    print(f"X range: {data[0][0]} to {data[0][-1]}")
    print(f"Y range: {min(data[1])} to {max(data[1])}")

    with open('/Users/william/git/amazon-bedrock-workshop/output.csv', mode = 'w', newline = '') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['x', 'y'])  # Header
        zipped = zip(data[0], data[1])
        for x, y in zipped:
            # writer.writerow([f'{x:.0f}', f'{y}'])
            writer.writerow([x, y])

    print(f"CSV file written with {len(data[0])} data points")
