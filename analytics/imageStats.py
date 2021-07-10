import os
import numpy as np
import matplotlib.pyplot as plt


log_folder = "/home/vagrant/SharedFolder/StegozoaCaps/"


def computePsnrSsim(cap_folder):

    log_list = os.listdir(cap_folder)
    psnrList = []
    ssimList = []

    for log_filename in log_list:

        with open(cap_folder + "/" + log_filename, "rt") as logfile:
            lines = logfile.readlines()

            totalPsnr = 0
            totalSsim = 0
            n = 0
            for i in range(100, len(lines)): # delete first 100 (video call is not in a stable state at the beginning
                words = lines[i].split(" ")
                if len(words) == 6 and words[0] == "Frame:":
                    totalPsnr += float(words[3][:-1])
                    totalSsim += float(words[5])
                    n += 1
            
            if n > 0:

                psnrList += [totalPsnr / n]
                ssimList += [totalSsim / n]

    return psnrList, ssimList

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

    stegoPsnrs, stegoSsims = computePsnrSsim(stegozoa_cap_folder)
    regularPsnrs, regularSsims = computePsnrSsim(regular_cap_folder)


    plot(stegoPsnrs, regularPsnrs, "PSNR.pdf", "PSNR comparision")
    plot(stegoSsims, regularSsims, "SSIM.pdf", "SSIM comparision")

    