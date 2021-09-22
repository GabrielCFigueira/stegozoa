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

def plot(dist, savefile):

    fig = plt.figure()

    plt.title(title)
    plt.boxplot(dist)

    fig.savefig(savefile)
    plt.close(fig)


def plot3(dists, savefile, labels):

    fig = plt.figure()

    plt.boxplot(dists, labels=labels)

    fig.savefig(savefile)
    plt.close(fig)

            

if __name__ == "__main__":

    baseline = "Chat"

    network_condition = "delay_50"
    stegozoa_cap_folder = log_folder + "StegozoaTraffic" + "/" + baseline + "/" + network_condition
    throughput = computeThroughput(stegozoa_cap_folder)
    plot(throughput, "Throughput.pdf")


    network_conditions = ["delay_50-bw_1500", "delay_50-bw_750", "delay_50-bw_250"]
    labels = ["1500Kbps", "750Kbps", "250Kbps"]
    throughputs = []
    for network_condition in network_conditions:
        stegozoa_cap_folder = log_folder + "StegozoaTraffic" + "/" + baseline + "/" + network_condition
        throughput = computeThroughput(stegozoa_cap_folder)
        throughputs += [throughput]
    plot3(throughputs, "Throughput_bw.pdf", labels)

    network_conditions = ["delay_50-loss_2", "delay_50-loss_5", "delay_50-loss_10"]
    labels = ["2% loss", "5% loss", "10% loss"]
    throughputs = []
    for network_condition in network_conditions:
        stegozoa_cap_folder = log_folder + "StegozoaTraffic" + "/" + baseline + "/" + network_condition
        throughput = computeThroughput(stegozoa_cap_folder)
        throughputs += [throughput]
    plot3(throughputs, "Throughput_loss.pdf", labels)
    
