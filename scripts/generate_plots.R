library(tidyverse)
library(scales)

args <- commandArgs(trailingOnly = TRUE)
data_path <- args[1]
output_path <- args[2]

data <- read.csv(data_path)
names(data) <- c("samples", "mse")

pdf(output_path, width = 10, height = 7)

data %>%
	ggplot(aes(x = samples, y = mse)) +
	geom_line() +
	scale_x_log10() +
    scale_y_log10() +
	labs(title = "MSE against reference image", x = "Samples", y = "MSE")

dev.off()
