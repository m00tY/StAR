library(stringr)

createSignatureBlock <- function(fileName, startLine, endLine) {
    return(c(fileName, startLine, endLine))
}

destroySignatureBlock <- function(fileName, funName) {
    fullName <- paste(fileName, funName, sep = ":")
    if (exists(fullName, envir = .GlobalEnv)) {
        rm(list = fullName, envir = .GlobalEnv)
    }
}

validLinterTags <- c("@param", "@return")  # Define valid tags

validateLinterTag <- function(block) {
    validTags <- c()
    
    fileName <- block[[1]]
    startLine <- as.numeric(block[[2]])
    endLine <- as.numeric(block[[3]])
    
    lines <- readLines(fileName, warn = FALSE)
    blockLines <- lines[startLine:endLine]
    
    for (line in blockLines) {
        match <- str_match(line, "#\\s*(@param|@return)")
        
        if (!is.na(match[2]) && match[2] %in% validLinterTags) {
            validTags <- c(validTags, match[2])
        }
    }
    
    return(validTags)
}

validateParameterSymbols <- function(block) {
    parameterSymbols <- c()
    
    fileName <- block[[1]]
    startLine <- as.numeric(block[[2]])
    endLine <- as.numeric(block[[3]])
    
    lines <- readLines(fileName, warn = FALSE)
    blockLines <- lines[startLine:endLine]
    
    for (line in blockLines) {
        match <- str_match(line, "#\\s*(@param)\\s+(\\S+):")
        
        if (!is.na(match[3])) {
            parameterSymbols <- c(parameterSymbols, match[3])
        }
    }
    
    return(parameterSymbols)
}

validTypeSignatures <- c("logical", "integer", "double", "complex", "character", "raw")

validateTypeSignature <- function(block) {
    validTypes <- c()
    
    fileName <- block[[1]]
    startLine <- as.numeric(block[[2]])
    endLine <- as.numeric(block[[3]])
    
    lines <- readLines(fileName, warn = FALSE)
    blockLines <- lines[startLine:endLine]
    
    for (line in blockLines) {
        match <- str_match(line, "#\\s*(@param|@return)\\s+\\S+:?\\s*(\\S+)")
        
        if (!is.na(match[3]) && match[3] %in% validTypeSignatures) {
            validTypes <- c(validTypes, match[3])
        }
    }
    
    return(validTypes)
}

identifyConsecutiveLineNumbers <- function(numbers) {
    if (length(numbers) == 0) return(list())
    
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
    for (i in seq_along(lines)) {
        if (str_detect(lines[i], "#\\s*@\\S+")) {
            regexMatches <- c(regexMatches, i)
        }
    }
    
    consecutiveLineNumbers <- identifyConsecutiveLineNumbers(regexMatches)
    for (lineGroup in consecutiveLineNumbers) {
        start <- as.numeric(lineGroup[1])
        end <- as.numeric(lineGroup[length(lineGroup)])
        
        block <- createSignatureBlock(fileName, start, end)
        validTags <- validateLinterTag(block)
        parameterSymbols <- validateParameterSymbols(block)
        validTypes <- validateTypeSignature(block)
        
        signatureBlocks <- c(signatureBlocks, list(list(block = block, tags = validTags, parameters = parameterSymbols, types = validTypes)))
    }
    
    return(signatureBlocks)
}

sig <- identifySignatureBlocks("R/star.R")
print(sig)