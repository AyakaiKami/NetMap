#!/bin/bash

# Specify the name of your C++ source file
CPP_FILE="client.cpp"
rm -f "client.exe"
# Specify the name of the output executable
OUTPUT_EXECUTABLE="client.exe"

# Compiler command
g++ -o $OUTPUT_EXECUTABLE $CPP_FILE -lsfml-graphics -lsfml-window -lsfml-system -pthread

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful. Executable: ./$OUTPUT_EXECUTABLE"
else
    echo "Compilation failed."
fi

# Specify the name of your C++ source file
CPP_FILE="server.cpp"
rm -f "server.exe"
# Specify the name of the output executable
OUTPUT_EXECUTABLE="server.exe"

# Compiler command
g++ -o $OUTPUT_EXECUTABLE $CPP_FILE -pthread -lvirt -lvirt-qemu -lsqlite3

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful. Executable: ./$OUTPUT_EXECUTABLE"
else
    echo "Compilation failed."
fi