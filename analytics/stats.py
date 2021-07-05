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
            for line in lines:
                words = line.split(" ")
                if len(words) == 6 and words[0] == "Frame:":
                    totalPsnr += float(words[3][:-1])
                    totalSsim += float(words[5])
                    n += 1
            
            if n > 0:
                print("Average PSNR: " + str(totalPsnr / n))
                print("Average SSIM: " + str(totalSsim / n))

                psnrList += [totalPsnr / n]
                ssimList += [totalSsim / n]

    return psnrList, ssimList

def normal_dist(x, mean, sd):
    prob_density = (np.pi * sd) * np.exp(-0.5 * ((x-mean) / sd) ** 2)
    return prob_density

def plot(dist):
    mean = np.mean(dist)
    sd = np.std(dist)

    pdf = normal_dist(dist,mean,sd)

    plt.plot(x,pdf , color = 'red')
    plt.xlabel('Data points')
    plt.ylabel('Probability Density')


            

if __name__ == "__main__":

    baseline = "Chat"
    network_condition = "regular.regular"

    regular_cap_folder = log_folder + "RegularTraffic" + "/" + baseline + "/" + network_condition
    stegozoa_cap_folder = log_folder + "StegozoaTraffic" + "/" + baseline + "/" + network_condition

    stegoPsnrs, stegoSsims = computePsnrSsim(stegozoa_cap_folder)
    regularPsnrs, regularSsims = computePsnrSsim(regular_cap_folder)


    plot(stegoPsnrs)
    plot(stegoSsims)
    plot(regularPsnrs)
    plot(regularPsnrs)

    
