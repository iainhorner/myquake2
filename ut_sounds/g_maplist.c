#include "g_local.h"

void remove_new_line(char* string);
char *mapListNames[MAX_MAPS] = { 0 };

void GetMapList()
{
	char* tmpMapNames[MAX_MAPS][MAX_MAP_NAME_LEN] = { 0 };

	char path[100] = { 0 };
	char null[5];

	char* gameName = GAMEVERSION;

	strcpy(path, gameName);
	strcat(path, "/");
	strcat(path, sv_maplist->string);


	FILE* file = fopen(path, "r"); /* should check the result */

	if (file == NULL) { //maplist file not found!
		Com_Printf("mapfile '%s' NOT found!\n", path);
		return;
	}

	char line[256];

	int i = 0;
	while (i < MAX_MAPS && fgets(line, sizeof(line), file)) {
		/* note that fgets don't strip the terminating \n, checking its
		   presence would allow to handle lines longer that sizeof(line) */
		printf("%s", line);
		remove_new_line(line);
		strcpy(tmpMapNames[i], line);
		i++;
	}

	fclose(file);

	char* nextMapName;

	i = 0;
	do {
		nextMapName = &tmpMapNames[i][0];
		if (strlen(nextMapName) != 0) {
			mapListNames[i] = &tmpMapNames[i][0];
			++i;
		}

	} while (strlen(nextMapName) != 0);


	int n, j, count;
	char temp[25];
	count = i - 1;
	for (i = 0; i <= count; i++)
		for (j = i + 1; j <= count; j++) {
			if (strcmp(mapListNames[i], mapListNames[j]) > 0) {
				strcpy(temp, mapListNames[i]);
				strcpy(mapListNames[i], mapListNames[j]);
				strcpy(mapListNames[j], temp);
			}
		}
}


void remove_new_line(char* string)
{
	size_t length = strlen(string);
	if ((length > 0) && (string[length - 1] == '\n'))
	{
		string[length - 1] = '\0';
	}
}


