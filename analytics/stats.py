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
            totalPsnr += float(words[3][:-1])
            totalSsim += float(words[5])
            n += 1

    print("Average PSNR: " + str(totalPsnr / n))
    print("Average SSIM: " + str(totalSsim / n))
            


