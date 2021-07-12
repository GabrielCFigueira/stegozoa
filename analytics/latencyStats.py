import os
import numpy as np
import matplotlib.pyplot as plt


log_folder = "/home/vagrant/SharedFolder/StegozoaCaps/"


def computeRTTs(cap_folder):

    log_list = os.listdir(cap_folder)
    RTT = []

    for log_filename in log_list:

        if not log_filename.__contains__("ping_log"):
            continue

        with open(cap_folder + "/" + log_filename, "rt") as logfile:
            lines = logfile.readlines()

            for line in lines:
                words = line.split(" ")
                if len(words) == 3:
                    if words[0] == "Average":
                        try:
                            rtt = float(words[2])
                        except ValueError:
                            break
                        RTT += [rtt]

    return RTT

def plot(dist, savefile, title):


    fig = plt.figure()

    plt.title(title)
    plt.boxplot(dist)

    fig.savefig(savefile)
    plt.close(fig)


            

if __name__ == "__main__":

    baseline = "Chat"
    network_condition = "regular.regular"

    stegozoa_cap_folder = log_folder + "StegozoaTraffic" + "/" + baseline + "/" + network_condition

    RTT = computeRTTs(stegozoa_cap_folder)


    plot(RTT, "RTT.pdf", "Stegozoa RTT")
