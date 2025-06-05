# @contract generate_data (numeric) -> dataframe
generate_data <- function(n) {
  data.frame(
    id = 1:n,
    age = round(runif(n, 18, 70)),
    income = round(rnorm(n, mean = 50000, sd = 15000)),
    gender = sample(c("Male", "Female"), n, replace = TRUE),
    stringsAsFactors = FALSE
  )
}

# @contract clean_data (dataframe) -> dataframe
clean_data <- function(df) {
  df <- df[df$income > 0, ]  # Remove non-positive incomes
  df <- df[df$age >= 18 & df$age <= 100, ]
  return(df)
}

# @contract summarize_by_gender (dataframe) -> dataframe
summarize_by_gender <- function(df) {
  aggregate(income ~ gender, data = df, FUN = function(x) {
    c(mean = mean(x), median = median(x), sd = sd(x))
  })
}

# @contract plot_income_distribution (dataframe) -> void
plot_income_distribution <- function(df) {
  hist(df$income,
       breaks = 30,
       main = "Income Distribution",
       xlab = "Income",
       col = "skyblue",
       border = "white")
}

main <- function() {
  raw_data <- generate_data(100)
  cat("Generated", nrow(raw_data), "rows\n")
  
  clean <- clean_data(raw_data)
  cat("After cleaning:", nrow(clean), "rows\n")
  
  print(summarize_by_gender(clean))
  
  plot_income_distribution(clean)
}

# Run the pipeline
main()