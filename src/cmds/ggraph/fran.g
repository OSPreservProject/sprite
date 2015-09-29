xla value
yla # of points with this value
typ 1
com  This doesn't seem implemented yet!  cro on
com  xst 4 0.1 0.0 310.0 this "works" -- below doesn't (enforces 0.0 xstart)
xst 8 0.1 -0.4 150.0
yst 8 10.0 0.0 50.0
xpr 0 1
tis 1
yts 1
yti off
com 256  1.5      0  Diagonal=0.0;   387=|E|; GiantC=  236 nodes,   385 edges.
com dBar= 3.02, GiantC= 3.26  Vecs 2 to  25.  2-byte reals. eigrs:  1252 secs.
com Vector    2; eigenvalue=  3.778970
das   2
-0.21000   0
-0.20000   1
-0.19000   0
-0.18000   1
-0.17000   1
-0.16000   1
-0.12000   0
-0.11000   1
-0.10000   1
-0.09000   3
-0.08000   1
-0.07000   4
-0.06000   5
-0.05000   5
-0.04000   6
-0.03000   6
-0.02000   8
-0.01000  15
 0.00000  37
 0.01000  36
 0.02000  26
 0.03000  21
 0.04000  10
 0.05000   6
 0.06000   8
 0.07000   8
 0.08000   4
 0.09000   5
 0.10000   1
 0.11000   3
 0.12000   2
 0.13000   1
 0.14000   1
 0.15000   0
 0.16000   1
 0.18000   0
 0.19000   1
 0.20000   1
 0.21000   1
 0.22000   1
 0.24000   0
 0.25000   1
 0.28000   0
 0.29000   1
 0.35000   0
 0.36000   1
dae
gti 256  1.5      0 0: maxCount= 37 
dra   ghout.0
sxt -0.4 0.4 8
syt 0 20 8
dra   ghout.1
sxt -0.4 0.4 8
syt 0 64 4
dra   ghout.2
