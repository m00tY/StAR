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
        match <- str_match(line, "#\\s*(@param|@return)\\s+(\\S+):?\\s*(\\S+)?")
        
        if (!is.na(match[4]) && match[4] %in% validTypeSignatures) {
            validTypes <- c(validTypes, match[4])
        } else if (is.na(match[4]) && match[3] %in% validTypeSignatures) {
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

# identify function type signature comment blocks and map each parameter to a given type
createSignatureBlocks <- function(fileName) {
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
        
        # Create mapping from parameter name to type
        paramTypeMap <- list()
        blockLines <- lines[start:end]
        
        for (line in blockLines) {
            match <- str_match(line, "#\\s*(@param|@return)\\s+(\\S+):?\\s*(\\S+)?")
            
            if (!is.na(match[2])) {
                tag <- match[2]
                name <- str_replace(match[3], ":$", "")  # <-- remove trailing colon
                type <- match[4]
                
                if (!is.na(type) && type %in% validTypeSignatures) {
                    paramTypeMap[[name]] <- list(tag = tag, type = type)
                } else if (name %in% validTypeSignatures) {
                    paramTypeMap[[name]] <- list(tag = tag, type = name)
                }
            }
        }
        
        mappedSignatureBlocks <- c(signatureBlocks, list(
            list(
                block = block,
                tags = validTags,
                parameters = parameterSymbols,
                types = validTypes,
                mapping = paramTypeMap
            )
        ))
    }
    
    return(mappedSignatureBlocks)
}

identifyFunctionCalls <- function(fileName) {
    # Parse the file and get source data
    parsed <- parse(fileName, keep.source = TRUE)
    sourceData <- getParseData(parsed)
    
    # Sort by line and column for token ordering
    sourceData <- sourceData[order(sourceData$line1, sourceData$col1), ]
    
    calls <- list()
    i <- 1 
    
    while (i <= nrow(sourceData)) {
        token <- sourceData[i, ]
        
        if (token$token == "SYMBOL_FUNCTION_CALL" && token$text != "return") {
            funcName <- token$text
            startLine <- token$line1
            
            # Initialize parentheses counter
            parenCount <- 0
            callTokens <- c(funcName)  # Start the function call with the function name
            
            j <- i + 1
            
            while (j <= nrow(sourceData)) {
                t <- sourceData[j, ]
                callTokens <- c(callTokens, t$text)
                
                # Track parentheses to handle nested functions
                if (t$text == "(") {
                    parenCount <- parenCount + 1
                } else if (t$text == ")") {
                    parenCount <- parenCount - 1
                    if (parenCount == 0) {
                        break  # Stop once parentheses are balanced
                    }
                }
                
                j <- j + 1
            }
            
            callText <- paste(callTokens, collapse = "")
            
            calls[[length(calls) + 1]] <- list(
                name = funcName,
                line = startLine,
                call = callText
            )
            i <- j
        } else {
            i <- i + 1
        }
    }
    
    # Return the list of function calls
    return(calls)
}



verifyFunctionCalls <- function(fileName) {
    mappedSignatureBlocks <- createSignatureBlocks(fileName)
    
}

#identifyFunctionCalls("R/star.R")
#verifyFunctionCalls("R/star.R")

identifyFunctionCalls("R/test.R")