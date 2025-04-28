# @contract (integer, integer) -> integer
f <- function(value, other) {
  return(value + other)
}

# @contract (double, double) -> logical
g <- function(number, alsonumber) {
	return((number * alsonumber) %% 2 == 0)
}

f(1, 2)

f(2, 3.5)

g(2.0, 3.0)
