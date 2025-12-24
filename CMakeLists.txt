cmake_minimum_required(VERSION 3.18)
project(prosim C)

set(CMAKE_C_STANDARD 99)

add_executable(prosim main.c context.c context.h node.c node.h prio_q.h prio_q.c barrier.c messages.c)
