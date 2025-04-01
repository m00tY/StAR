createContractChunk <- function(fileName, startLine, endLine) {
    return (c(fileName, startLine, endLine))
}

destroyContractChunk <- function(fileName, funName) {
    if (exists(toString(fileName + ":" + funName), contracts)) {
        rm(funName, envir = contracts)
    }
}
