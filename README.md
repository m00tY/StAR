# Star
Star is a source-to-source compiler designed to implement dynamic type contracts for the R programming language.

## Example Code
```r
# Function with a declared type contract.
# Takes two integers as input
# Returns an integer

# @contract f (integer, integer) -> integer
f <- function(value, other) {
  return(value + other)
}

f(1L, 2L) # Runs with no issue

f(2, 3.5) # Program halts, contract breached
```


## Building From Source
On Mac/Linux:
```bash
mkdir build
cd build
cmake ..
make
```

## Usage
```bash
star run <input filename> -o <output filename> 
```

