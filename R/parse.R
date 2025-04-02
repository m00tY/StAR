library(stringr)

createSignatureBlock <- function(fileName, startLine, endLine) {
    return(c(fileName, startLine, endLine))
}

destroySignatureBlock <- function(fileName, funName) {
    if (exists(toString(fileName + ":" + funName), contracts)) {
        rm(funName, envir = contracts)
    }
}

validateLinterTag <- function(block) {
    validTags <- list()
    
    fileName <- block[[1]]
    startLine <- as.numeric(block[[2]])
    endLine <- as.numeric(block[[3]])
    
    lines <- readLines(fileName, warn = FALSE)
    blockLines <- lines[startLine:endLine]
    
    for (i in seq_along(blockLines)) {
        match <- str_match(blockLines[i], "#\\s*(@param|@return)\\s*(\\S+)?:\\s(\\S+)") #valid tags defined here
        if (!is.na(match[2]) && (match[2] %in% validLinterTags)) {
            validTags <- append(validTags, match[2])
        }
    }
    
    return(validTags)
}

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

identifySignatureBlocks <- function(fileName) {
    signatureBlocks <- list()
    
    regexMatches <- c()
    lines <- readLines(fileName, warn = FALSE)
    lineNumber <- 0
    
    for (line in lines) {
        lineNumber <- lineNumber + 1
        if (str_detect(line, "#\\s*@\\S+")) {
            regexMatches <- c(regexMatches, lineNumber)
        }
    }
    
    consecutiveLineNumbers <- identifyConsecutiveLineNumbers(regexMatches)
    for (lineNumberGroup in consecutiveLineNumbers) {
        start <- as.numeric(lineNumberGroup[1])
        end <- as.numeric(lineNumberGroup[length(lineNumberGroup)])
        
        block <- createSignatureBlock(fileName, start, end)
        validTags <- validateLinterTag(block)
        
        # Add both the signature block and its valid tags as a list element
        signatureBlocks <- append(signatureBlocks, list(list(block = block, tags = validTags)))
    }
    
    return(signatureBlocks)
}

# Example usage
result <- identifySignatureBlocks("R/star.R")
print(result)
