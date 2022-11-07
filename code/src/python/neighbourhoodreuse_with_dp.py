from glob import glob
import numpy as np
from numpy import dtype, genfromtxt, index_exp
import math
import sys
import multiprocessing as mp
from threading import Thread
import time

# # 100k 1700
neighbourcount = 943
itemcount = 1682

# # 100k 9000
# neighbourcount = 610
# itemcount = 9742

# # 1m 4000
# neighbourcount = 6040
# itemcount = 3900

# # 10m 10000
# neighbourcount = 71567
# itemcount = 10681

# # 20m 27000
# neighbourcount = 138493
# itemcount = 27278

# # 100m 17770
#neighbourcount = 480189
#itemcount = 17770


# We use Gain approach !!

# # [ [item1, rating], [item2, rating], ...]
targetuseritemlist = []
def maketargetuseritemlist(targetuserid, dataset):
    global targetuseritemlist
    for d in dataset:
        if d[0] == targetuserid:
            targetuseritemlist.append([d[1], d[2]])


# [neighbour1, neighbour2, ...]
neighbourlist = []
# contains the list of all the unique neighbours that exists in the dataset
def makeNeighbourList(targetuserid):
    global neighbourlist, neighbourcount
    for i in range(1, targetuserid):
        neighbourlist.append(i)
    for i in range(targetuserid+1, neighbourcount+1):
        neighbourlist.append(i)

# [ [ neighbourid1, rating1, rating2, ... ], ... ]
neighboursitemratinglist = []
# initilaizing the list
def initializeneighboursitemratinglist():
    global neighbourcount, neighboursitemratinglist
    for i in range(1, neighbourcount+1):
            neighboursitemratinglist.append([i])

# Here we make arrays of the neighbours and their corresponding items they have rated WHICH ALSO THE TARGET USER HAS RATED
def makeTargetUserItemsListForNeighbours(targetuserid, dataset):
    global targetuseritemlist, neighbourlist, neighboursitemratinglist

    # going thorugh targetuseritemlist first, because the items should be in the same order as this list for function calculateCosineSimilarityAndGainReusability()
    for tuil in targetuseritemlist:
        for d in dataset:  
            if abs(int(d[1]) - int(tuil[0])) < 10:
            #if d[1] == tuil[0]:
                if d[0] != targetuserid:
                    neighboursitemratinglist[int(d[0]-1)].append(tuil[1])



neighboursitemratinglistzeroclean = []
# Here we remove all the neighbours who have a zero vector, that is, those who have not rated any item that the target user has rated
def removeNeighboursZeroVectors():
    global neighboursitemratinglist, neighboursitemratinglistzeroclean
    
    for i in neighboursitemratinglist:
        if len(i) > 1:
            neighboursitemratinglistzeroclean.append(i)


# [ [neighbourid, cos-similarity], ... ]
cosinesimilarity = []
# [ [neighbourid, resuabilityscore], ... ]
gainresuability = []
# Performs the cosine similarity between the common items the target user and the neighbours have rated
# Difference between similarity and gain reusabilty is that
# ... similarity measures how many points the target user and a candidate user have RATED SIMILARY (similarity of the 2 vectors), whereas
# ... gain reusability defines how many points the target user and a candidate user have RATED IN COMMON
def calculateCosineSimilarityAndGainReusability():
    global neighboursitemratinglistzeroclean, targetuseritemlist, cosinesimilarity, gainresuability
    # iterating through each neighbour
    for neighbour in neighboursitemratinglistzeroclean:
        AiBi = 0
        Aisq = 0
        Bisq = 0
        # iterate through eachitem of neighbour
        for idx_, rating in enumerate(neighbour):
            if idx_ == 0:
                continue
            if idx_ > len(targetuseritemlist):
                break
            # getting the ratings
            Ai = rating
            Bi = targetuseritemlist[idx_ - 1][1]
            AiBi += Ai*Bi
            Aisq += Ai**2
            Bisq += Bi**2

        # getting the cosine similaroty for a neighbour and the target user
        similarity = AiBi/(math.sqrt(Aisq)*math.sqrt(Bisq))
        cosinesimilarity.append([neighbour[0], similarity])

        # calculating gain reusability, -1 because of removing the target user present in the list
        resuability = (len(neighbour)-1)/len(targetuseritemlist)
        gainresuability.append([neighbour[0], resuability])


# [ [neighbourid, resuabilityscore], ... ]
gainresuabilitysorted = []
# [ [neighbourid, cos-similarity], ... ]
cosinesimilaritysorted = [] 
# sorting the lists smaller to greater on the basis of their index=1
# it is accesnding BECAUSE then the item with best cosine simi and gain reuse will have the heighest index, which will lead to heighest rank, 
# leading to heighest score in the score function
def setNeighbourRank():
    global cosinesimilarity, gainresuability, gainresuabilitysorted, cosinesimilaritysorted
    gainresuabilitysorted = sorted(gainresuability, key=lambda x: x[1])
    cosinesimilaritysorted = sorted(cosinesimilarity, key=lambda x: x[1])


# SET ACCORDING to the sorted cosinesimilaritysorted
# [ [neighbourid, score], ... ]
neighbourscores = []
# calculates the scores of each user on the basis of their ranks (HEIGHER IS BETTER) for the cosine similarity and gain reusability
def calculateScore(lmbda):
    global gainresuabilitysorted, cosinesimilaritysorted, neighbourscores
    # iterate through all the accesnding ranking cosine similarity list
    for rankcossimi, neighbourcossimi in enumerate(cosinesimilaritysorted):
        # iterate and find the corresponding neighbour and its rank in the accesnding ranking gain reuse list
        for rankgainreuse, neighbourgainreuse in enumerate(gainresuabilitysorted):
            # if candidate neighbours match, calculate score and append to the score list
            if neighbourgainreuse[0] == neighbourcossimi[0]:
                score = lmbda*(rankcossimi+1) + (1-lmbda)*(rankgainreuse+1)
                neighbourscores.append([neighbourcossimi[0], score])
                break


# [ [neighbourid, item, rating], .... ]
notratedbytargetuserneighbouritemlist = []
# make a list of all the items rated by all the neighbours that the target user has not rated
def makeNotTargetUserItemsListForNeighbours(dataset):
    global targetuseritemlist, notratedbytargetuserneighbouritemlist

    for d in dataset:
        targetuseralredyrated = False
        for item in targetuseritemlist:
            if d[1] == item[0]:
                targetuseralredyrated = True
                break
        if targetuseralredyrated == False:
            notratedbytargetuserneighbouritemlist.append([d[0], d[1], d[2]])


# list of items that have not been rated by the target user but only by the scored neighbours
# [ [item1, [ [neighbourid1, rating, score], [neighbourid2, rating, score], ...]], ... ]
itemsratedonlybythescoredneighbours = []
# choose candidate neighbours who have rated items not rated by the target user
def selectScoredCandidateNeighboursForEachItem():
    global notratedbytargetuserneighbouritemlist, neighbourscores, itemsratedonlybythescoredneighbours

    # all candidate with thier rated items
    for neighbouritem in notratedbytargetuserneighbouritemlist:
        for nscore in neighbourscores:
            if nscore[0] == neighbouritem[0]:
                itemfound = False
                for idx, i in enumerate(itemsratedonlybythescoredneighbours):
                    if neighbouritem[1] == i[0]:
                        itemsratedonlybythescoredneighbours[idx][1].append([neighbouritem[0], neighbouritem[2], nscore[1]])
                        itemfound = True
                        break
                if itemfound == False:
                    itemsratedonlybythescoredneighbours.append([neighbouritem[1], [[neighbouritem[0], neighbouritem[2], nscore[1]]]])
                break


# [ [item1, [ [neighbourid1, rating, score], [neighbourid2, rating, score], ...] ], ... ]
topkitemsratedonlybythescoredneighbours = []
# sorts the item-neighbour set according to the neighbour scores for the top k selection
def selectTopKScoredCandidateNeighboursForEachItem(k):
    global itemsratedonlybythescoredneighbours, topkitemsratedonlybythescoredneighbours
    for itemneighbourset in itemsratedonlybythescoredneighbours:
        sortedlist = sorted(itemneighbourset[1], key=lambda x: x[2], reverse=True)
        topkitemsratedonlybythescoredneighbours.append([itemneighbourset[0], sortedlist[:k]])


# [ [neighbourid, itemid, rating] ]
reduceddataset = []
# makes a new reduced dataset from the reduced neighbour algorithm
def saveReducedDataset():
    global topkitemsratedonlybythescoredneighbours, reduceddataset
    for itemneighbourset in topkitemsratedonlybythescoredneighbours:
        for neighbourset in itemneighbourset[1]:
            reduceddataset.append([neighbourset[0], itemneighbourset[0], neighbourset[1]])
            
    with open('../dataset/ratings_reduced.csv', "wb") as f:
        np.savetxt(f, reduceddataset, delimiter=",", fmt="%10.2f")


def neighbourhoodreuse(datasetpath = None, coloumnnumber = None, targetuserid_ = None, lmbda_ = None, k_ = None):
    global targetuseritemlist, neighbourlist, neighboursitemlist, cosinesimilarity, gainresuability, neighbourscores
    global itemsratedonlybythescoredneighbours, notratedbytargetuserneighbouritemlist, reduceddataset

    dataset = genfromtxt(sys.argv[1], delimiter=',')
    # taking only the first n coloumns
    n_coloumn = int(sys.argv[2])
    dataset = dataset[:,:n_coloumn]
    targetuserid = int(sys.argv[3])
    lmbda = float(sys.argv[4])
    k = int(sys.argv[5])

    start_time = time.perf_counter()

    maketargetuseritemlist(targetuserid, dataset)

    print(time.perf_counter() - start_time, "seconds")

    makeNeighbourList(targetuserid)

    print(time.perf_counter() - start_time, "seconds")

    initializeneighboursitemratinglist()

    print(time.perf_counter() - start_time, "seconds")

    makeTargetUserItemsListForNeighbours(targetuserid, dataset)

    print(time.perf_counter() - start_time, "seconds")

    removeNeighboursZeroVectors()

    print(time.perf_counter() - start_time, "seconds")

    calculateCosineSimilarityAndGainReusability()

    print(time.perf_counter() - start_time, "seconds")

    setNeighbourRank()

    print(time.perf_counter() - start_time, "seconds")

    calculateScore(lmbda)

    print(time.perf_counter() - start_time, "seconds")

    makeNotTargetUserItemsListForNeighbours(dataset)

    print(time.perf_counter() - start_time, "seconds")

    selectScoredCandidateNeighboursForEachItem()
    
    print(time.perf_counter() - start_time, "seconds")

    selectTopKScoredCandidateNeighboursForEachItem(k)

    print(time.perf_counter() - start_time, "seconds")

    saveReducedDataset()

    print(time.perf_counter() - start_time, "seconds")


neighbourhoodreuse()
