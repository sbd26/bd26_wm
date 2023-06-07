
#!/bin/bash
filename="/home/bd26/.cache/bd26_util.txt"
first_line=$(head -n 1 "$filename")

# Use the head command to extract the first line of the file
first_line=$(head -n 1 "$filename")

# Check the value of first_line and print the corresponding icon
if [[ "$first_line" == "0" ]]; then
    echo " "
elif [[ "$first_line" == "1" ]]; then
    echo " "
elif [[ "$first_line" == "2" ]]; then
    echo " "
else
    echo "[]"
fi
