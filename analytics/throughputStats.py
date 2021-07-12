import os
import numpy as np
import matplotlib.pyplot as plt


log_folder = "/home/vagrant/SharedFolder/StegozoaCaps/"


def computeThroughput(cap_folder):

    log_list = os.listdir(cap_folder)
    Speed = []

    for log_filename in log_list:

        if not log_filename.__contains__("download_log"):
            continue

        with open(cap_folder + "/" + log_filename, "rt") as logfile:
            lines = logfile.readlines()

            for line in lines:
                words = line.split(" ")

                if len(words) == 2:

                    if words[0] == "Throughput(bits/s):":
                        try:
                            speed = float(words[1])
                        except ValueError:
                            break
                        Speed += [speed]


    return Speed

def plot(dist, savefile, title):


    fig = plt.figure()

    plt.title(title)
    plt.boxplot(dist)

    fig.savefig(savefile)
    plt.close(fig)


            

if __name__ == "__main__":

    baseline = "Chat"
    network_condition = "regular.regular"

    regular_cap_folder = log_folder + "RegularTraffic" + "/" + baseline + "/" + network_condition

    throughputs = computeThroughput(stegozoa_cap_folder)


    plot(throughputs, "Throughput.pdf", "Stegozoa throughput")

    
