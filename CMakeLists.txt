cmake_minimum_required(VERSION 3.17)
project(Hurt_bitow C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

add_executable(Hurt_bitow_producent producent.c parse.c parse.h)

add_executable(Hurt_bitow_klient konsument.c parse.c parse.h)