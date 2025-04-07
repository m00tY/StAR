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

validLinterTags <- c("@param", "@return")

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
        } else if (!is.na(match[3]) && match[3] %in% validTypeSignatures) {
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
        
        # Try to find the next function definition
        funName <- NA
        for (i in (end + 1):length(lines)) {
            match <- str_match(lines[i], "^(\\w+)\\s*<-\\s*function\\s*\\(")
            if (!is.na(match[2])) {
                funName <- match[2]
                break
            }
        }
        
        paramTypeMap <- list()
        blockLines <- lines[start:end]
        
        for (line in blockLines) {
            match <- str_match(line, "#\\s*(@param|@return)\\s+(\\S+):?\\s*(\\S+)?")
            if (!is.na(match[2])) {
                tag <- match[2]
                name <- str_replace(match[3], ":$", "")
                type <- match[4]
                
                if (!is.na(type) && type %in% validTypeSignatures) {
                    paramTypeMap[[name]] <- list(tag = tag, type = type)
                } else if (name %in% validTypeSignatures) {
                    paramTypeMap[[name]] <- list(tag = tag, type = name)
                }
            }
        }
        
        signatureBlocks[[length(signatureBlocks) + 1]] <- list(
            block = block,
            tags = validTags,
            parameters = parameterSymbols,
            types = validTypes,
            mapping = paramTypeMap,
            funName = funName
        )
    }
    
    return(signatureBlocks)
}

parseFunctionCalls <- function(fileName) {
    parsed <- parse(fileName, keep.source = TRUE)
    sourceData <- getParseData(parsed)
    sourceData <- sourceData[order(sourceData$line1, sourceData$col1), ]
    
    calls <- list()
    i <- 1
    sigBlocks <- createSignatureBlocks(fileName)
    
    while (i <= nrow(sourceData)) {
        token <- sourceData[i, ]
        
        if (token$token == "SYMBOL_FUNCTION_CALL" && token$text != "return") {
            funcName <- token$text
            startLine <- token$line1
            parenCount <- 0
            callTokens <- c(funcName)
            callTypes <- c()
            
            j <- i + 1
            expr <- ""
            
            while (j <= nrow(sourceData)) {
                t <- sourceData[j, ]
                callTokens <- c(callTokens, t$text)
                expr <- paste0(expr, t$text)
                
                if (t$text == "(") {
                    parenCount <- parenCount + 1
                } else if (t$text == ")") {
                    parenCount <- parenCount - 1
                    if (parenCount == 0) {
                        break
                    }
                }
                
                j <- j + 1
            }
            
            callText <- paste(callTokens, collapse = "")
            typesChecked <- TRUE
            
            tryCatch({
                callExpr <- parse(text = callText)[[1]]
                args <- as.list(callExpr)[-1]
                
                for (arg in args) {
                    evaluatedArg <- tryCatch(eval(arg, envir = .GlobalEnv), error = function(e) NULL)
                    argType <- if (!is.null(evaluatedArg)) typeof(evaluatedArg) else "unknown"
                    callTypes <- c(callTypes, argType)
                }
            }, error = function(e) {
                callTypes <- c(callTypes, "error_parsing_call")
            })
            
            matchedSignature <- NULL
            for (block in sigBlocks) {
                if (!is.null(block$funName) && block$funName == funcName) {
                    matchedSignature <- block
                    break
                }
            }
            
            calls[[length(calls) + 1]] <- list(
                name = funcName,
                line = startLine,
                call = callText,
                types = callTypes,
                matchedSignature = matchedSignature
            )
            
            i <- j
        } else {
            i <- i + 1
        }
    }
    
    return(calls)
}

parseFunctionCalls("R/star.R")