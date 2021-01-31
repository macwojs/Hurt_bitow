cmake_minimum_required(VERSION 3.17)
project(Hurt_bitow C)

set(CMAKE_C_STANDARD 11)

add_executable(Hurt_bitow_producent producent.c)

add_executable(Hurt_bitow_klient konsument.c)