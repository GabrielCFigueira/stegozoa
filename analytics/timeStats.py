import os
import numpy as np
import matplotlib.pyplot as plt


log_folder = "/home/vagrant/SharedFolder/StegozoaCaps/"


def computeTimes(cap_folder):

    log_list = os.listdir(cap_folder)
    Embedding = []
    Encoding = []

    for log_filename in log_list:

        with open(cap_folder + "/" + log_filename, "rt") as logfile:
            lines = logfile.readlines()

            totalEmbeddingTime = 0.0
            totalEncodingTime = 0.0
            nEmbbed = 0
            nEncode = 0
            for i in range(100, len(lines)): # delete first 100 (video call is not in a stable state at the beginning
                words = lines[i].split(" ")

                if len(words) >= 7: # both have 7 or more words

                    if words[2] == "embedding":
                        try:
                            totalEmbeddingTime += float(words[8])
                        except:
                            continue
                    elif words[2] == "encoding":
                        try:
                            totalEncodingTime += float(words[6])
                        except:
                            continue
            
            if nEmbbed > 0 and nEncode > 0:

                Embedding += [totalEmbeddingTime / n]
                Embedding += [totalEncodingTime / n]

    return Embedding, Encoding

def plot(stegoDist, regularDist, savefile, title):


    fig = plt.figure()

    plt.title(title)
    plt.boxplot([stegoDist, regularDist], labels=["Stegozoa", "Regular"])

    fig.savefig(savefile)
    plt.close(fig)


            

if __name__ == "__main__":

    baseline = "Chat"
    network_condition = "regular.regular"

    regular_cap_folder = log_folder + "RegularTraffic" + "/" + baseline + "/" + network_condition
    stegozoa_cap_folder = log_folder + "StegozoaTraffic" + "/" + baseline + "/" + network_condition

    stegoEmbedding, stegoEncoding = computeTimes(stegozoa_cap_folder)
    _, regularEncoding = computePsnrSsim(regular_cap_folder)


    plot(stegoEncoding, regularEncoding, "EncodingTimes.pdf", "Encoding times comparision")
    plot(stegoSsims, [], "EmbeddingTimes.pdf", "Embedding times")

    
