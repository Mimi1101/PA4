cmake_minimum_required(VERSION 3.26)
project(PA4_Operating_Systems C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(PA4_Operating_Systems
        exercisesoftwaredisk.c
        filesystem.c
        filesystem.h
        softwaredisk.c
        softwaredisk.h
        testfs0.c
        testfs4a.c)
