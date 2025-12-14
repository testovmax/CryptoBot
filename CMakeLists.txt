cmake_minimum_required(VERSION 3.14)
project(CryptoBot CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Источники
file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(crypto_bot ${SOURCES})

# Под Windows: используем pthread
if(WIN32)
    target_link_libraries(crypto_bot pthread)
endif()