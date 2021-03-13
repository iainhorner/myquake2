#include "server.h"


void remove_new_line(char* string);
char *mapNames[MAX_NUM_MAPS][MAX_MAP_NAME_LEN];


void GetMapList()
{
	char path[100];
	char null[5];
	char *dir;
	char *noGameName = "";
	

	dir = (char*)malloc(15);
	strcpy(dir, "baseq2/");

	if (strcmp(game_name->string, noGameName)==0)
		strcpy(path, dir);
	else {
		strcpy(path, game_name->string);
		strcat(path, "/");
	}

	strcat(path, maplist_file->string);

	FILE* file = fopen(path, "r"); /* should check the result */

	if (file == NULL) { //maplist file not found!
		Com_Printf("mapfile '%s' NOT found!\n", LOG_SERVER | LOG_WARNING, path);
		return;
	}
	else
	{
		Com_Printf("mapfile '%s' found!\n", LOG_SERVER | LOG_WARNING, path);
	}


	char line[256];

	int i = 0;
	while (i< MAX_NUM_MAPS && fgets(line, sizeof(line), file)) {
		/* note that fgets don't strip the terminating \n, checking its
		   presence would allow to handle lines longer that sizeof(line) */
		printf("%s", line);
		remove_new_line(line);
		strcpy(mapNames[i], line);
		i++;
	}

	fclose(file);
}


void remove_new_line(char* string)
{
	size_t length = strlen(string);
	if ((length > 0) && (string[length - 1] == '\n'))
	{
		string[length - 1] = '\0';
	}
}



