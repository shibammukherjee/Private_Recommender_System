# Setup SEAL #
git submodule add https://github.com/microsoft/SEAL.git (inside code)
cmake -S . -B build
cmake --build build
sudo cmake --install build

sudo apt-get install libmpfr-dev libgmp-dev libboost-all-dev libssl-dev libbsd-dev

# Setup ABY #
git submodule add https://github.com/encryptogroup/ABY.git (inside code)
mkdir build
cd build
cmake ..

Required packages for ABY:

    g++ (version >=8) or another compiler and standard library implementing C++17 including the filesystem library
    make
    cmake
    libgmp-dev
    libssl-dev
    libboost-all-dev

# Setup Project # (inside code)
mkdir build
cd build
cmake ..
make -j 
make run