# @param value: integer
# @param other: double
# @return double
f <- function(value, other) {
    return (value+other)
}

# @param number: double
# @param alsonumber: double
# @return logical
g <- function (number, alsonumber) {
    return ((number * alsonumber) %% 2 == 0)
}

f(1, 2.5)

f(2, 3.5)