library(stringr)
source("R/contract.R")

identifyConsecutiveLineNumbers <- function(numbers) {
    if (length(numbers) == 0) return(list())  # Handle empty input
    
    groups <- list()
    currentGroup <- c(numbers[1])
    
    for (i in 2:length(numbers)) {
        if (numbers[i] == numbers[i - 1] + 1) {
            currentGroup <- c(currentGroup, numbers[i])
        } else {
            groups <- append(groups, list(currentGroup))
            currentGroup <- c(numbers[i])
        }
    }
    
    groups <- append(groups, list(currentGroup))
    return(groups)
}

# Hashmap to store all line numbers of identified contracts
contractChunks <- list()

identifyContractChunks <- function(fileName) {
    regexMatches <- c()
    lines <- readLines(fileName, warn = FALSE)
    lineNumber <- 0
    
    for (line in lines) {
        lineNumber <- lineNumber + 1
        if (str_detect(line, "#\\s*@\\S+\\s*(\\S+):\\s(\\S+)")) {
            regexMatches <- c(regexMatches, lineNumber)
        }
    }
    
    consecutiveLineNumbers <- identifyConsecutiveLineNumbers(regexMatches)
    for (lineNumberGroup in consecutiveLineNumbers) {
        start <- lineNumberGroup[1]
        end <- lineNumberGroup[length(lineNumberGroup)]
        
        contractChunks <<- append(contractChunks, list(createContractChunk(fileName, start, end)))
    }
}

# TODO: 
