# Makefile for Project 1725
# Perspective Image Preprocessor
# Dependencies:
# GLFW3
# GLEW

CFLAGS	= `pkg-config --cflags glfw3 glew`
LIBS	= `pkg-config --libs glfw3 glew`

CFLAGS	+= -std=c++11
CFLAGS += -O2

LIBS += -lpthread

main : main.o Shader.o
	g++ main.o Shader.o -o main ${LIBS}

main.o : main.cpp
	g++ -c main.cpp -o main.o ${CFLAGS}

Shader.o : Shader.cpp Shader.h
	g++ -c Shader.cpp -o Shader.o ${CFLAGS}

