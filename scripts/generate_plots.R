library(tidyverse)
library(scales)

args <- commandArgs(trailingOnly = TRUE)
data_path <- args[1]
output_path <- args[2]

data <- read.csv(data_path)

names(data) <- c("name", "samples", "mse", "time_ms")

pdf(output_path, width = 10, height = 7)

print(
    data %>%
        group_by(name) %>%
        ggplot(aes(x = log2(samples), y = mse, color = name)) +
        geom_line() +
        scale_x_continuous("Samples", labels = math_format(2^.x)) +
        scale_y_continuous("MSE", trans = "log2", labels = label_number()) +
        labs(title = "Log MSE against reference image", color = "Name")
)

print(
    data %>%
        group_by(name) %>%
        ggplot(aes(x = time_ms, y = mse, color = name)) +
        geom_line() +
        scale_x_continuous("Time (ms)", trans = "log2", labels = label_number()) +
        scale_y_continuous("MSE", trans = "log2", labels = label_number()) +
        labs(title = "Log MSE against reference image over time", color = "Name")
)

dev.off()
