#pragma once

#include <stdio.h>

struct gamedata;

int export_db(struct gamedata *gd, FILE *F);
int export_map(struct gamedata *gd, FILE *F);
