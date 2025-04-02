library(stringr)

createSignatureBlock <- function(fileName, startLine, endLine) {
    return(c(fileName, startLine, endLine))
}

destroySignatureBlock <- function(fileName, funName) {
    if (exists(paste(fileName, funName, sep = ":"), envir = contracts)) {
        rm(funName, envir = contracts)
    }
}

validLinterTags <- c("@param", "@return")  # Define valid tags for argument

validateLinterTag <- function(block) {
    validTags <- c()
    
    fileName <- block[[1]]
    startLine <- as.numeric(block[[2]])
    endLine <- as.numeric(block[[3]])
    
    lines <- readLines(fileName, warn = FALSE)
    blockLines <- lines[startLine:endLine]
    
    for (i in seq_along(blockLines)) {
        match <- str_match(blockLines[i], "#\\s*(@param|@return)\\s*(\\S+)?:\\s(\\S+)") # update according to validLinterTags
        
        if (!is.na(match[2]) && match[2] %in% validLinterTags) {
            validTags <- c(validTags, match[2])
        }
    }
    
    return(validTags)
}

validTypeSignatures <- c("logical", "integer", "double", "complex", "character", "raw")

validateTypeSignature <- function(block) {
    validTypes <- c()
    
    fileName <- block[[1]]
    startLine <- as.numeric(block[[2]])
    endLine <- as.numeric(block[[3]])
    
    lines <- readLines(fileName, warn = FALSE)
    blockLines <- lines[startLine:endLine]
    
    for (i in seq_along(blockLines)) {
        match <- str_match(blockLines[i], "#\\s*(@param|@return)\\s*(\\S+)?\\s*(logical|integer|double|complex|character|raw)") 
        
        if (!is.na(match[4])) {
            validTypes <- c(validTypes, match[4])
        } else if (is.na(match[4]) && match[2] %in% validTypeSignatures) {
            validTypes <- c(validTypes, match[2])
        } else {
            validTypes <- c(validTypes, "something broke")
        }
    }
    
    return(validTypes)
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
        validTypes <- validateTypeSignature(block)
        
        # Add both the signature block and its valid tags as a list element
        signatureBlocks <- c(signatureBlocks, list(list(block = block, tags = validTags, types = validTypes)))
    }
    
    return(signatureBlocks)
}
