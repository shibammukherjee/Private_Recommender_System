# 0) GROUP SIZE AND STASH SIZE OBSERVATION #
Surprisingly if the group size is smaller and the stash size is bigger, it goes much faster in comparision to what we had though before that maybe it is the other way around


# 1) Some assumptions made by me #
In the paper in figure 7, it is mentioned in point 6 that client and server input their secret shares of all indices (group_index, cluster_index)

Now what I assume from this statement is both the group indices containing the clusters and the individual cluster indices have to be secret shared

But my idea was that maybe it is not required to secret share the group index because anyway everytime, all the groups will send their Top-K cluster ids,
these cluster ids are anyway secret shared and is thus different each time. Till this point from what I understand is that the user cant gain any information or
determine any relation between the cluster index and group index. After this point when the DORAM is being performed, the server also doesn't gain or relate any
information between the cluster and group because the combining of the secret shared index is done over the GC and the data is retrieved from each group with DORAMread.
In the end when the client gets the final result label of the data point, it still can't relate anything to the group index.

This should belong to the glovalvarclient.cpp above the gcgroupdistanceshare 
vector<uint32_t> groupindexshare;


# 2) GC for PRF Kreyvium #
For the DB masking we need to perform the kreyvium + xor in GC,
    We have 2 ways of doing this ---
        First performing the kreyvium + xor in GC where we need to send just the secret key to the server for GC (more code but faster)
        Second is, we perform the kreyvium on the client side seperately for each index and then send the entire thing where we just perform xor in GC (less code but slower)
I choose the first for the sake of speed :D


# 3) DIFFERENCE FROM PAPER (MAYBE) #
1) Figure 7, point 8, I am directly sending the top-k ID instaed of sending a share and then again send sending the server share to the client to get the actual value

2) For DORAM_read I am doing the Figure 5 with a help of an index dataset which contains the indices of the group and the cluster and helps to retrive the cluster datapoiunts in the GC, refer to doramreadclusterstopk.cpp

3) Figure 7, point 3, to put the dummy points in the doram dataset, I filling the doram dataset with the actual dataset like
doram cluster size -> 4 (0,0,0,0,0)
some actual cluster size -> 2 (5,3,1)
doram cluster will be (5,3,1,5,3) because in the topk GC we are only consider less than and not less than eq thus avoiding any repetation in the top-k array


# 4) DATASET NORMALIZATION OPTIMISATION #
The dataset is normalized coloumn by coloumn instead of all, this give us more space before hitting the uint32_max size for the bfv when we are performing the sum of all the squared differences. Also try to normalize by a smaller number to make it remove the decimals, like anything that is 2.0,2.5,3.0,3.5 can be just multipled by 2 to make it an integer instaed of multiplying by 10 


# 5) TEST RESULTS WITH/WITHOUT NEIGHBOURHOOD REDUCTION #

## 20000 ##
WITH REDUCTION
Distance 13793.0 ID 131714.0    (same)
Distance 15770.0 ID 131749.0    (same)
Distance 17982.0 ID 131826.0

WITHOUT REDUCTION
Distance 0.0 ID 131724.0
Distance 13793.0 ID 131714.0    (same)
Distance 15770.0 ID 131749.0    (same)

## 100,000 ##
WITH REDUCTION
Distance 13793.0 ID 131714.0    (same)
Distance 15770.0 ID 131749.0    (same)
Distance 17982.0 ID 131826.0


# 6) WHY USE SO LARGE PLAIN MODULOS IN HE #

As per SEAL exmaple 2_encoders.cpp line:21
    "....one may ask why not just increase the plain_modulus parameter until no
    overflow occurs, and the computations behave as in integer arithmetic. The problem
    is that increasing plain_modulus increases noise budget consumption, and decreases
    the initial noise budget too."

But here in our situation since we are not multiplying anything, the noise budget doesn't go down by that much, so no problem i guess

## BUT MAYBE BATCHING HELPS TO SPEED THINGS UP, SO WHY NOT TRY IT ? ##


# 7) DATA OVERFLOW DETECTION IN HE ? #
As per SEAL example 2_encoders.cpp line:190
It is not possible to detect overflow in encrypted HE in BFV which has been used in this project.
CKKS does that but only gives approximate results.


# 8) FORMULIZE PRIVACY MESUREMENT #

?? WHAT DID THEY MEAN ??


# 9) LOW BIN NUMBER PROBLEM #

Low bin number really messes up the accuracy of the stash


# 10) ADDED + 1 TO THE DISTANCE IN THE STASH AND GROUP GC #

A + 1 has been added to the calculation in the client ROLE in the shareddistancetopk.cpp GC, so that we get the an inaccuracy of +1, because
for some unknown reason in GC if the combined share distance is 0 it is excluded from the GC top - k list,, strange


# 11) DISTANCE CALCULATION IN STASH AND GROUP #

In stash and group we are calculating the distance with an efficeint vectorized method of plaintext encoding. Where we put the values that we have in the coeffecients
of the plaintext and eventually ciphertext as well.

So what we can now do is to break the disnatce formula (a-b)^2 into a^2(cleint knows it) + b^2(server knows it) - 2.a.b(we calculate this part only in the HE)
The rest of the parts can be easily added in the client share and the server share on their own side in plane calculation.

In the GC we just random the random value and thus get the distance and eventually calculate the top-k


# 12) DISTANCE CALCULATION IN DORAMREADCLUSTERTOPK #

The major problem that we have in for distance calculation and fidning top-k in doram read cluster points is, that in the server doesn't know the query point
and also it doesn't know the dataset point.
Because of this reason we only have a and b in HE and the server doesn't have the b in plain uint. This problem lets to the server not able to perform the 
minus b^2 on its side in plain which is much more ffecient.

Because of this reason the server has to perform the - b^2 either in the HE or GC.

Now the problem with HE is, when we square a number the plaintext vector increases and thus it is not suitable for plaintext encoding.

So now we are just left with the GC where we can do the - b^2,, so in HE we can perform 2.a.b + random, then the client can do a^2 - (2.a.b - random)
and in GC they can do a^2 - (2.a.b - random) + random + b^2.

But it turned out to be more computationally expensive, thus it is better if we actually do a (a - b) + random in the HE, we don't add the dimensions and
then + random but instead we + random with each of the dimensions seperately.
In GC we - random from each dimension, square them, and add them to get the distance and eventually calculate the top k

Operation Summary
1) First method
HE
minus random
c = a.b
d = 2.c
D = d1 + d2 + ....
Dr = D + random
Plain
A = a1^2 + a2^2 + ...
S = A - D - random
GC
B = b1^2 + b2^2 + ...
distance = A - D - random + random + B

2) (BETTER IN MY OPINION)
HE
Here we don't do any +/- of random
c1 = (a1 - b1), c2 = (a1 - b1), ...
Plain
NOTHING
GC
c1R = c1 + random - random, c2R = c2 + random - random, ...
c1sq = c1^2, c2sq = c2^2, ...
distance = c1sq + c2sq + ....


# 13) Plaintext encoding polynomial 0 value problem #
Here we have a strange problem in plaintext encoding subtraction or any even operation, if the result is 0, the plynomial gets reduced, quite dangerous for bugs.


# 14) TYPES OF ATTACKS THAT CAN BE DONE ON DIFFERENT DATASETS #

# Fully Unprotected dataset #

    Since this is quite trivial, I couldn't find any list.

1) Manupulate dataset by attacker if has server access
2) Reading dataset
3) Manupulating recommendations
4) Guessing client query based on the recommendation results
5) If the client is attacker, then can query enough number of time to reveal the dataset even if we don't have server access.


# Reduced dataset #

1) SHILLING ATTACK
    On reduced datasets, attacks like "shilling attacks" are quite common. In a shilling attack, the attacker inject a few unscrupulous “shilling profiles” into the database of ratings for altering the system's recommendation, due to which some inappropriate items are recommended by the system (1). 
    
    There have been some work on shilling attack detection, In this paper, a shilling behaviour detection structure based on abnormal group user findings and rating time series analysis is proposed. A method for detecting suspicious ratings based on suspicious time windows and target item analysis is proposed (2).

    The existing attack methods usually generate malicious profiles by rating the item selected randomly. However, as these rating patterns are different from the real users, who have their own preferences on items, these attack methods can be easily detected by shilling attack detection, which significantly reduces the attack ability. This study proposes a shilling attack which generates malicious samples with strong attack ability and similarity to real users. To imitate the rating behavior of genuine users, our attack model considers both rated item correlation and item popularity when choosing items to rate. The profiles generated by our attack model is expected to be more similar to real user profiles, which increases the disguise ability. (3)

    This paper proposes an innovative robust method, based on matrix factorization, to neutralize the shilling attacks. (4)

    Another approach focuses on techniques that remove or reduce the influence of power users and determine their impact on RS accuracy and robustness using established metrics. We introduce a new metric used to assess the trade-off between accuracy and robustness when our mitigation approaches are applied. And our results show that, for user-based systems, reducing power user influence is more effective than removing power users from the dataset. (5)

    This paper addresses the privacy issue in CF by proposing a Private Neighbor Collaborative Filtering (PriCF) algorithm, which is constructed on the basis of the notion of differential privacy. PriCF contains an essential privacy operation, Private Neighbor Selection, in which the Laplace noise is added to hide the identity of neighbors and the ratings of each neighbor. (6)

    What if we put fake reviews in the dataset, such that when the dataset is reduced, the fake dataset in positioned in such a way that if there is a
    client query and the server sends recommendations, and if an attacker can see these recommendation then they can also easily guess the approximate point which
    the client queries because the recommendations were based on the fake dataset set by the attacker. (MY)


(1) https://ieeexplore.ieee.org/document/7824865/authors#authors
    https://www.semanticscholar.org/paper/Shilling-attacks-against-recommender-systems%3A-a-Gunes-Kaleli/0c696eb04e6a9075d88e41c508faaee25065e733 (BETTER)

(2) https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0196533
(3) https://link.springer.com/article/10.1007/s13042-018-0861-2
(4) https://ieeexplore.ieee.org/abstract/document/8668763
(5) https://www.aaai.org/ocs/index.php/FLAIRS/FLAIRS15/paper/viewPaper/10451
(6) https://link.springer.com/article/10.1007/s13278-014-0196-2


2) POISIONING ATTACK
    Deep learning in a collaborative setting is emerging as a corner-stone of many upcoming applications, wherein untrusted users collaborate to generate more accurate models. From the security perspective, this opens collaborative deep learning to poisoning attacks, wherein adversarial users deliberately alter their inputs to mis-train the model. These attacks are known for machine learning systems in general, but their impact on new deep learning systems is not well-established. The accuracy of a model trained using Auror drops by only 3% even when 30% of all the users are adversarial. (1)


(1) https://dl.acm.org/doi/abs/10.1145/2991079.2991125


# Fully encrypted dataset #

1) SEARCH AND ACCESS PATTERN ATTACK
    We show that search pattern leakage can severely undermine current SSE defenses. We propose an attack that leverages both access and search pattern leakage, as well as some background and query distribution information, to recover the keywords of the queries performed by the client. (1)

    In order to efficiently defend against leakage-abuse attacks on SE-based systems, we propose SEAL, a family of new SE schemes with adjustable leakage. In SEAL, the amount of privacy loss is expressed in leaked bits of search or access pattern and can be defined at setup. As our experiments show, when protecting only a few bits of leakage (e.g., three to four bits of access pattern), enough for existing and even new more aggressive attacks to fail. (2)

    Our attacks exploit a generic k-NN query leakage profile: the attacker observes the identifiers of matched records. (3)

    Then we empirically investigate the security of searchable encryption by providing query recovery and plaintext recovery attacks that exploit these leakage profiles. (4)

    Underlying our new attacks is a framework in which we cast the adversary's challenge as a non-crossing bipartite matching problem. This allows easy tailoring of attacks to a specific scheme's leakage profile. In a case study of customer records, we show attacks that recover 99% of first names, 97% of last names, and 90% of birthdates held in a database, despite all values being encrypted with the OPE scheme most widely used in practice. (5)

    The bulk of our work applies to a generic setting, where the adversary's view is limited to the set of records matched by each query (known as access pattern leakage). We also consider a more specific setting where rank information is also leaked, which is inherent inherent to multiple recent encryption schemes supporting range queries. (6)


(1) https://www.usenix.org/conference/usenixsecurity21/presentation/oya
(2) https://www.usenix.org/conference/usenixsecurity20/presentation/demertzis
(3) https://ieeexplore.ieee.org/abstract/document/8835292
(4) https://dl.acm.org/doi/abs/10.1145/2810103.2813700
(5) https://ieeexplore.ieee.org/abstract/document/7958603
(6) https://ieeexplore.ieee.org/document/8418610



2) MIXED ATTACKS

    In this paper, we study the concrete security provided by such systems. We present a series of attacks that recover the plaintext from DTE- and OPE-encrypted database columns using only the encrypted column and publicly-available auxiliary information. We consider well-known attacks, including frequency analysis and sorting, as well as new attacks based on combinatorial optimization. (1)

    We then design four new leakage-abuse attacks that rely on much weaker assumptions. Three of these attacks are volumetric in the sense thatthey only exploit leakage related to document sizes. In particular, this means that they work not only on SSE/STE-based ESAs but also against ORAM-based solutions. We also introduce two volumetric injection attack which use adversarial file additions to recover queries even from ORAM-based solutions. As far as we know, these are the first attacks of their kind. (2)

    This work shows that more plaintext information can be extracted from ORE ciphertexts than was previously thought. We identify two issues: First, we show that when multiple columns of correlated data are encrypted with ORE, attacks can use the encrypted columns together to reveal more information than prior attacks could extract from the columns individually. (3)

    we present new attacks for recovering the content of individual user queries, assuming no leakage from the system except the number of results and avoiding the limiting assumptions above. Unlike prior attacks, our attacks require only a single query to be issued by the user for recovering the keyword. (4)

     we introduce new leakage-abuse attacks that recover plaintexts from OPE/ORE-encrypted databases. Underlying our new attacks is a framework in which we cast the adversary’s challenge as a non-crossing bipartite matching problem. This allows easy tailoring of attacks to a specific scheme’s leakage profile. (5)

(1) https://dl.acm.org/doi/abs/10.1145/2810103.2813651
(2) https://eprint.iacr.org/2019/1175.pdf
(3) https://dl.acm.org/doi/abs/10.1145/2976749.2978379
(4) https://arxiv.org/abs/2008.06627
(5) https://eprint.iacr.org/2016/895.pdf



# Partially Encrypted #
referred to the partailly encrypted dataset to a scenario, were only the reused neighbors are encrypted.

Does that make sense in our scenario ?

-- First we assume that the reused dataset is encrypted by the server, because if it is encrypted by the client then it is not possible 
to compute the private top-k,
UNLESS the dataset is stored as a HE ciphertext form because only then the server can perform computations in the encrypted manner. BUT this only works
if we are not performing any groupping or clustering but instead a straight forward calculation with each individual point. Performing the groupping or clustering should be difficult if not impossible in a client encrypted reduced dataset. Also to make the client encrypted reduced dataset firstly the original dataset has to be unencrypted to perform the calculations which any reveal the dataset to the server anyway. Also the client calculating the dataset is not an option since that reveals all data to the client (or attacker). Thus having a client encrypted dataset makes no sense.

So let's say the server encrypts the dataset and stores it. The server has to decrypts the whole dataset everytime it wants to perform a calculation with the private recommender system.

PROS - 
1) Nothing is stored in plain, so if the server is directly attacked, its safe

CONS -
1) It needs to decrypt the entire reduced dataset if the server wants to calculate the top-k private recommendation (maybe try to get some speed results)
2) Still doesn't protect against collaborative K-NN attacks since in the end the results are shown in plain to the client (or attacker) :(

So having an encrypted partial dataset makes no sense in our scenerio. Same with a fully encrypted dataset. For now it seems the only thing increasing the privacy of the users is the dataset reduction.


And does it give a performance boost compared to fully encrypted ?

Definetly its much faster by the factor of dataset reduced in comparison to the full dataset (maybe try to get some speed results)

does it leak some additional information ?

Firstly it stills is prone to the K-NN attack. 



# 15) DATASET REDUCTION SPEED #
The dataset reduction speed depends on the number of items an user has rated, 2 takes 8 minutes, 3 takes almost 11 minues (a little bigger) whereas 6 which is much bigger than 2 and 3 takes more than an hour.


# 16) VOLUME DATA LEAK #
I think we are safe from any volume analysis since the size of each oram stays same