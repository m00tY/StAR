# @contract f (integer, integer) -> integer
f <- function(value, other) {
  return(value + other)
}

# @contract g (double, double) -> logical
g <- function(number, alsonumber) {
	return((number * alsonumber) %% 2 == 0)
}

# @contract h (integer[]) -> integer
h <- function(L) {
	sum <- 0
	for (num in L) {
		sum <- num + sum
	}
	return (sum)
}

f(1, 2)

f(2, 3.5)

g(2.0, 3.0)

h(c(1L, 2L, 3L))
