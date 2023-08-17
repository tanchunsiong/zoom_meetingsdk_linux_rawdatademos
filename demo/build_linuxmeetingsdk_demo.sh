#!/bin/bash
current_directory="$PWD"
if [ -d "build" ]; then
    rm -r build
fi
mkdir build
cd build
dir_path=$1 ;

cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
if [ $? -eq 0 ]; then
    echo "meetingsdk linux demo cmake successfully."
else
    echo "meetingsdk linux demo executed failed."
    exit 1
fi

make -j8
if [ $? -eq 0 ]; then
    echo "meetingsdk linux demo make successfully."
else
    echo "meetingsdk linux demo make failed."
    exit 1
fi

if [ -f "${current_directory}/../libmeetingsdk.so.1" ]; then
    rm "${current_directory}/../libmeetingsdk.so.1"
fi

if [ ! -f "${current_directory}/../libmeetingsdk.so" ]; then
    echo "libmeetingsdk.so file does not exist"
    exit 1
fi

ln -s ${current_directory}/../libmeetingsdk.so ${current_directory}/../libmeetingsdk.so.1

