# Multidimensional Matrix Market Formats

## MDX

To store a $n$-dimensional array:
* We can use a Matrix Market Exchange Format style to store and retrieve it. We can call it Multidimensional Matrix Market Exchange Format (with extension `.mdx`) defined as follows:

```
%%MatrixMarket matrix coordinate pattern general
% <comments>
<number N of dimensions> <max coord val dim1> <max coord val dim2> <max coord val dim3> ... <max coord val dimN> <num non-zero entries>
<coord dim1> <coord dim2> <coord dim3> ... <coord dimN>
...
```

Example: Where coordinates are encoded as `(z,y,x)` or `(d,r,c)`, *i.e.* depth, row, column.

```
%%MatrixMarket matrix coordinate pattern general
% <comments>
3 3 3 3 23
0 0 0
0 0 2
0 0 3
0 1 1
0 3 1
1 0 0
1 0 2
1 0 3
1 1 1
1 2 0
1 2 1
1 3 0
1 3 1
2 0 2
2 0 3
2 2 1
2 3 0
2 3 1
3 0 2
3 0 3
3 1 2
3 1 3
3 3 1
```

## MXS

Let $I$ be a vector of positive integers as frequencies counting different values per entry in each dimension.

Let $P$ be a vector containing the payload. 

- HEADER: $s$ $s^n$ $d$ $d^{\prime}$ $\text{dist}$ $c$ $c_{derr}$ $n$ $max_1$ $max_2$ $max_3$ $...$ $max_n$ $e$
- PAYLOAD: $v_{n}$ $v_{n-1}$ $v_{n-3}$ $...$ $v_{2}$ $v^1_{1}$ $v^2_{1}$ $v^3_{1}$ $...$ $v^m_{1}$ $...$ 
- TAIL (Index): $i_{n}$ $i_{n-1}$ $i_{n-3}$ $...$ $i_{2}$ $i_{1}$ $...$ $i^m_{1}$ $...$
- METADATA: $|I|$ $\max{(P)}$ $\max{(I)}$

...with $v_k \in P$, and $i_k \in I$.

### Encoding

```
funct encode( E, n ) {
    Let P be a vector of positive integers.
    Let I be a vector of positive integers.
    Let Pi be an array of n cells to store pointers to P, initially as Pi = [0,1,2,...,n-1].
    Let Ii be an array of n cells to store pointers to I, initially as Ii = [0,1,2,...,n-1].

    init = true
    for e in E do {
        if init then {
            for i = 0 to n-1 do {
                P.add(e[i])
                I.add(1)
            }
            init = false
        } else {
            j = 0
            while j < n do {
                if e[ j ] != P[ Pi[ j ] ] then {
                    break
                }
                j++
            }
            while j < n do {
                P.add( e[ j ] )
                Pi[ j ] = |P| - 1
                I[ Ii[ j ] ]++
                j++
                if j < n then {
                    I.add( 0 )
                    Ii[ j ] = |I| - 1
                } 
            }
        }
    }
}
```

- As an example, let us suppose to encode the following sequence of entries:

```
1 1 1
1 1 2
1 3 1
2 4 1
2 5 1
2 5 2
2 6 0
```
```
entry    = 1 1 1 | 1 1 2 | 1 3 1
is_open  = true
n        = 3
init     = false

P (slzr) = 1 1 1 2 3 1
I        = 1 2 2 1
Pi       = 1 3 1
Ii       = 0 1 3

j        = 0 1 2 3 0 1 2
```


### Decoding

```
func decode( P, I, n ) {
    Let C be a vector of coordinates.
    Let Pi be an array of n cells to store pointers to P, initially as Pi = [0,1,2,...,n-1].
    Let Ii be an array of n cells to store pointers to I, initially as Ii = [0,1,2,...,n-1].

    p' = n // Index for P
    i' = n // Index for I
    j = n-1 // Index for both Pi and Ii

    while true {
        C.add( gen_coord( P, Pi ) )
        I[ Ii[ j ] ]--

        if I[ Ii[ j ] ] > 0 then {
            Pi[ j ] = p'
            p'++
        } else {
            // Checking backward:
            while I[ Ii[ j ] ] == 0 do {
                j--
                I[ Ii[ j ] ]--
                if j == 0 and I[ Ii[ j ] ] == 0 then {
                    return // Decoding completed!
                }
            }
            
            // Checking forward:
            while j < n then {
                Pi[ j ] = p'
                p'++
                j++
                if j < n then {
                    Ii[ j ] = i'
                    i'++
                }
            }
            j--
        }
    }
}

func gen_coord( P, Pi ) {
    Let c be an array of |Pi| cells.
    for i = 0 to |Pi| - 1 do {
        c[ i ] = P[ Pi[ i ] ]
    }
    return c
}
```