
## 1) Datasize for random modulo ##
uint32

## 2) Datasize for vector items ##
uint32

## 3) For-loops datasize ##
size_t 

## 4) POSITIVE INTEGERS IN THE DATASET ##
It has been assumed that the dataset will only contain positive integers, if the dataset contains negative integers then appropriate normalisation must be done

## 5) Quantity ##
To describe quantity both num (number) and size has been used, both are slightly different in this context
- num is mostly used to decsribe the number of objects, like the number of clusters, number of dimension, number of mangoes, trees etc
- size is mostly used to describe the size of the particilar object in question, like what is the size of the cluster? which means how many elemenst are there inside the cluster, or like what is the minimum group size, which means what is the minimum number of elemenst the group can have

## 6) Allgroupcluster(dataset/labels) LABELS Iteration ##
allgroupclusterlabels IS ALWAYS USED IN THE FOR-LOOP TO GET THE NUMBER OF CLUSTERS IN A GROUP INSTEAD OF allgroupclusterdataset, BECAUSE THE SECOND VECTOR IS ACTUALLY THE DIMENSION VECTOR AND NOT SOMETHING RELATED TO THE NUMBER OF CLUSTERS, THE THIRD VECTOR CAN BE USED BECAUSE IT REPRESENTS THE NUMBER OF CLUSTERS, BUT THIS WOULD LOOK UGLY

## 7) Labels ##
Labels are the value that is associated with a point, for example in the doramlabels and its corresponding doramdataset, it contains the labels for the datapoints which is
(a, b, c, label)

## 8) gcgroupclusteridshare and randomgroupclusteridshare ##
Technically we don't need to have the group id share, since we can assume that it is going and coming in the same sequence to and from the client, that is, we send the
group id 1,2,3,4.. and we recieve the group id 1,2,3,4.. along with its cluster id share. So just having the cluster id share should be enough.
BUT.... If we want to send the group id shuffled to the client (maybe in future), so it is even more complex to derieve some meaning for the client then having the
group id share would be nice and we don't have make any serious changes to the structures :)
(THE DESIGN IN THE GC CIRCUIT EXISTS (COMMENTED OUT), SO IF THE NEED ARRISES IT CAN BE USED, NOW IT IS SIMPLY ITERATING OVER THE GROUPS SEQUENTIALLY)

## 9) BIT REDUCTION ##
Even though in the paper the bit reduction part seems to be a part of GC but it can be done at the server and client side seperately and then the GC substraction is performed as above, I think it is the same, maybe even better

## 10) POLY_MODULUS_DEGREE CHECK ##
A poly modulus degree check has been put in each HE distance calculation. This check has been put to limit the vector size that will be converted to Plaintext polynomial. This prevents the vector to be greater than the maximum plaintext polynomial degree allowed, thus preventing any miscalculation.