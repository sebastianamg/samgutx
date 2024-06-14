# Multidimensional Matrix Market Formats

## MDX
<!-- TODO -->

## MXS

- HEADER: $s$ $s^n$ $d$ $d^{\prime}$ $\text{dist}$ $c$ $c_{derr}$ $n$ $max_1$ $max_2$ $max_3$ $...$ $max_n$ $e$
- PAYLOAD: $l_{n}$ $v_{n}$ $l_{n-1}$ $v_{n-1}$ $l_{n-2}$ $v_{n-3}$ $l_{n-4}$ $...$ $v_{2}$ $l_{1}$ $v^1_{1}$ $v^2_{1}$ $v^3_{1}$ $...$ $v^m_{1}$ $...$

Let $I$ be a vector of positive integers as frequencies counting different values per entry in each dimension.

Let $P$ be a vector containing the payload. 

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