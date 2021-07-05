import sys

logfilename = sys.argv[1]

with open(logfilename, "rt") as logfile:
    lines = logfile.readlines()

    totalPsnr = 0
    totalSsim = 0
    n = 0
    for line in lines:
        words = line.split(" ")
        if words[0] == "Frame:":
            totalPsnr += float(words[4][:-1])
            totalSsim += float(words[6])
            n += 1
            


