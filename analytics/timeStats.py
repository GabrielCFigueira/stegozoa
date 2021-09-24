import os
import numpy as np
import matplotlib.pyplot as plt


log_folder = "/home/vagrant/SharedFolder/StegozoaCaps/"


def computeTimes(cap_folder):

    log_list = os.listdir(cap_folder)
    Embedding = []
    Encoding = []

    for log_filename in log_list:

        if not log_filename.__contains__("chromium_log"):
            continue

        with open(cap_folder + "/" + log_filename, "rt") as logfile:
            lines = logfile.readlines()

            totalEmbeddingTime = 0.0
            totalEncodingTime = 0.0
            nEmbbed = 0
            nEncode = 0
            for i in range(2000, len(lines)): # delete first 2000 (video call is not in a stable state at the beginning
                words = lines[i].split(" ")

                if len(words) >= 7: # both have 7 or more words

                    if words[2] == "embbedding" and len(words) >= 9:
                        try:
                            time = float(words[8][:-1])
                        except ValueError:
                            continue
                        totalEmbeddingTime += time
                        nEmbbed += 1
                    elif words[2] == "encoding":
                        try:
                            time = float(words[6])
                        except ValueError:
                            continue
                        totalEncodingTime += time
                        nEncode += 1
            
            if nEmbbed > 0:
                Embedding += [totalEmbeddingTime / nEmbbed]

            if nEncode > 0:
                Encoding += [totalEncodingTime / nEncode]

    return Embedding, Encoding

def plot(stegoDist, regularDist, savefile, title):


    fig = plt.figure()

    plt.title(title)

    if not regularDist:
        plt.boxplot(stegoDist, label=["Stegozoa"], notch=False, showfliers=False, showmeans=True, meanprops={'markerfacecolor': 'slategray', 'markeredgecolor': 'slategray'})
    else:
        plt.boxplot([stegoDist, regularDist], labels=["Stegozoa", "Regular"], notch=False, showfliers=False, showmeans=True, meanprops={'markerfacecolor': 'slategray', 'markeredgecolor': 'slategray'})

    fig.savefig(savefile)
    plt.close(fig)


            

if __name__ == "__main__":

    baseline = "Chat"
    network_condition = "regular.regular"

    regular_cap_folder = log_folder + "RegularTraffic" + "/" + baseline + "/" + network_condition
    stegozoa_cap_folder = log_folder + "StegozoaTraffic" + "/" + baseline + "/" + network_condition

    stegoEmbedding, stegoEncoding = computeTimes(stegozoa_cap_folder)
    _, regularEncoding = computeTimes(regular_cap_folder)


    plot(stegoEncoding, regularEncoding, "EncodingTimes.pdf", "Encoding times comparision")
    plot(stegoEmbedding, [], "EmbeddingTimes.pdf", "Embedding times")

    
