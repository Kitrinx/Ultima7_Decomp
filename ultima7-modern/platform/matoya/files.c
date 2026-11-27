/* The game's DOS files, found in the game's folder whatever their letter case.
 *
 * "STATIC\\u7ifix00." becomes <game>/static/U7IFIX00 when those are the names on disk.
 *
 * Writes go to the saves folder, <saves>/<Black Gate|Serpent Isle>, under the same relative
 * names; reads look there first, then in the game's folder, which is never written:
 *
 *   write STATIC\\ENDSTATS.DAT -> <saves>/Serpent Isle/STATIC/ENDSTATS.DAT
 *   read STATIC\\SHAPES.VGA    -> that name in <saves>, else <game>/STATIC/SHAPES.VGA
 *
 * The game in progress (GAMEDAT), saved games, gameargs.dat and temporary files are read from
 * the saves folder only, so the original games' saves are not picked up.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define mkdir(path, mode) _mkdir(path)
#define rmdir _rmdir
#else
#include <dirent.h>
#include <strings.h>
#include <sys/statvfs.h>
#include <unistd.h>
#endif

#include "backend.h"
#include "opl3score.h"

#define PATH_SIZE 1024
#define NAME_SIZE 256
#define MAX_HANDLES 64

enum { LAST_NONE, LAST_READ, LAST_WRITE };

typedef struct {
	FILE *file;
	int last;
} handle;

typedef struct {
	char name[13];
	uint32_t size;
} found_file;

typedef struct {
	found_file *files;
	uint32_t count;
	uint32_t next;
} find_list;

static char game_roots[2][PATH_SIZE] = {".", "."};
static char saves_dir[PATH_SIZE] = "Saves";
static handle handles[MAX_HANDLES + 1];

static void set_dir(char *out, const char *dir)
{
	size_t n;

	snprintf(out, PATH_SIZE, "%s", dir);
	n = strlen(out);
	while (n > 1 && (out[n - 1] == '/' || out[n - 1] == '\\'))
		out[--n] = '\0';
}

const char *files_game_root(int16_t game)
{
	return game_roots[game == 0 ? 0 : 1];
}

const char *files_root(void)
{
	return files_game_root(plat_played_game());
}

void files_set_game_root(int16_t game, const char *dir)
{
	set_dir(game_roots[game == 0 ? 0 : 1], dir);
}

void files_set_saves(const char *dir)
{
	set_dir(saves_dir, dir);
}

/* The played game's saves folder. */
static const char *save_root(void)
{
	static char path[PATH_SIZE];

	snprintf(path, sizeof path, "%s/%s", saves_dir, plat_played_game() == 0 ? "Black Gate" : "Serpent Isle");
	return path;
}

static bool is_separator(char c)
{
	return c == '/' || c == '\\';
}

static bool stat_path(const char *path, struct stat *st)
{
	return stat(path, st) == 0;
}

static bool is_file(const char *path)
{
	struct stat st;

	return stat_path(path, &st) && (st.st_mode & S_IFMT) == S_IFREG;
}

static bool is_dir(const char *path)
{
	struct stat st;

	return stat_path(path, &st) && (st.st_mode & S_IFMT) == S_IFDIR;
}

/* Makes the folder and any missing ones above it, from "/" or "\" paths, relative or not. */
bool files_make_dir(const char *dir)
{
	char path[PATH_SIZE];

	snprintf(path, sizeof path, "%s", dir);
	for (char *p = path + 1; *p != '\0'; p++) {
		if (is_separator(*p) && !is_separator(p[-1]) && p[-1] != ':') {
			char separator = *p;

			*p = '\0';
			if (!is_dir(path))
				mkdir(path, 0777);
			*p = separator;
		}
	}
	return is_dir(path) || mkdir(path, 0777) == 0;
}

static bool join(char *path, size_t size, const char *dir, const char *name)
{
	int n = snprintf(path, size, "%s/%s", dir, name);

	return n > 0 && (size_t) n < size;
}

/* Finds the name on disk of one entry of dir, ignoring case. */
static bool real_name(const char *dir, const char *name, char *real)
{
	char path[PATH_SIZE];
	bool found = false;

	if (join(path, sizeof path, dir, name) && stat_path(path, &(struct stat) {0})) {
		snprintf(real, NAME_SIZE, "%s", name);
		return true;
	}
#ifndef _WIN32
	DIR *d = opendir(dir);
	struct dirent *entry;

	if (d == NULL)
		return false;
	while (!found && (entry = readdir(d)) != NULL) {
		if (strcasecmp(entry->d_name, name) == 0) {
			snprintf(real, NAME_SIZE, "%s", entry->d_name);
			found = true;
		}
	}
	closedir(d);
#endif
	return found;
}

enum { FIND, MAKE_LAST, MAKE_ALL };

/* Turns a DOS name into a host path under base. With MAKE_LAST a missing last part is allowed
 * and spelt in upper case; MAKE_ALL also makes missing folders on the way. */
static bool resolve_in(const char *base, const char *dos, int make, char *path)
{
	const char *p = dos;
	char part[NAME_SIZE];
	char real[NAME_SIZE];

	if (make == FIND && !is_dir(base))
		return false;
	snprintf(path, PATH_SIZE, "%s", base);
	if (isalpha((unsigned char) p[0]) && p[1] == ':')
		p += 2;
	for (;;) {
		size_t n;
		bool last;

		while (is_separator(*p))
			p++;
		if (*p == '\0')
			return true;
		for (n = 0; p[n] != '\0' && !is_separator(p[n]); n++)
			;
		if (n >= sizeof part)
			return false;
		memcpy(part, p, n);
		part[n] = '\0';
		p += n;
		last = true;
		for (const char *q = p; *q != '\0'; q++) {
			if (!is_separator(*q))
				last = false;
		}

		if (strcmp(part, ".") == 0)
			continue;
		if (strcmp(part, "..") != 0) {
			/* "NAME." is NAME with no extension. */
			while (n > 0 && part[n - 1] == '.')
				part[--n] = '\0';
			if (n == 0)
				continue;
		}
		if (!real_name(path, part, real)) {
			if (make == FIND || (make == MAKE_LAST && !last))
				return false;
			for (size_t i = 0; i <= n; i++)
				real[i] = (char) toupper((unsigned char) part[i]);
		}
		if (strlen(path) + 1 + strlen(real) >= PATH_SIZE)
			return false;
		strcat(path, "/");
		strcat(path, real);
		if (make == MAKE_ALL && !last && !is_dir(path) && mkdir(path, 0777) != 0)
			return false;
	}
}

static const char *last_part(const char *dos)
{
	const char *last = dos;

	for (const char *p = dos; *p != '\0'; p++) {
		if (is_separator(*p))
			last = p + 1;
	}
	return last;
}

/* What the game keeps for itself: GAMEDAT and what is in it, saved games (GAMEnn.U7),
 * gameargs.dat and temporary files (*.$$$). These are never read from the game's folder. */
static bool saves_only(const char *dos)
{
	const char *p = dos, *name = last_part(dos), *dot = strrchr(name, '.');

	if (isalpha((unsigned char) p[0]) && p[1] == ':')
		p += 2;
	while (is_separator(p[0]) || (p[0] == '.' && (is_separator(p[1]) || p[1] == '\0')))
		p++;
	if (strncasecmp(p, "GAMEDAT", 7) == 0 && (p[7] == '\0' || is_separator(p[7])))
		return true;
	if (dot != NULL && strcmp(dot, ".$$$") == 0)
		return true;
	if (name != p)
		return false;
	return strcasecmp(name, "gameargs.dat") == 0 || (strlen(name) == 9 && strncasecmp(name, "GAME", 4) == 0
		&& isdigit((unsigned char) name[4]) && isdigit((unsigned char) name[5]) && strcasecmp(name + 6, ".U7") == 0);
}

/* Where a read finds the name: the saves folder, then the game's folder. */
static bool resolve(const char *dos, char *path)
{
	return resolve_in(save_root(), dos, FIND, path)
		|| (!saves_only(dos) && resolve_in(files_root(), dos, FIND, path));
}

/* Where a write puts the name: always the saves folder. Folders the game's folder has, such as
 * STATIC, are made there as needed; GAMEDAT only when the game makes it, as DOS required. */
static bool resolve_write(const char *dos, char *path)
{
	const char *last = last_part(dos);
	char dir_dos[PATH_SIZE], game_dir[PATH_SIZE];
	int make = MAKE_LAST;

	if (!files_make_dir(save_root()))
		return false;
	snprintf(dir_dos, sizeof dir_dos, "%.*s", (int) (last - dos), dos);
	if (!saves_only(dos) && resolve_in(files_root(), dir_dos, FIND, game_dir) && is_dir(game_dir))
		make = MAKE_ALL;
	return resolve_in(save_root(), dos, make, path);
}

/* ---- Handles ---- */

/* Closes whatever the last program left open. */
void files_reset(void)
{
	for (int i = 1; i <= MAX_HANDLES; i++) {
		if (handles[i].file != NULL)
			fclose(handles[i].file);
		handles[i].file = NULL;
		handles[i].last = LAST_NONE;
	}
}

static int16_t add_handle(FILE *file)
{
	if (file == NULL)
		return -1;
	for (int16_t i = 1; i <= MAX_HANDLES; i++) {
		if (handles[i].file == NULL) {
			handles[i].file = file;
			handles[i].last = LAST_NONE;
			return i;
		}
	}
	fclose(file);
	return -1;
}

/* The handle's stream, ready for a read or a write. C needs a seek between the two. */
static FILE *use_handle(int16_t file, int op)
{
	handle *h;

	if (file < 1 || file > MAX_HANDLES || handles[file].file == NULL)
		return NULL;
	h = &handles[file];
	if (op != LAST_NONE && h->last != LAST_NONE && h->last != op)
		fseek(h->file, 0, SEEK_CUR);
	h->last = op;
	return h->file;
}

/* SERPENT.CFG is the sound setup the DOS installer wrote: line 1 the music card, line 2 the
 * speech card's port, IRQ, DMA and an 'o' for the Sound Blaster Pro driver. Here music is MT-32
 * or AdLib emulation, and speech always has an output, so the game reads a setup to match:
 * "r 330" when the MT-32 plays, else "a 388", and the installer's default speech line when the
 * file has none. The file is never changed. Black Gate's U7.CFG is the same without the 'o'. */
static bool is_sound_config(const char *name)
{
	const char *file = plat_played_game() == 0 ? "u7.cfg" : "serpent.cfg";

	return strcasecmp(name, file) == 0 || (strncmp(name, ".\\", 2) == 0 && strcasecmp(name + 2, file) == 0);
}

static FILE *open_sound_config(const char *name)
{
	char path[PATH_SIZE], music[81] = "p", speech[81] = "", copy[81];
	int tokens = 0;
	FILE *f;

	if (resolve(name, path) && is_file(path) && (f = fopen(path, "rb")) != NULL) {
		if (fgets(music, sizeof music, f) == NULL || fgets(speech, sizeof speech, f) == NULL)
			speech[0] = '\0';
		fclose(f);
		music[strcspn(music, "\r\n")] = '\0';
		speech[strcspn(speech, "\r\n")] = '\0';
	}
	/* the game turns speech on only with all three of port, IRQ and DMA */
	snprintf(copy, sizeof copy, "%s", speech);
	for (char *t = strtok(copy, " \t"); t != NULL; t = strtok(NULL, " \t"))
		tokens++;
	snprintf(music, sizeof music, plat_midi_available() ? "r 330" : "a 388");
	if (tokens < 3)
		snprintf(speech, sizeof speech, plat_played_game() == 0 ? "220 7 1" : "220 7 1 o");

	if ((f = tmpfile()) == NULL)
		return NULL;
	fprintf(f, "%s\r\n%s\r\n", music, speech);
	rewind(f);
	return f;
}

static FILE *open_opl3_score(const char *name, const char *path)
{
	static const struct {
		const char *fm, *mt;
		int game, kind;
	} scores[] = {
		{"adlibmus.dat", "mt32mus.dat", -1, U7_OPL3_SCORE_MUSIC_BANK},
		{"introadm.dat", "intrordm.dat", 0, U7_OPL3_SCORE_BG_INTRO_BANK},
		{"a_sintro.xmi", "r_sintro.xmi", 1, U7_OPL3_SCORE_XMIDI_PAIR},
		{"a_send.xmi", "r_send.xmi", 1, U7_OPL3_SCORE_XMIDI_PAIR},
		{"endscore.xmi", NULL, 0, U7_OPL3_SCORE_BG_ENDING},
		{"mainshp.flx", NULL, 1, U7_OPL3_SCORE_SI_MENU},
		{"adlibsfx.dat", NULL, 1, U7_OPL3_SCORE_SI_EFFECTS},
	};
	const char *file = last_part(name);
	int game = plat_played_game();
	char mate[PATH_SIZE], mt_path[PATH_SIZE];

	if (!plat_opl3_enabled())
		return NULL;
	for (size_t i = 0; i < sizeof scores / sizeof *scores; i++) {
		if ((scores[i].game < 0 || scores[i].game == game) && strcasecmp(file, scores[i].fm) == 0) {
			if (scores[i].mt != NULL) {
				if (snprintf(mate, sizeof mate, "%.*s%s", (int) (file - name), name, scores[i].mt)
					>= (int) sizeof mate || !resolve(mate, mt_path) || !is_file(mt_path))
					return NULL;
			}
			return u7_opl3_score_open(game, path, scores[i].mt ? mt_path : NULL, scores[i].kind);
		}
	}
	return NULL;
}

int16_t plat_file_open(const char *name, int16_t mode)
{
	char path[PATH_SIZE];
	FILE *config, *enhanced;

	if (is_sound_config(name) && (config = open_sound_config(name)) != NULL)
		return add_handle(config);
	/* Only the saves folder opens for writing; callers then fall back to reading. */
	if (mode & PLAT_FILE_WRITE) {
		if (!resolve_in(save_root(), name, FIND, path) || !is_file(path))
			return -1;
		return add_handle(fopen(path, "r+b"));
	}
	if (!resolve(name, path) || !is_file(path))
		return -1;
	if ((enhanced = open_opl3_score(name, path)) != NULL)
		return add_handle(enhanced);
	return add_handle(fopen(path, "rb"));
}

/* Opens one of the two games' files for reading, whichever game is played. */
int16_t plat_game_file_open(int16_t game, const char *name)
{
	char path[PATH_SIZE];

	if (!resolve_in(files_game_root(game), name, FIND, path) || !is_file(path))
		return -1;
	return add_handle(fopen(path, "rb"));
}

int16_t plat_file_create(const char *name)
{
	char path[PATH_SIZE];

	if (!resolve_write(name, path) || is_dir(path))
		return -1;
	return add_handle(fopen(path, "w+b"));
}

void plat_file_close(int16_t file)
{
	FILE *f = use_handle(file, LAST_NONE);

	if (f != NULL) {
		fclose(f);
		handles[file].file = NULL;
	}
}

int32_t plat_file_read(int16_t file, void *buffer, int32_t count)
{
	FILE *f = use_handle(file, LAST_READ);
	size_t n;

	if (f == NULL || count < 0)
		return -1;
	n = fread(buffer, 1, (size_t) count, f);
	return n == 0 && ferror(f) ? -1 : (int32_t) n;
}

int32_t plat_file_write(int16_t file, const void *buffer, int32_t count)
{
	FILE *f = use_handle(file, LAST_WRITE);
	size_t n;

	if (f == NULL || count < 0)
		return -1;
	n = fwrite(buffer, 1, (size_t) count, f);
	return n == 0 && count > 0 ? -1 : (int32_t) n;
}

int32_t plat_file_seek(int16_t file, int32_t offset, int16_t origin)
{
	FILE *f = use_handle(file, LAST_NONE);
	int whence = origin == PLAT_SEEK_CUR ? SEEK_CUR : origin == PLAT_SEEK_END ? SEEK_END : SEEK_SET;

	if (f == NULL || fseek(f, offset, whence) != 0)
		return -1;
	return (int32_t) ftell(f);
}

int32_t plat_file_length(int16_t file)
{
	FILE *f = use_handle(file, LAST_NONE);
	long position, length;

	if (f == NULL)
		return -1;
	position = ftell(f);
	fseek(f, 0, SEEK_END);
	length = ftell(f);
	fseek(f, position, SEEK_SET);
	return (int32_t) length;
}

int16_t plat_file_truncate(int16_t file)
{
	FILE *f = use_handle(file, LAST_NONE);
	long position;

	if (f == NULL || fflush(f) != 0 || (position = ftell(f)) < 0)
		return 0;
#ifdef _WIN32
	return _chsize_s(_fileno(f), position) == 0;
#else
	return ftruncate(fileno(f), position) == 0;
#endif
}

/* ---- Names ---- */

int16_t plat_file_exists(const char *name)
{
	char path[PATH_SIZE];

	return resolve(name, path) && is_file(path);
}

int16_t plat_dir_exists(const char *name)
{
	char path[PATH_SIZE];

	return resolve(name, path) && is_dir(path);
}

int16_t plat_dir_create(const char *name)
{
	char path[PATH_SIZE];

	if (resolve(name, path) || !resolve_write(name, path) || stat_path(path, &(struct stat) {0}))
		return 0;
	return mkdir(path, 0777) == 0;
}

int16_t plat_dir_remove(const char *name)
{
	char path[PATH_SIZE];

	return resolve_in(save_root(), name, FIND, path) && is_dir(path) && rmdir(path) == 0;
}

int16_t plat_file_remove(const char *name)
{
	char path[PATH_SIZE];

	return resolve_in(save_root(), name, FIND, path) && is_file(path) && remove(path) == 0;
}

int16_t plat_file_rename(const char *from, const char *to)
{
	char from_path[PATH_SIZE];
	char to_path[PATH_SIZE];

	/* DOS refused to rename over an existing file. */
	if (!resolve_in(save_root(), from, FIND, from_path) || !resolve_write(to, to_path)
		|| stat_path(to_path, &(struct stat) {0}))
		return 0;
	return rename(from_path, to_path) == 0;
}

uint32_t plat_disk_free(void)
{
	const char *root = save_root();
	uint64_t bytes = 0;

	files_make_dir(root);
#ifdef _WIN32
	ULARGE_INTEGER available;

	if (GetDiskFreeSpaceExA(root, &available, NULL, NULL))
		bytes = available.QuadPart;
#else
	struct statvfs fs;

	if (statvfs(root, &fs) == 0)
		bytes = (uint64_t) fs.f_bavail * fs.f_frsize;
#endif
	/* Callers may hold this in a signed long. */
	return bytes > INT32_MAX ? INT32_MAX : (uint32_t) bytes;
}

/* ---- Listings ---- */

/* Splits a name into DOS's blank-padded 8 and 3 character fields; false if it has no
 * 8.3 form. In a pattern, '*' fills the rest of its field with '?'. */
static bool dos_fields(const char *name, bool pattern, char fields[11])
{
	const char *dot = strchr(name, '.');
	size_t base = dot ? (size_t) (dot - name) : strlen(name);
	size_t ext = dot ? strlen(dot + 1) : 0;

	if (base > 8 || ext > 3 || (dot && strchr(dot + 1, '.')))
		return false;
	if (base == 0 && !pattern)
		return false;
	memset(fields, ' ', 11);
	for (int field = 0; field < 2; field++) {
		const char *src = field == 0 ? name : dot ? dot + 1 : "";
		size_t length = field == 0 ? base : ext;
		char *dst = fields + (field == 0 ? 0 : 8);
		size_t width = field == 0 ? 8 : 3;

		for (size_t i = 0; i < length; i++) {
			if (pattern && src[i] == '*') {
				memset(dst + i, '?', width - i);
				break;
			}
			dst[i] = (char) toupper((unsigned char) src[i]);
		}
	}
	return true;
}

static bool fields_match(const char pattern[11], const char name[11])
{
	for (int i = 0; i < 11; i++) {
		if (pattern[i] != '?' && pattern[i] != name[i])
			return false;
	}
	return true;
}

static void dos_name(const char fields[11], char out[13])
{
	int n = 0;

	for (int i = 0; i < 8 && fields[i] != ' '; i++)
		out[n++] = fields[i];
	if (fields[8] != ' ') {
		out[n++] = '.';
		for (int i = 8; i < 11 && fields[i] != ' '; i++)
			out[n++] = fields[i];
	}
	out[n] = '\0';
}

/* Adds a file unless the saves folder already listed one by its name. From the game's folder
 * (dir_dos set), what the game keeps for itself is left out. */
static void add_found(find_list *list, const char *dir_dos, const char fields[11], uint64_t size)
{
	char name[13], dos[PATH_SIZE];
	found_file *f;

	dos_name(fields, name);
	snprintf(dos, sizeof dos, "%s%s", dir_dos != NULL ? dir_dos : "", name);
	if (dir_dos != NULL && saves_only(dos))
		return;
	for (uint32_t i = 0; i < list->count; i++) {
		if (strcmp(list->files[i].name, name) == 0)
			return;
	}
	list->files = realloc(list->files, (list->count + 1) * sizeof *list->files);
	f = &list->files[list->count++];
	dos_name(fields, f->name);
	f->size = size > UINT32_MAX ? UINT32_MAX : (uint32_t) size;
}

/* Adds every file in dir whose 8.3 name matches; dir_dos as add_found takes it. */
static void add_matches(find_list *list, const char *dir, const char *dir_dos, const char want[11])
{
	char have[11];

#ifdef _WIN32
	char glob[PATH_SIZE];
	WIN32_FIND_DATAA data;
	HANDLE h;

	join(glob, sizeof glob, dir, "*");
	h = FindFirstFileA(glob, &data);
	if (h != INVALID_HANDLE_VALUE) {
		do {
			if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				&& dos_fields(data.cFileName, false, have) && fields_match(want, have))
				add_found(list, dir_dos, have, (uint64_t) data.nFileSizeHigh << 32 | data.nFileSizeLow);
		} while (FindNextFileA(h, &data));
		FindClose(h);
	}
#else
	DIR *d = opendir(dir);
	struct dirent *entry;

	while (d != NULL && (entry = readdir(d)) != NULL) {
		char path[PATH_SIZE];
		struct stat st;

		if (!dos_fields(entry->d_name, false, have) || !fields_match(want, have))
			continue;
		if (join(path, sizeof path, dir, entry->d_name) && stat_path(path, &st)
			&& (st.st_mode & S_IFMT) == S_IFREG)
			add_found(list, dir_dos, have, (uint64_t) st.st_size);
	}
	if (d != NULL)
		closedir(d);
#endif
}

/* Every file in the pattern's directory, in the saves folder and the game's, whose 8.3 name
 * matches, listed up front so deleting files during the listing is safe. */
static find_list *list_matches(const char *dos_pattern)
{
	const char *wild = last_part(dos_pattern);
	char dir_dos[PATH_SIZE], saves[PATH_SIZE], game[PATH_SIZE];
	bool in_saves, in_game;
	char want[11];
	find_list *list;

	snprintf(dir_dos, sizeof dir_dos, "%.*s", (int) (wild - dos_pattern), dos_pattern);
	in_saves = resolve_in(save_root(), dir_dos, FIND, saves) && is_dir(saves);
	in_game = !saves_only(dos_pattern) && resolve_in(files_root(), dir_dos, FIND, game) && is_dir(game);
	if ((!in_saves && !in_game) || !dos_fields(wild, true, want))
		return NULL;

	list = calloc(1, sizeof *list);
	if (in_saves)
		add_matches(list, saves, NULL, want);
	if (in_game)
		add_matches(list, game, dir_dos, want);
	return list;
}

void plat_find_close(plat_find *find)
{
	find_list *list = find->state;

	if (list != NULL) {
		free(list->files);
		free(list);
		find->state = NULL;
	}
}

int16_t plat_find_next(plat_find *find)
{
	find_list *list = find->state;

	if (list == NULL)
		return 0;
	if (list->next >= list->count) {
		plat_find_close(find);
		return 0;
	}
	/* As DOS findnext: a shorter name leaves the end of the last one after its NUL. */
	strcpy(find->name, list->files[list->next].name);
	find->size = list->files[list->next].size;
	list->next++;
	return 1;
}

int16_t plat_find_first(const char *pattern, plat_find *find)
{
	find->state = list_matches(pattern);
	return plat_find_next(find);
}

/* ---- Start-up check ---- */

/* STATIC files every game needs. The game stops with its own message when one is missing, but
 * a wrong folder then reads 'File "STATIC\linkdep1." not found!', and an empty file can show as
 * an unrelated error (an empty SHAPES.VGA reads "Out of voodoo memory"). */
static const char *const required_files[] = {
	"AMMO.DAT", "ARMOR.DAT", "EQUIP.DAT", "FACES.VGA", "FONTS.VGA", "GUMPS.VGA", "INITGAME.DAT",
	"LINKDEP1", "LINKDEP2", "MONSTERS.DAT", "OCCLUDE.DAT", "PALETTES.FLX", "POINTERS.SHP",
	"READY.DAT", "SCHEDULE.DAT", "SHAPES.VGA", "SHPDIMS.DAT", "SPRITES.VGA", "TEXT.FLX", "TFA.DAT",
	"U7CHUNKS", "U7MAP", "USECODE", "WEAPONS.DAT", "WGTVOL.DAT", "WIHH.DAT", "XFORM.TBL",
};
#define FILE_LIST_SIZE 512

static const char *const black_gate_files[] = {"ENDSHAPE.FLX"};
static const char *const serpent_isle_files[] = {"MAINSHP.FLX", "PAPERDOL.VGA"};

static void append(char *message, size_t size, const char *text)
{
	size_t n = strlen(message);

	if (n < size)
		snprintf(message + n, size - n, "%s", text);
}

/* Adds each file in the list that base's STATIC lacks, or has empty, to the matching message. */
static void check_files(const char *base, const char *const *files, size_t count, char *missing,
	int *missing_count, char *empty, int *empty_count)
{
	for (size_t i = 0; i < count; i++) {
		char name[64], path[PATH_SIZE];
		struct stat st;

		snprintf(name, sizeof name, "STATIC\\%s", files[i]);
		if (!resolve_in(base, name, FIND, path) || !stat_path(path, &st) || (st.st_mode & S_IFMT) != S_IFREG) {
			append(missing, FILE_LIST_SIZE, (*missing_count)++ ? ", " : "");
			append(missing, FILE_LIST_SIZE, files[i]);
		} else if (st.st_size <= 0) {
			append(empty, FILE_LIST_SIZE, (*empty_count)++ ? ", " : "");
			append(empty, FILE_LIST_SIZE, files[i]);
		}
	}
}

/* How to point the program at a game's folder. */
static void where_to_put(int16_t game, char *out, size_t size)
{
	const char *name = game == 0 ? "Black Gate" : "Serpent Isle";
	const char *folder = game == 0 ? "Ultima7" : "Serpent";
	const char *option = game == 0 ? "--bg-data" : "--si-data";

#ifdef __APPLE__
	snprintf(out, size, "Copy your %s folder into Documents and name it %s, or point the game at it "
		"with %s <folder>.", name, folder, option);
#else
	snprintf(out, size, "Put %s in Documents/%s or next to this program in a folder named %s, or "
		"point the game at it with %s <folder>.", name, folder, folder, option);
#endif
}

bool files_check_data(int16_t game, char *message, size_t size)
{
	char missing[FILE_LIST_SIZE] = "", empty[FILE_LIST_SIZE] = "", where[256], dir[PATH_SIZE];
	const char *name = game == 0 ? "Black Gate" : "Serpent Isle";
	const char *root = files_game_root(game);
	int missing_count = 0, empty_count = 0;

	message[0] = '\0';
	where_to_put(game, where, sizeof where);
	if (!is_dir(root)) {
		snprintf(message, size, "The %s folder \"%s\" does not exist.\n\n%s", name, root, where);
		return false;
	}
	if (!resolve_in(root, "STATIC", FIND, dir) || !is_dir(dir)) {
		snprintf(message, size, "\"%s\" is not a %s folder: it has no STATIC folder.\n\n%s",
			root, name, where);
		return false;
	}
	check_files(root, required_files, sizeof required_files / sizeof *required_files, missing,
		&missing_count, empty, &empty_count);
	if (game == 0)
		check_files(root, black_gate_files, sizeof black_gate_files / sizeof *black_gate_files, missing,
			&missing_count, empty, &empty_count);
	else
		check_files(root, serpent_isle_files, sizeof serpent_isle_files / sizeof *serpent_isle_files,
			missing, &missing_count, empty, &empty_count);
	if (missing_count == 0 && empty_count == 0)
		return true;

	snprintf(message, size, "The %s installation in \"%s\" is incomplete.\n", name, root);
	if (missing_count) {
		append(message, size, "\nMissing from STATIC: ");
		append(message, size, missing);
		append(message, size, "\n");
	}
	if (empty_count) {
		append(message, size, "\nEmpty in STATIC: ");
		append(message, size, empty);
		append(message, size, "\n");
	}
	append(message, size, "\nCopy these files from a complete installation.");
	return false;
}
