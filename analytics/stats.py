import sys



log_folder = "/home/vagrant/SharedFolder/StegozoaCaps/"


def computePsnrSsim(cap_folder):

    log_list = os.listdir(cap_folder)
    psnrList = []
    ssimList = []

    for log_filename in log_list:

        with open(log_filename, "rt") as logfile:
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

            psnrList += [totalPsnr / n]
            ssimList += [totalSsim / n]

    return psnrList, ssimList
            

if __name__ == "__main__":

    baseline = "Chat"
    network_condition = "regular.regular"

    regular_cap_folder = log_folder + "/" + "RegularTraffic" + "/" + baseline + "/" + network_condition
    stegozoa_cap_folder = log_folder + "/" + "RegularTraffic" + "/" + baseline + "/" + network_condition

    stegoPsnrs, stegoSsims = computePsnrSsim(stegozoa_cap_folder)
    regularPsnrs, regularSsims = computePsnrSsim(regular_cap_folder)

    print(stegozoaPsnrs)
    print(stegoSsims)
    print(regularPsnrs)
    print(regularSsims)
