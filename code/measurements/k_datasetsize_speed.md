# IMPROVED SPEED WITH BFV PLAINTEXT ENCODING #

K = 3
The process was taking as follows for 20000 items

Without Batch Encoding
Stash Process   Group Process   ORAM Read   ORAM Top-K Cluster Points   Stash + ORAM Top-K GC   Over All time
HE - 108s       HE+GC - 53s     10s         HE - 43s                    <1s                      230s
GC - 2s                                     GC - 4s

Memory Consumption - 9.1 GB approx

With Plaintext Encoding
Stash Process   Group Process   ORAM Read   ORAM Top-K Cluster Points   Stash + ORAM Top-K GC   Over All time
HE - 1s        HE+GC - 2s      5s          HE - <1s                    <1s                      18s
GC - 5s                                     GC - 5s

Memory consumption - 6.17 GB approx     (   32.2% less memory consumption )
Speed              -                    (   Takes 7.8% of earlier time, almost 13 times faster )


K = 1
With Plaintext Encoding (9500)
Stash Process   Group Process   ORAM Read   ORAM Top-K Cluster Points   Stash + ORAM Top-K GC   Over All time
HE - <1s        HE+GC - 1s      1s          HE - 1s                    <1s                      12s
GC - 5s                                     GC - 3s


K = 4
With Plaintext Encoding (24300)
Stash Process   Group Process   ORAM Read   ORAM Top-K Cluster Points   Stash + ORAM Top-K GC   Over All time
HE - <1s        HE+GC - 3s      7s          HE - <1s                    <1s                      26s
GC - 7s                                     GC - 6s


K = 5
With Plaintext Encoding (27800)
Stash Process   Group Process   ORAM Read   ORAM Top-K Cluster Points   Stash + ORAM Top-K GC   Over All time
HE - <1s        HE+GC - 3s      8s          HE - 1s                    1s                      27s
GC - 8s                                     GC - 5s


K = 7
With Plaintext Encoding (33000)
Stash Process   Group Process   ORAM Read   ORAM Top-K Cluster Points   Stash + ORAM Top-K GC   Over All time
HE - <1s        HE+GC - 3s      11s          HE - <1s                    <1s                      30s
GC - 8s                                     GC - 7s

Memory consumption - 10.85 GB approx