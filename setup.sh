: '
* Copyright (c) 2018 Intel Corporation.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
* LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
* OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
* WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
'
PROJECT_DIRECTORY=$(pwd)
sudo apt-get update

# Install Casablanca for Azure Storage
sudo apt-get install libcpprest-dev
sudo apt-get install g++ git libboost-atomic-dev libboost-thread-dev libboost-system-dev libboost-date-time-dev libboost-regex-dev libboost-filesystem-dev libboost-random-dev libboost-chrono-dev libboost-serialization-dev libwebsocketpp-dev openssl libssl-dev ninja-build
git clone https://github.com/Microsoft/cpprestsdk.git casablanca
cd casablanca
git checkout 43773b9858f0672a4d5856a48f79d97193b295d9
mkdir build.debug
cd build.debug
cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Debug
ninja
sudo ninja install
cd $PROJECT_DIRECTORY

# Azure Storage Client Library
sudo apt-get install gcc-4.8
sudo ln -s /usr/bin/g++ /usr/bin/g++-4.8
sudo apt-get install libboost-all-dev
git clone https://github.com/Azure/azure-storage-cpp.git
sudo apt-get install libxml2-dev uuid-dev
cd azure-storage-cpp/Microsoft.WindowsAzure.Storage
mkdir build.release
cd build.release
CASABLANCA_DIR=../../../casablanca/ CXX=g++-4.8 cmake .. -DCMAKE_BUILD_TYPE=Release
make
cd $PROJECT_DIRECTORY

# Cloning json parser from github
if [ -d "json" ]
then
  sudo rm -r json
fi

git clone https://github.com/nlohmann/json

# Downloading the video required for the RI
cd resources
wget -O store-aisle-detection.mp4 https://github.com/intel-iot-devkit/sample-videos/raw/master/store-aisle-detection.mp4
