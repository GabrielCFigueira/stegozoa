import os
import numpy as np
import matplotlib.pyplot as plt
from scipy.stats import norm


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
            for line in lines:
                words = line.split(" ")
                if len(words) == 6 and words[0] == "Frame:":
                    totalPsnr += float(words[3][:-1])
                    totalSsim += float(words[5])
                    n += 1
            
            if n > 0:

                psnrList += [totalPsnr / n]
                ssimList += [totalSsim / n]

    return psnrList, ssimList

def plot(dist, savefile, x_label):
    mean = np.mean(dist)
    sd = np.std(dist)


    fig = plt.figure()
    pdf = norm.pdf(dist, mean, sd)

    plt.title(savefile)
    plt.scatter(dist, pdf, color = 'red', label = x_label + r' = %0.2f $\pm$ %0.3f' % (mean, sd))
    plt.xlabel(x_label)
    plt.ylabel('Probability Density')

    fig.savefig(savefile)
    plt.close(fig)


            

if __name__ == "__main__":

    baseline = "Chat"
    network_condition = "regular.regular"

    regular_cap_folder = log_folder + "RegularTraffic" + "/" + baseline + "/" + network_condition
    stegozoa_cap_folder = log_folder + "StegozoaTraffic" + "/" + baseline + "/" + network_condition

    stegoPsnrs, stegoSsims = computePsnrSsim(stegozoa_cap_folder)
    regularPsnrs, regularSsims = computePsnrSsim(regular_cap_folder)


    plot(stegoPsnrs, "stegoPSNR.pdf", "PSNR")
    plot(stegoSsims, "stegoSSIM.pdf", "SSIM")
    plot(regularPsnrs, "regularPSNR.pdf", "PSNR")
    plot(regularSsims, "regularSSIM.pdf", "SSIM")

    
