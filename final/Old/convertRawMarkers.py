#! /usr/local/bin/python3

# converts raw pasted markers from Reaper to simple timings of downbeats, one per line

with open("Markers.txt") as f:
    content = f.readlines()

with open("downbeats.txt", "w") as f:
    for line in content:
        f.write(line.split(" ")[1] + "\n")
