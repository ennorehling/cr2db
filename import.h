#pragma once

#include <stdio.h>

struct gamedata;

int import(struct gamedata *gd, FILE *F, const char *filename);
