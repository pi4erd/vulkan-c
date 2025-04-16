#!/bin/sh

CFLAGS=-std=c17 -Wall -Wextra -Wpedantic
LDFLAGS=

TARGET=$(BINDIR)/embedder

$(TARGET): embedder.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
