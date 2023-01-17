# SPDX-FileCopyrightText: Copyright (C) 2023 Florian Thake, <contact |at| tea-age.solutions>.
# SPDX-License-Identifier: MIT

import time

def fib( n ):
    if n == 1 or n == 0:
        return n
    else:
        return ( fib(n-1) + fib(n-2) )

def main():
   x = 25
   start = time.time()
   res = fib( x )
   end = time.time()
   print( 'Fibonacci number of {} is {}.'.format( x, res) )
   print( 'Calculation took {}.'.format(end-start) )



if __name__ == "__main__":
    main()
