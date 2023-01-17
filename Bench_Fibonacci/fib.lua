-- SPDX-FileCopyrightText: Copyright (C) 2023 Florian Thake, <contact |at| tea-age.solutions>.
-- SPDX-License-Identifier: MIT

-- recursive fibonacci in lua
fib = function ( n )
  if n == 1 or n == 0 then return n
  else return fib( n - 2 ) + fib( n - 1 )
  end
end

start_time = os.clock()

res = fib( 25 )

end_time = os.clock()

print( 'value: ', res )
print(string.format("Calculation took: %.8f\n", end_time - start_time))
