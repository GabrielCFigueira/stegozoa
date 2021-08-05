import os
import numpy as np
import matplotlib.pyplot as plt


log_folder = "/home/vagrant/SharedFolder/StegozoaCaps/"

def getRes(w, h):
    w = w[2:]
    h = h[2:]
    return w + "x" + h

def computePsnrSsim(cap_folder):

    log_list = os.listdir(cap_folder)
    psnrList = []
    ssimList = []

    resDict = {}

    for log_filename in log_list:
        
        if not log_filename.__contains__("chromium_log"):
            continue

        with open(cap_folder + "/" + log_filename, "rt") as logfile:
            lines = logfile.readlines()

            totalPsnr = 0
            totalSsim = 0
            n = 0
            for i in range(1000, len(lines)): # delete first 1000 (video call is not in a stable state at the beginning
                words = lines[i].split(" ")
                if len(words) == 6 and words[0] == "Frame:":
                    try:
                        psnr = float(words[3][:-1])
                        ssim = float(words[5])
                    except:
                        continue
                    totalPsnr += psnr
                    totalSsim += ssim
                    n += 1
                elif len(words) == 5 and words[1] == "Resolution":
                    try:
                        res = getRes(words[3], words[4])
                    except:
                        continue

                    if res not in resDict:
                        resDict[res] = 1
                    else:
                        resDict[res] += 1

            
            if n > 0:

                psnrList += [totalPsnr / n]
                ssimList += [totalSsim / n]

                
    for key in list(resDict.keys()):
        if resDict[key] < 100:
            del(resDict[key])
    return psnrList, ssimList, resDict

def plot(stegoDist, regularDist, savefile, title):


    fig = plt.figure()

    plt.title(title)
    plt.boxplot([stegoDist, regularDist], labels=["Stegozoa", "Regular"])

    fig.savefig(savefile)
    plt.close(fig)

def barChart(resDict, savefile, title):

    fig = plt.figure()

    plt.title(title)
    plt.bar(resDict.keys(), resDict.values())

    fig.savefig(savefile)
    plt.close(fig)


            

if __name__ == "__main__":

    baseline = "Chat"
    network_condition = "regular.regular"

    regular_cap_folder = log_folder + "RegularTraffic" + "/" + baseline + "/" + network_condition
    stegozoa_cap_folder = log_folder + "StegozoaTraffic" + "/" + baseline + "/" + network_condition

    stegoPsnrs, stegoSsims, stegoRes = computePsnrSsim(stegozoa_cap_folder)
    regularPsnrs, regularSsims, regularRes = computePsnrSsim(regular_cap_folder)


    plot(stegoPsnrs, regularPsnrs, "PSNR.pdf", "PSNR comparision")
    plot(stegoSsims, regularSsims, "SSIM.pdf", "SSIM comparision")
    barChart(stegoRes, "stegoRes.pdf", "Stegozoa Resolutions")
    barChart(regularRes, "regularRes.pdf", "Regular Resolutions")
    
