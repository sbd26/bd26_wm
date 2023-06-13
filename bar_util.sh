#!/bin/bash
filename="/home/bd26/.cache/bd26_util.txt"
first_line=$(head -n 1 "$filename")

GREEN="%{F#00FF00}"
YELLOW="%{F#FFFF00}"
BLUE="%{F#0000FF}"
RESET="%{F-}"

# Use the head command to extract the first line of the file
first_line=$(head -n 1 "$filename")
workspace=(" " " " " " "[]")

if [ "$first_line" = "0" ]; then
    workspace[0]="${GREEN} ${RESET}"
fi

if [ "$first_line" = "1" ]; then
    workspace[1]="${GREEN} ${RESET}"
fi

if [ "$first_line" = "2" ]; then
    workspace[2]="${GREEN} ${RESET}"
fi

if [ "$first_line" = "3" ]; then
    workspace[3]="${GREEN}[]${RESET}"
fi

for element in "${workspace[@]}"; do
    echo -n "$element "
done
