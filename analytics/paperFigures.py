#!/usr/bin/env python
import subprocess
import socket
import os
import math
import csv
import numpy as np
import sys
from termcolor import colored

import matplotlib
if os.environ.get('DISPLAY','') == '':
    print('no display found. Using non-interactive Agg backend')
    matplotlib.use('Agg')

import matplotlib.pyplot as plt
from matplotlib.pyplot import cm
from matplotlib.ticker import MultipleLocator, FormatStrFormatter

plt.rcParams['font.family'] = 'sans-serif'
plt.rcParams['font.sans-serif'] = ['Helvetica']

colors = ["slategray",  "lightskyblue", "sandybrown" ,"darkseagreen", "palevioletred", "lightsteelblue", "salmon"]




def PerturbationsAUCPerProfile():

    dataset = "StegozoaCaps"
    
    perturbation_configs = [
        [
        "delay_50-bw_1500/" + "RegularTraffic-StegozoaTraffic/",
        "delay_50-bw_750/" + "RegularTraffic-StegozoaTraffic/",
        "delay_50-bw_250/" + "RegularTraffic-StegozoaTraffic/"],
        [
        "delay_50-loss_2/" + "RegularTraffic-StegozoaTraffic/",
        "delay_50-loss_5/" + "RegularTraffic-StegozoaTraffic/",
        "delay_50-loss_10/" + "RegularTraffic-StegozoaTraffic/"],
        [
        "delay_50/" + "RegularTraffic-StegozoaTraffic/"]
    ]
        

    feature_sets = [
        "PL_0_30_246",
        "Stats_0_30_246"
    ]


    for videoProfile in os.listdir("classificationData/" + dataset):
        if (".DS_Store" in videoProfile):
            continue
        for feature_set in feature_sets:

            for perturbation_config in perturbation_configs:
            
                fig = plt.figure(figsize=[6.4, 5.2])
                ax1 = fig.add_subplot(111)
                

                for n, cfg in enumerate(perturbation_config):
                
                    sensitivity = np.load("classificationData/" + dataset + "/" + videoProfile + "/" + cfg + feature_set + "/ROC_10CV_XGBoost_Sensitivity.npy")
                    specificity = np.load("classificationData/" + dataset + "/" + videoProfile + "/" + cfg + feature_set + "/ROC_10CV_XGBoost_Specificity.npy")

                    auc = np.trapz(sensitivity, specificity)
                    print "stats AUC " + cfg + ": " + str(auc)

                    label_text = "AUC"

                    if(cfg.split("/")[0] == "delay_50-bw_1500"):
                        label_text = "bw 1500Kbps " + ' - AUC = %0.2f' % (auc)
                    elif(cfg.split("/")[0] == "delay_50-bw_750"):
                        label_text = "bw 750Kbps " + ' - AUC = %0.2f' % (auc)
                    elif(cfg.split("/")[0] == "delay_50-bw_250"):
                        label_text = "bw 250Kbps " + ' - AUC = %0.2f' % (auc)

                    elif(cfg.split("/")[0] == "delay_50-loss_2"):
                        label_text = "loss 2% " + ' - AUC = %0.2f' % (auc)
                    elif(cfg.split("/")[0] == "delay_50-loss_5"):
                        label_text = "loss 5% " + ' - AUC = %0.2f' % (auc)
                    elif(cfg.split("/")[0] == "delay_50-loss_10"):
                        label_text = "loss 10% " + ' - AUC = %0.2f' % (auc)

                    elif(cfg.split("/")[0] == "delay_50"):
                        label_text = "RTT 50ms " + ' - AUC = %0.2f' % (auc)


                    ax1.plot(specificity, sensitivity, lw=6, color=colors[n], label = label_text)

                ax1.plot([0, 1], [0, 1], 'k--', lw=2, color="0.0", label = 'Random Guess')
                ax1.yaxis.grid(color='grey', linestyle='dotted', lw=0.2)
                ax1.spines['right'].set_visible(False)
                ax1.spines['top'].set_visible(False)
                plt.xlabel('False Positive Rate', fontsize=26)
                plt.ylabel('True Positive Rate', fontsize=26)
                plt.legend(loc='lower right', frameon=False, handlelength=1.0, fontsize=14)

                plt.setp(ax1.get_xticklabels(), fontsize=20)
                plt.setp(ax1.get_yticklabels(), fontsize=20)
                ax1.set(xlim=(0, 1), ylim=(0.0, 1))
                

                if not os.path.exists("Figures/PerturbationsAUC/" + dataset + "/" + videoProfile):
                    os.makedirs("Figures/PerturbationsAUC/" + dataset + "/" + videoProfile)
                

                if(cfg.split("/")[0] == "delay_50"):
                    perturbation = "latency"

                if(cfg.split("/")[0] == "delay_50-bw_1500" or cfg.split("/")[0] == "delay_50-bw_750" or cfg.split("/")[0] == "delay_50-bw_250"):
                    perturbation = "bandwidth"
                
                if(cfg.split("/")[0] == "delay_50-loss_2" or cfg.split("/")[0] == "delay_50-loss_5" or cfg.split("/")[0] == "delay_50-loss_10"):
                    perturbation = "loss"


                if perturbation == "bandwidth":
                    plot_title = "b) Bandwidth"
                elif perturbation == "loss":
                    plot_title = "c) Packet Loss"

                plt.xlabel('False Positive Rate\n%s'%plot_title, fontsize=24)
                plt.tight_layout()
                fig.savefig("Figures/PerturbationsAUC/" + dataset + "/" + videoProfile + "/" + perturbation + "_" + feature_set + "_ROC_plot.pdf")   # save the figure to file
                plt.close(fig)



def PerturbationsAUCAll():

    dataset = "ProtozoaCaps3March"

    perturbation_configs = [
        [
        "delay_50-bw_1500.delay_15/" + "RegularTraffic-StegozoaTraffic/",
        "delay_50-bw_750.delay_15/" + "RegularTraffic-StegozoaTraffic/",
        "delay_50-bw_250.delay_15/" + "RegularTraffic-StegozoaTraffic/"],
        [
        "delay_50-loss_2.delay_15/" + "RegularTraffic-StegozoaTraffic/",
        "delay_50-loss_5.delay_15/" + "RegularTraffic-StegozoaTraffic/",
        "delay_50-loss_10.delay_15/" + "RegularTraffic-StegozoaTraffic/"],
        [
        "delay_15.delay_15/" + "RegularTraffic-StegozoaTraffic/",
        "delay_50.delay_15/" + "RegularTraffic-StegozoaTraffic/",
        "delay_100.delay_15/" + "RegularTraffic-StegozoaTraffic/"],
        [
        "delay_50-jitter1.delay_15/" + "RegularTraffic-StegozoaTraffic/",
        "delay_50-jitter4.delay_15/" + "RegularTraffic-StegozoaTraffic/",
        "delay_50-jitter8.delay_15/" + "RegularTraffic-StegozoaTraffic/",
        "delay_50-jitter16.delay_15/" + "RegularTraffic-StegozoaTraffic/",
        "delay_50-jitter32.delay_15/" + "RegularTraffic-StegozoaTraffic/"],
        [
        "delay_50.delay_15_last/" + "RegularTraffic-StegozoaTraffic/",
        "delay_50.delay_50_last/" + "RegularTraffic-StegozoaTraffic/",
        "delay_50.delay_100_last/" + "RegularTraffic-StegozoaTraffic/"],
    ]
        

    feature_sets = [
        "PL_0_30_246",
        "Stats_0_30_246"
    ]


    for videoProfile in os.listdir("classificationData/" + dataset):
        if (".DS_Store" in videoProfile):
            continue
        for feature_set in feature_sets:

            for perturbation_config in perturbation_configs:

                for n, cfg in enumerate(perturbation_config):
                    
                    fig = plt.figure()#figsize=[5.4, 5.4])
                    ax1 = fig.add_subplot(111)

                    sensitivity = np.load("classificationData/" + dataset + "/" + videoProfile + "/" + cfg + feature_set + "/ROC_10CV_XGBoost_Sensitivity.npy")
                    specificity = np.load("classificationData/" + dataset + "/" + videoProfile + "/" + cfg + feature_set + "/ROC_10CV_XGBoost_Specificity.npy")

                    auc = np.trapz(sensitivity, specificity)
                    print "stats AUC " + cfg + ": " + str(auc)
                    
                    label_text = "AUC"

                    if(cfg.split("/")[0] == "delay_50-bw_1500.delay_15"):
                        label_text = "bw 1500Kbps " + ' - AUC = %0.2f' % (auc)
                    elif(cfg.split("/")[0] == "delay_50-bw_750.delay_15"):
                        label_text = "bw 750Kbps " + ' - AUC = %0.2f' % (auc)
                    elif(cfg.split("/")[0] == "delay_50-bw_250.delay_15"):
                        label_text = "bw 250Kbps " + ' - AUC = %0.2f' % (auc)

                    elif(cfg.split("/")[0] == "delay_50-loss_2.delay_15"):
                        label_text = "loss 2% " + ' - AUC = %0.2f' % (auc)
                    elif(cfg.split("/")[0] == "delay_50-loss_5.delay_15"):
                        label_text = "loss 5% " + ' - AUC = %0.2f' % (auc)
                    elif(cfg.split("/")[0] == "delay_50-loss_10.delay_15"):
                        label_text = "loss 10% " + ' - AUC = %0.2f' % (auc)

                    elif(cfg.split("/")[0] == "delay_15.delay_15"):
                        label_text = "RTT 15ms " + ' - AUC = %0.2f' % (auc)
                    elif(cfg.split("/")[0] == "delay_50.delay_15"):
                        label_text = "RTT 50ms " + ' - AUC = %0.2f' % (auc)
                    elif(cfg.split("/")[0] == "delay_100.delay_15"):
                        label_text = "RTT 100ms " + ' - AUC = %0.2f' % (auc)

                    elif(cfg.split("/")[0] == "delay_50-jitter1.delay_15"):
                        label_text = "jitter 1ms " + ' - AUC = %0.2f' % (auc)
                    elif(cfg.split("/")[0] == "delay_50-jitter4.delay_15"):
                        label_text = "jitter 4ms " + ' - AUC = %0.2f' % (auc)
                    elif(cfg.split("/")[0] == "delay_50-jitter8.delay_15"):
                        label_text = "jitter 8ms " + ' - AUC = %0.2f' % (auc)
                    elif(cfg.split("/")[0] == "delay_50-jitter16.delay_15"):
                        label_text = "jitter 16ms " + ' - AUC = %0.2f' % (auc)
                    elif(cfg.split("/")[0] == "delay_50-jitter32.delay_15"):
                        label_text = "jitter 32ms " + ' - AUC = %0.2f' % (auc)

                    elif(cfg.split("/")[0] == "delay_50.delay_15_last"):
                        label_text = "last leg 15ms " + ' - AUC = %0.2f' % (auc)
                    elif(cfg.split("/")[0] == "delay_50.delay_50_last"):
                        label_text = "last leg 50ms " + ' - AUC = %0.2f' % (auc)
                    elif(cfg.split("/")[0] == "delay_50.delay_100_last"):
                        label_text = "last leg 100ms " + ' - AUC = %0.2f' % (auc)

                    ax1.plot(specificity, sensitivity, lw=6, color=colors[n], label = label_text)

                    ax1.plot([0, 1], [0, 1], 'k--', lw=2, color="0.0", label = 'Random Guess')
                    ax1.yaxis.grid(color='grey', linestyle='dotted', lw=0.2)
                    ax1.spines['right'].set_visible(False)
                    ax1.spines['top'].set_visible(False)
                    
                    plt.ylabel('True Positive Rate', fontsize=26)
                    plt.legend(loc='lower right', frameon=False, handlelength=1.0, fontsize=16)

                    plt.setp(ax1.get_xticklabels(), fontsize=20)
                    plt.setp(ax1.get_yticklabels(), fontsize=20)
                    ax1.set(xlim=(0, 1), ylim=(0.0, 1))
                    plt.tight_layout()

                    if not os.path.exists("Figures/PerturbationsAUC/" + dataset + "/" + videoProfile):
                        os.makedirs("Figures/PerturbationsAUC/" + dataset + "/" + videoProfile)
                    
                    if(cfg.split("/")[0] == "delay_15.delay_15" or cfg.split("/")[0] == "delay_50.delay_15" or cfg.split("/")[0] == "delay_100.delay_15"):
                        perturbation = "latency"

                    if(cfg.split("/")[0] == "delay_50-bw_1500.delay_15" or cfg.split("/")[0] == "delay_50-bw_750.delay_15" or cfg.split("/")[0] == "delay_50-bw_250.delay_15"):
                        perturbation = "bandwidth"
                    
                    if(cfg.split("/")[0] == "delay_50-loss_2.delay_15" or cfg.split("/")[0] == "delay_50-loss_5.delay_15" or cfg.split("/")[0] == "delay_50-loss_10.delay_15"):
                        perturbation = "loss"

                    if(cfg.split("/")[0] == "delay_50-jitter1.delay_15" or cfg.split("/")[0] == "delay_50-jitter4.delay_15" or cfg.split("/")[0] == "delay_50-jitter8.delay_15" or cfg.split("/")[0] == "delay_50-jitter16.delay_15" or cfg.split("/")[0] == "delay_50-jitter32.delay_15"):
                        perturbation = "jitter"
                    
                    if(cfg.split("/")[0] == "delay_50.delay_15_last" or cfg.split("/")[0] == "delay_50.delay_50_last" or cfg.split("/")[0] == "delay_50.delay_100_last"):
                        perturbation = "lastLegRTT"
                    
                    
                    plt.xlabel('False Positive Rate', fontsize=26)

                    fig.savefig("Figures/PerturbationsAUC/" + dataset + "/" + videoProfile + "/" + perturbation + "_" + feature_set + "_" + cfg.split("/")[0] + "_ROC_plot.pdf")   # save the figure to file
                    plt.close(fig)


def DifferentProfilesAUC():

    dataset = "ProtozoaCaps3March_Profiles"

    perturbation_configs = [
        [
        "delay_50" + "RegularTraffic-StegozoaTraffic/",
        ]
    ]

    feature_sets = [
        "PL_0_30_246",
        "Stats_0_30_246"
    ]

    for feature_set in feature_sets:

        for perturbation_config in perturbation_configs:
            

            fig = plt.figure()#figsize=[5.4, 5.4])
            ax1 = fig.add_subplot(111)

            for cfg in perturbation_config:
                    
                for n, videoProfile in enumerate(["Chat", "LiveCoding", "Gaming", "Sports"]):#enumerate(os.listdir("classificationData/" + dataset)):
                    if (".DS_Store" in videoProfile):
                        continue

                    sensitivity = np.load("classificationData/" + dataset + "/" + videoProfile + "/" + cfg + feature_set + "/ROC_10CV_XGBoost_Sensitivity.npy")
                    specificity = np.load("classificationData/" + dataset + "/" + videoProfile + "/" + cfg + feature_set + "/ROC_10CV_XGBoost_Specificity.npy")

                    auc = np.trapz(sensitivity, specificity)
                    print "stats AUC " + cfg + ": " + str(auc)

                    label_text = "AUC"
                    if(videoProfile == "Chat"):
    #PlotMPTComparisonStats
                        label_text = "Chat " + ' - AUC = %0.2f' % (auc)
                    elif(videoProfile == "LiveCoding"):
                        label_text = "LiveCoding " + ' - AUC = %0.2f' % (auc)
                    elif(videoProfile == "Gaming"):
                        label_text = "Gaming " + ' - AUC = %0.2f' % (auc)
                    elif(videoProfile == "Sports"):
                        label_text = "Sports " + ' - AUC = %0.2f' % (auc)
                

                    ax1.plot(specificity, sensitivity, lw=6, color=colors[n], label = label_text)

                ax1.plot([0, 1], [0, 1], 'k--', lw=2, color="0.0", label = 'Random Guess')
                ax1.yaxis.grid(color='grey', linestyle='dotted', lw=0.2)
                ax1.spines['right'].set_visible(False)
                ax1.spines['top'].set_visible(False)
                plt.xlabel('a) False Positive Rate', fontsize=26)
                plt.ylabel('True Positive Rate', fontsize=26)
                plt.legend(loc='lower right', frameon=False, handlelength=1.0, fontsize=14)

                plt.setp(ax1.get_xticklabels(), fontsize=20)
                plt.setp(ax1.get_yticklabels(), fontsize=20)
                ax1.set(xlim=(0, 1), ylim=(0.0, 1))
                plt.tight_layout()

                if not os.path.exists("Figures/ProfilesAUC/" + dataset):
                    os.makedirs("Figures/ProfilesAUC/" + dataset)
                

                fig.savefig("Figures/ProfilesAUC/" + dataset + "/" + cfg.split("/")[0] + "_" + feature_set + "_ROC_plot.pdf")   # save the figure to file
                plt.close(fig)




def DifferentAppsAUCSolo():

    datasets = [
        "ProtozoaCapsApprtc640x480April28",
        "ProtozoaCapsCoderpad640x480April29",
    ]

    perturbation_configs = [
        [
        "delay_50.delay_15/" + "RegularTraffic-StegozoaTraffic/",
        ]
    ]

    feature_sets = [
        "PL_0_30_250",
        "Stats_0_30_250"
    ]

    for dataset in datasets:
        for feature_set in feature_sets:
            for perturbation_config in perturbation_configs:

                fig = plt.figure()
                ax1 = fig.add_subplot(111)

                for n, cfg in enumerate(perturbation_config):
                        
                    sensitivity = np.load("classificationData/" + dataset + "/Chat/" + cfg + feature_set + "/ROC_10CV_XGBoost_Sensitivity.npy")
                    specificity = np.load("classificationData/" + dataset + "/Chat/" + cfg + feature_set + "/ROC_10CV_XGBoost_Specificity.npy")

                    auc = np.trapz(sensitivity, specificity)
                    print "stats AUC " + cfg + ": " + str(auc)

                    label_text = "AUC"
                    if("Coderpad" in dataset):
                        label_text = "coderpad.io " + ' - AUC = %0.2f' % (auc)
                    elif("Apprtc" in dataset):
                        label_text = "appr.tc " + ' - AUC = %0.2f' % (auc)
                

                    ax1.plot(specificity, sensitivity, lw=6, color=colors[n], label = label_text)

                    ax1.plot([0, 1], [0, 1], 'k--', lw=2, color="0.0", label = 'Random Guess')
                    ax1.yaxis.grid(color='grey', linestyle='dotted', lw=0.2)
                    ax1.spines['right'].set_visible(False)
                    ax1.spines['top'].set_visible(False)
                    plt.xlabel('False Positive Rate', fontsize=26)
                    plt.ylabel('True Positive Rate', fontsize=26)
                    plt.legend(loc='lower right', frameon=False, handlelength=1.0, fontsize=14)

                    plt.setp(ax1.get_xticklabels(), fontsize=20)
                    plt.setp(ax1.get_yticklabels(), fontsize=20)
                    ax1.set(xlim=(0, 1), ylim=(0.0, 1))
                    plt.tight_layout()

                    if not os.path.exists("Figures/AppsAUC/" + dataset):
                        os.makedirs("Figures/AppsAUC/" + dataset)
                    

                    fig.savefig("Figures/AppsAUC/" + dataset + "/" + cfg.split("/")[0] + "_" + feature_set + "_ROC_plot.pdf")   # save the figure to file
                    plt.close(fig)


def DifferentAppsAUCTogether():

    datasets = [
        "ProtozoaCapsApprtc640x480April28",
        "ProtozoaCapsCoderpad640x480April29",
    ]

    perturbation_configs = [
        [
        "delay_50.delay_15/" + "RegularTraffic-StegozoaTraffic/",
        ]
    ]

    feature_sets = [
        #"PL_0_30_250",
        "Stats_0_30_250"
    ]

    fig = plt.figure()
    ax1 = fig.add_subplot(111)

    for j, dataset in enumerate(datasets):
        for feature_set in feature_sets:
            for perturbation_config in perturbation_configs:

                

                for n, cfg in enumerate(perturbation_config):
                        
                    sensitivity = np.load("classificationData/" + dataset + "/Chat/" + cfg + feature_set + "/ROC_10CV_XGBoost_Sensitivity.npy")
                    specificity = np.load("classificationData/" + dataset + "/Chat/" + cfg + feature_set + "/ROC_10CV_XGBoost_Specificity.npy")

                    auc = np.trapz(sensitivity, specificity)
                    print "stats AUC " + cfg + ": " + str(auc)

                    label_text = "AUC"
                    if("Coderpad" in dataset):
                        label_text = "coderpad.io " + ' - AUC = %0.2f' % (auc)
                    elif("Apprtc" in dataset):
                        label_text = "appr.tc " + ' - AUC = %0.2f' % (auc)
                

                    ax1.plot(specificity, sensitivity, lw=6, color=colors[j], label = label_text)

                    
                    ax1.yaxis.grid(color='grey', linestyle='dotted', lw=0.2)
                    ax1.spines['right'].set_visible(False)
                    ax1.spines['top'].set_visible(False)
                    plt.xlabel('a) False Positive Rate', fontsize=26)
                    plt.ylabel('True Positive Rate', fontsize=26)
                    plt.legend(loc='lower right', frameon=False, handlelength=1.0, fontsize=14)

                    plt.setp(ax1.get_xticklabels(), fontsize=20)
                    plt.setp(ax1.get_yticklabels(), fontsize=20)
                    ax1.set(xlim=(0, 1), ylim=(0.0, 1))
                    plt.tight_layout()

                    if not os.path.exists("Figures/AppsAUC/" + dataset):
                        os.makedirs("Figures/AppsAUC/" + dataset)
                    
    ax1.plot([0, 1], [0, 1], 'k--', lw=2, color="0.0", label = 'Random Guess')
    fig.savefig("Figures/AppsAUC/" + dataset + "/" + "together.pdf")   # save the figure to file
    plt.close(fig)



def PLStatsComparison():

    datasets = [
        "ProtozoaCaps3March",
    ]

    perturbation_configs = [
        [
        "delay_50/" + "RegularTraffic-StegozoaTraffic/",
        ]
    ]

    feature_sets = [
        "PL_0_30_250",
        "Stats_0_30_250"
    ]

    fig = plt.figure()
    ax1 = fig.add_subplot(111)

    for dataset in datasets:
        for j, feature_set in enumerate(feature_sets):
            for perturbation_config in perturbation_configs:

                

                for n, cfg in enumerate(perturbation_config):
                        
                    sensitivity = np.load("classificationData/" + dataset + "/Chat/" + cfg + feature_set + "/ROC_10CV_XGBoost_Sensitivity.npy")
                    specificity = np.load("classificationData/" + dataset + "/Chat/" + cfg + feature_set + "/ROC_10CV_XGBoost_Specificity.npy")

                    auc = np.trapz(sensitivity, specificity)
                    print "stats AUC " + cfg + ": " + str(auc)

                    label_text = "AUC"
                    if("PL" in feature_set):
                        label_text = "Packet Length Dist." + ' - AUC = %0.2f' % (auc)
                    elif("Stats" in feature_set):
                        label_text = "Sum. Stats." + ' - AUC = %0.2f' % (auc)
                

                    ax1.plot(specificity, sensitivity, lw=6, color=colors[j], label = label_text)
                    ax1.text(0.05, 0.95, "RTT=50ms", fontsize=16, bbox=dict(facecolor='white'))
                    
                    ax1.yaxis.grid(color='grey', linestyle='dotted', lw=0.2)
                    ax1.spines['right'].set_visible(False)
                    ax1.spines['top'].set_visible(False)
                    plt.xlabel('a) False Positive Rate', fontsize=26)
                    plt.ylabel('True Positive Rate', fontsize=26)
                    plt.legend(loc='lower right', frameon=False, handlelength=1.0, fontsize=14)

                    plt.setp(ax1.get_xticklabels(), fontsize=20)
                    plt.setp(ax1.get_yticklabels(), fontsize=20)
                    ax1.set(xlim=(0, 1), ylim=(0.0, 1))
                    plt.tight_layout()

                    if not os.path.exists("Figures/AppsAUC/" + dataset):
                        os.makedirs("Figures/AppsAUC/" + dataset)
                    
    ax1.plot([0, 1], [0, 1], 'k--', lw=2, color="0.0", label = 'Random Guess')
    fig.savefig("Figures/baselineAUC_PLStats.pdf")   # save the figure to file
    plt.close(fig)




if __name__ == "__main__":
    PerturbationsAUCPerProfile()
    ##PerturbationsAUCAll()

    ##DifferentProfilesAUC()
    ##DifferentProfilesAUC_RegularVsRegular()

    #DifferentAppsAUCSolo()
    ##DifferentAppsAUCTogether()

    #PLStatsComparison()
