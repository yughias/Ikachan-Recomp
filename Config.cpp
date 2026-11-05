#include "Config.h"
#include "System.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

GameConfig gConfig = {
	SDL_TRUE // default: autosave_on_exit = 1
};

static void TrimString(char *s)
{
	if (!s) return;
	char *p = s;
	while (*p && isspace((unsigned char)*p)) p++;
	if (p != s) memmove(s, p, strlen(p) + 1);
	size_t len = strlen(s);
	while (len > 0 && isspace((unsigned char)s[len - 1])) {
		s[--len] = '\0';
	}
}

static bool StrCaseEqual(const char *a, const char *b)
{
	if (!a || !b) return false;
	while (*a && *b) {
		if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
			return false;
		a++;
		b++;
	}
	return *a == '\0' && *b == '\0';
}

void SaveConfig(const char *filename)
{
	FILE *fp = fopen(filename, "w");
	if (!fp) return;

	fprintf(fp, "# Ikachan Configuration File\n\n");
	fprintf(fp, "[Game]\n");
	fprintf(fp, "# Enable autosave on exit (0 = disabled, 1 = enabled)\n");
	fprintf(fp, "# When enabled, the game automatically saves session state upon exit\n");
	fprintf(fp, "# and seamlessly resumes gameplay when launched again.\n");
	fprintf(fp, "autosave_on_exit = %d\n", gConfig.autosave_on_exit ? 1 : 0);

	fclose(fp);
}

void LoadConfig(const char *filename)
{
	// Default values
	gConfig.autosave_on_exit = SDL_TRUE;

	FILE *fp = fopen(filename, "r");
	if (!fp)
	{
		// Try gModulePath if available
		if (gModulePath[0] != '\0')
		{
			char fullPath[MAX_PATH];
			snprintf(fullPath, sizeof(fullPath), "%s/%s", gModulePath, filename);
			fp = fopen(fullPath, "r");
		}
	}

	if (!fp)
	{
		// Create default config file if it does not exist
		SaveConfig(filename);
		return;
	}

	char line[256];
	while (fgets(line, sizeof(line), fp))
	{
		TrimString(line);

		// Skip empty lines and comments
		if (line[0] == '\0' || line[0] == '#' || line[0] == ';')
			continue;

		// Skip section headers
		if (line[0] == '[' && line[strlen(line) - 1] == ']')
			continue;

		char *eq = strchr(line, '=');
		if (!eq)
			continue;

		*eq = '\0';
		char *key = line;
		char *val = eq + 1;
		TrimString(key);
		TrimString(val);

		if (StrCaseEqual(key, "autosave_on_exit"))
		{
			if (StrCaseEqual(val, "1") || StrCaseEqual(val, "true") ||
			    StrCaseEqual(val, "yes") || StrCaseEqual(val, "on"))
			{
				gConfig.autosave_on_exit = SDL_TRUE;
			}
			else if (StrCaseEqual(val, "0") || StrCaseEqual(val, "false") ||
			         StrCaseEqual(val, "no") || StrCaseEqual(val, "off"))
			{
				gConfig.autosave_on_exit = SDL_FALSE;
			}
		}
	}

	fclose(fp);
}
