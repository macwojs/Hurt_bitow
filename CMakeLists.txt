cmake_minimum_required(VERSION 3.17)
project(Hurt_bitow C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

add_executable(Hurt_bitow_producent producent.c producent.h parse.c parse.h list.c list.h)

add_executable(Hurt_bitow_klient konsument.c konsument.h parse.c parse.h)