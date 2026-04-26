library(tidyverse)
library(scales)

args <- commandArgs(trailingOnly = TRUE)
data_path <- args[1]
output_path <- args[2]

data <- read.csv(data_path)

# ensure expected columns
names(data) <- c("name", "samples", "mse")

pdf(output_path, width = 10, height = 7)

data %>%
    group_by(name) %>%
    ggplot(aes(x = log2(samples), y = log2(mse), color = name)) +
    geom_line() +
    scale_x_continuous("Samples", labels = math_format(2^.x)) +
    scale_y_continuous("Log MSE") +
    labs(title = "Log MSE against reference image", color = "Name")

dev.off()
