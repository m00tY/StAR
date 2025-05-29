# @contract f (numeric, numeric) -> numeric
f <- function(value, other) {
  return(value + other)
}

# @contract g (numeric, numeric) -> logical
g <- function(number, alsonumber) {
	return((number * alsonumber) %% 2 == 0)
}

# @contract h (numeric[]) -> numeric
h <- function(L) {
	sum <- 0
	for (num in L) {
		sum <- num + sum
	}
	return (sum)
}

# @contract k (dataframe, dataframe) -> dataframe
k <- function(df1, df2) {
	return (df1*df2)
}

f(1L, 2L)

g(2.0, 3.0)

h(c(1L, 2L, 3L))

dataframe1 <- data.frame(field1=1:4, field2=5:8, field3=9:12)
dataframe2 <- data.frame(field1=9:12, field2=5:8, field3=1:4)

k(dataframe1, dataframe2)