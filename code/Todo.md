

#############################################################################################################################################

With the new reuse cluster paper, we are hiding the server dataset from multiple query attack (mention this in the final ppt)
So, there should be 3 main things
1) protection against query attack
2) private recommendation
3) feedback improvement (maybe)
4) reducing dataset of cipher dataset

#############################################################################################################################################

No need to send distance in the final top k (keep it for now, for debugging purpose)

###############################################################################################################################################

- discuss why if the distance is 0 then the GC doesnt include it in the top-k ?
- if i set the plain modulus to be 2^N and not 2^N-1, why is every output 0 ?

...

2) Check what attacks can be done on the reduced, full encrypted, partially encrypted and fully unprotected dataset ?

3) Calculate privacy gain for each.

An important parameter for any leakage-abuse attack is its known-data rate; that is, the fraction of client data that must be known to the adversary.
Also depends if the known-data is high frequency or low frequency data, like common and uncommon names
https://eprint.iacr.org/2019/1175.pdf

6) Dataset reduction on full encrypted dataset (IDEAAAA !!, LATERRRR)


http://openproceedings.org/2018/conf/edbt/paper-298.pdf
Have a look into this, maybe it helps since they discuss some stuff to do comparisions
THIS CAN ALSO BE A NICE COMPARISION TO WHAT I HAVE DONE



--------------------------------------------------------------------------------------------------------------


2) Check what attacks can be done on the reduced, full encrypted, partially encrypted (only the speficif neighbours, reduced dataset) and fully unprotected dataset ?


3) Calculate privacy gain for each.
An important parameter for any leakage-abuse attack is its known-data rate; that is, the fraction of client data that must be known to the adversary.
https://eprint.iacr.org/2019/1175.pdf


1) investigate particular slow user id
based on the number of items rated size


matches with the reduced/original dataset   k % reduction         how much time it takes the HE + plus the one time time it takes to prepare the stash and groups
accuracy                                        privavy                 performace


make prper graph
