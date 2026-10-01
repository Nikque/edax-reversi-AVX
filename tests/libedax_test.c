/**
 * @file libedax_test.c
 *
 * @brief Test of the Edax api (libedax).
 *
 * The program calls every function of the api and prints what it gets; a wrong structure
 * layout or a wrong answer which does not depend on the engine makes it fail (exit code 1).
 * The values which depend on the engine (scores, searched moves) are only printed.
 *
 * It is linked with the import library of the dll, and run from a directory containing
 * data/eval.dat (see build-libedax-test.cmd). It also works with the original libedax
 * (copied under the name of the dll), to compare the two libraries.
 *
 * @date 2026
 * @author Nikque
 */

#include "../src/libedax.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif

static int n_checks = 0, n_failures = 0;

#define CHECK(condition) do { \
	++n_checks; \
	if (!(condition)) { ++n_failures; printf("FAIL line %d: %s\n", __LINE__, #condition); } \
} while (0)

#define CHECK_INT(value, expected) do { \
	const long long v_ = (long long) (value), e_ = (long long) (expected); \
	++n_checks; \
	if (v_ != e_) { ++n_failures; printf("FAIL line %d: %s = %lld (expected %lld)\n", __LINE__, #value, v_, e_); } \
} while (0)

#define CHECK_STR(value, expected) do { \
	const char *v_ = (value), *e_ = (expected); \
	++n_checks; \
	if (v_ == NULL || strcmp(v_, e_) != 0) { ++n_failures; printf("FAIL line %d: %s = \"%s\" (expected \"%s\")\n", __LINE__, #value, v_ ? v_ : "(null)", e_); } \
} while (0)

static void section(const char *name)
{
	fflush(stdout);
	printf("\n## %s\n", name);
	fflush(stdout);
}

/** square name */
static const char* square(const int x)
{
	static char s[8][4];
	static int i = 0;
	char *p = s[i = (i + 1) & 7];
	if (x == LIBEDAX_PASS) strcpy(p, "pa");
	else if (x == LIBEDAX_NOMOVE) strcpy(p, "--");
	else if (0 <= x && x < 64) { p[0] = (char) ('a' + x % 8); p[1] = (char) ('1' + x / 8); p[2] = '\0'; }
	else strcpy(p, "??");
	return p;
}

static void play(const char *moves)
{
	char buffer[256];
	strcpy(buffer, moves);
	edax_play(buffer);
}

static const char* moves(void)
{
	static char buffer[200];
	return edax_get_moves(buffer);
}

static LibedaxBoard board(void)
{
	LibedaxBoard b = {0, 0};
	edax_get_board(&b);
	return b;
}

static void print_position(const char *name, const LibedaxPosition *p)
{
	int i;
	printf("%s: board %016llx %016llx level %d score %d [%d, %d] leaf %s:%d links",
		name, p->board[0].player, p->board[0].opponent, p->level, p->score.value, p->score.lower, p->score.upper,
		square(p->leaf.move), p->leaf.score);
	for (i = 0; i < p->n_link; ++i) printf(" %s:%d", square(p->link[i].move), p->link[i].score);
	printf(" | lines %u wins %u draws %u losses %u | bestpaths %d %d flag %d\n",
		p->n_lines, p->n_wins, p->n_draws, p->n_losses, p->n_player_bestpaths, p->n_opponent_bestpaths, p->flag);
}

static void print_movelist(const char *name, const LibedaxMoveList *l)
{
	const LibedaxMove *m;
	int i, n = 0;
	printf("%s: %d moves, array", name, l->n_moves);
	for (i = 1; i <= l->n_moves; ++i) printf(" %s:%d", square(l->move[i].x), l->move[i].score);
	printf(", list");
	for (m = l->move[0].next; m && n <= l->n_moves; m = m->next, ++n) printf(" %s", square(m->x));
	printf("\n");
	CHECK_INT(n, l->n_moves);
}

static void print_hint(const char *name, const LibedaxHint *h)
{
	int i;
	printf("%s: %s score %d [%d, %d] depth %d@%d book %d pv", name, square(h->move), h->score, h->lower, h->upper, h->depth, h->selectivity, h->book_move);
	if (h->move != LIBEDAX_NOMOVE) for (i = 0; i < h->pv[0].n_moves && i < 8; ++i) printf(" %s", square(h->pv[0].move[i]));
	printf("\n");
}

static long long file_size(const char *file)
{
	FILE *f = fopen(file, "rb");
	long long size = -1;
	if (f) {
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fclose(f);
	}
	return size;
}

static void write_file(const char *file, const char *text)
{
	FILE *f = fopen(file, "w");
	if (f) {
		fputs(text, f);
		fclose(f);
	}
}

/** functions added to the original api: NULL with the original libedax */
typedef void (*Deviate)(int, int);
static Deviate optional(const char *dll, const char *name)
{
#ifdef _WIN32
	HMODULE h = GetModuleHandleA(dll);
	return h ? (Deviate) (void*) GetProcAddress(h, name) : NULL;
#else
	(void) dll;
	return (Deviate) dlsym(RTLD_DEFAULT, name);
#endif
}

int main(int argc, char **argv)
{
	const char *dll = (argc > 1 ? argv[1] : "libedax-x64.dll");
	char *args[] = {"", "-book-file", "libtest-book.dat", "-level", "6", "-n-tasks", "1"};
	LibedaxBoard b, start;
	LibedaxMove move;
	LibedaxMoveList *movelist = (LibedaxMoveList*) calloc(1, sizeof *movelist);
	LibedaxHintList *hintlist = (LibedaxHintList*) calloc(1, sizeof *hintlist);
	LibedaxHint hint;
	LibedaxPosition position;
	LibedaxBook info;
	union { LibedaxBenchResult result; char room[256]; } bench; // the original libedax keeps a lock after the result
	Deviate deviate2, deviate3;
	bool original; // the original libedax: skip what it does not support
	char line[128];
	int i, n, sym;

	setvbuf(stdout, NULL, _IONBF, 0);

	// speed test: libedax_test <dll> bench <n problems> <n tasks> (as "edax -bench <n> -n <n tasks>")
	if (argc > 4 && strcmp(argv[2], "bench") == 0) {
		char *bench_args[] = {"", "-n-tasks", argv[4], "-book-file", "libtest-bench-book.dat"};
		libedax_initialize(5, bench_args);
		memset(&bench, 0, sizeof bench);
		edax_bench(&bench.result, atoi(argv[3]));
		printf("bench: %d positions, %llu nodes, %llu ms\n", bench.result.positions, bench.result.n_nodes, bench.result.T);
		return 0;
	}

	deviate2 = optional(dll, "edax_book_deviate2");
	deviate3 = optional(dll, "edax_book_deviate3");
	original = !(deviate2 && deviate3);
	printf("%s: %s\n", dll, original ? "original libedax" : "libedax of Edax 4.5.5");

	section("layout of the structures");
	CHECK_INT(sizeof (LibedaxBoard), 16);
	CHECK_INT(sizeof (LibedaxMove), 32);
	CHECK_INT(offsetof(LibedaxMove, next), 24);
	CHECK_INT(sizeof (LibedaxMoveList), 34 * 32 + 8);
	CHECK_INT(offsetof(LibedaxMoveList, n_moves), 34 * 32);
	CHECK_INT(sizeof (LibedaxLine), 88);
	CHECK_INT(sizeof (LibedaxHint), 136);
	CHECK_INT(offsetof(LibedaxHint, pv), 24);
	CHECK_INT(offsetof(LibedaxHint, time), 112);
	CHECK_INT(offsetof(LibedaxHint, book_move), 128);
	CHECK_INT(sizeof (LibedaxHintList), 34 * 136 + 8);
	CHECK_INT(sizeof (LibedaxLink), 2);
	CHECK_INT(sizeof (LibedaxPosition), 56);
	CHECK_INT(offsetof(LibedaxPosition, flag), 18);
	CHECK_INT(offsetof(LibedaxPosition, n_player_bestpaths), 20);
	CHECK_INT(offsetof(LibedaxPosition, link), 24);
	CHECK_INT(offsetof(LibedaxPosition, n_wins), 32);
	CHECK_INT(offsetof(LibedaxPosition, score), 48);
	CHECK_INT(offsetof(LibedaxPosition, n_link), 54);
	CHECK_INT(sizeof (LibedaxBook), 96);
	CHECK_INT(offsetof(LibedaxBook, options), 8);
	CHECK_INT(offsetof(LibedaxBook, stats), 28);
	CHECK_INT(offsetof(LibedaxBook, n), 56);
	CHECK_INT(offsetof(LibedaxBook, n_nodes), 60);

	section("bit & board utilities");
	CHECK_INT(bit_count(0x0000001818000000ULL), 4);
	CHECK_INT(bit_count(0xffffffffffffffffULL), 64);
	CHECK_INT(first_bit(0x0000001818000000ULL), 27);
	CHECK_INT(last_bit(0x0000001818000000ULL), 36);
	CHECK(get_moves(0x0000000810000000ULL, 0x0000001008000000ULL) == 0x0000102004080000ULL);
	CHECK(can_move(0x0000000810000000ULL, 0x0000001008000000ULL));
	CHECK(!can_move(0x1ULL, 0x8000000000000000ULL));

	section("before the initialization");
	edax_init();
	CHECK_INT(edax_is_game_over(), 0);
	CHECK_INT(edax_get_current_player(), -1);
	CHECK(edax_opening() == NULL);

	section("initialization");
	remove("libtest-book.dat");
	libedax_initialize(7, args);
	if (!original) libedax_initialize(7, args); // ignored
	edax_version();
	edax_init();
	start = board();
	CHECK(start.player == 0x0000000810000000ULL && start.opponent == 0x0000001008000000ULL);
	CHECK_INT(edax_get_current_player(), 0);
	CHECK_STR(moves(), "");
	if (!original) {
		edax_get_last_move(&move);
		CHECK_INT(move.x, LIBEDAX_NOMOVE);
	}

	section("game");
	edax_mode(3);
	play("F5d6C5f4d3");
	CHECK_STR(moves(), "F5d6C5f4D3");
	CHECK_INT(edax_get_current_player(), 1);
	CHECK_INT(edax_get_disc(0), 6);
	CHECK_INT(edax_get_disc(1), 3);
	CHECK_INT(edax_get_mobility_count(1), 8);
	CHECK_INT(edax_get_mobility_count(0), 6);
	CHECK_INT(edax_can_move(), 1);
	CHECK_INT(edax_is_game_over(), 0);
	edax_get_last_move(&move);
	CHECK_INT(move.x, 19);
	CHECK(move.flipped == 0x0000000018000000ULL); // d4 & e4
	b = board();
	CHECK_INT(edax_board_is_pass(&b), 0);
	CHECK_INT(edax_board_get_square_color(&b, 19), 1); // d3: the opponent of the player to move
	CHECK_INT(edax_board_get_square_color(&b, 0), 2);
	printf("opening: %s / %s\n", edax_opening(), edax_ouverture());
	edax_play_print();

	edax_undo();
	CHECK_STR(moves(), "F5d6C5f4");
	edax_undo();
	edax_redo();
	edax_redo();
	CHECK_STR(moves(), "F5d6C5f4D3");
	edax_redo();
	CHECK_STR(moves(), "F5d6C5f4D3");

	CHECK_INT(edax_move("c3"), 1);
	CHECK_INT(edax_move("a1"), 0);
	CHECK_STR(moves(), "F5d6C5f4D3c3");

	edax_vmirror(); b = board(); printf("vmirror: %016llx %016llx\n", b.player, b.opponent);
	edax_vmirror();
	edax_hmirror(); b = board(); printf("hmirror: %016llx %016llx\n", b.player, b.opponent);
	edax_hmirror();
	edax_rotate(90); b = board(); printf("rotate 90: %016llx %016llx\n", b.player, b.opponent);
	edax_rotate(270);
	edax_rotate(180); edax_rotate(-180);
	edax_symetry(3); edax_symetry(3);
	CHECK_STR(moves(), "F5d6C5f4D3c3");

	edax_setboard("-W----W--------------------WB------WBB-----W--------------------B");
	b = board();
	CHECK_INT(edax_get_disc(0), 3);
	CHECK_INT(edax_get_disc(1), 5);
	CHECK_INT(edax_get_current_player(), 0);
	b.player = 0x0000000810000000ULL; b.opponent = 0x0000001008000000ULL;
	edax_setboard_from_obj(&b, 1);
	CHECK_INT(edax_get_current_player(), 1);
	b = board();
	CHECK(b.player == 0x0000001008000000ULL && b.opponent == 0x0000000810000000ULL);
	CHECK_INT(edax_move("e6"), 0);
	CHECK_INT(edax_move("d6"), 1);
	edax_new();
	CHECK_STR(moves(), "");
	CHECK_INT(edax_get_current_player(), 1);
	edax_save("libtest-game.txt");
	edax_init();
	edax_load("libtest-game.txt");
	edax_init();
	{
		char forced[] = "F5F6";
		edax_force(forced);
	}
	edax_go();
	CHECK_STR(moves(), "F5");
	edax_go();
	CHECK_STR(moves(), "F5f6");

	section("options");
	edax_set_option("level", "4");
	edax_set_option("book-randomness", "0");
	edax_set_option("unknown-option", "1");
	edax_book_randomness(0);
	edax_book_off();
	edax_book_on();
	edax_options_dump();

	section("hints without book");
	edax_init();
	play("f5");
	edax_hint(2, hintlist);
	CHECK_INT(hintlist->n_hints, 2);
	for (i = 1; i <= hintlist->n_hints; ++i) print_hint("hint", hintlist->hint + i);
	CHECK(hintlist->hint[1].move != hintlist->hint[2].move);
	edax_hint(60, hintlist);
	CHECK_INT(hintlist->n_hints, 3);
	edax_hint_prepare(NULL);
	for (n = 0; n < 10; ++n) {
		if (n & 1) edax_hint_next(&hint); else edax_hint_next_no_multipv_depth(&hint);
		print_hint("hint next", &hint);
		if (hint.move == LIBEDAX_NOMOVE) break;
	}
	CHECK_INT(n, 3);
	// exclude d6 (43) & f6 (45): only f4 is left
	memset(movelist, 0, sizeof *movelist);
	movelist->move[1].x = 43; movelist->move[2].x = 45;
	movelist->move[0].next = movelist->move + 1; movelist->move[1].next = movelist->move + 2;
	movelist->n_moves = 2;
	edax_hint_prepare(movelist);
	edax_hint_next(&hint);
	CHECK_INT(hint.move, 29);
	edax_hint_next(&hint);
	CHECK_INT(hint.move, LIBEDAX_NOMOVE);

	section("a game played by edax, stored in a new book");
	edax_book_new(4, 12);
	edax_enable_book_verbose();
	edax_book_verbose(1);
	edax_book_verbose(0);
	edax_init();
	for (n = 0; !edax_is_game_over() && n < 80; ++n) edax_go();
	CHECK_INT(edax_is_game_over(), 1);
	printf("game: %s\n", moves());
	edax_book_store();
	edax_book_info(&info);
	printf("book: level %d depth %d positions %d\n", info.options.level, 61 - info.options.n_empties, info.n_nodes);
	CHECK_INT(info.options.level, 4);
	CHECK_INT(info.options.n_empties, 49);
	CHECK(info.n_nodes >= 13);
	CHECK(info.date.year >= 2026 && info.date.month >= 1 && info.date.month <= 12);
	edax_book_depth(10);
	edax_book_info(&info);
	CHECK_INT(info.options.n_empties, 51);

	section("book positions");
	edax_init();
	memset(&position, 0, sizeof position);
	edax_book_show(&position);
	print_position("root", &position);
	CHECK(position.board[0].player == 0x0000000810000000ULL);
	CHECK_INT(position.level, 4);
	CHECK(position.n_link >= 1);
	CHECK(position.n_lines >= 1);
	memset(movelist, 0, sizeof *movelist);
	edax_get_bookmove(movelist);
	print_movelist("book moves", movelist);
	CHECK(movelist->n_moves >= 1);
	memset(movelist, 0, sizeof *movelist);
	memset(&position, 0, sizeof position);
	sym = edax_get_bookmove_with_position(movelist, &position);
	printf("symetry %d\n", sym);
	CHECK(sym >= 0 && sym < 8);
	print_movelist("book moves", movelist);
	print_position("root", &position);
	memset(movelist, 0, sizeof *movelist);
	memset(&position, 0, sizeof position);
	strcpy(line, "F5"); // the original libedax changes the string
	sym = edax_get_bookmove_with_position_by_moves(line, movelist, &position);
	printf("by moves F5: symetry %d\n", sym);
	print_movelist("book moves", movelist);
	print_position("f5", &position);
	CHECK(sym >= 0 && sym < 8);
	strcpy(line, "F5F6E6F4G5E7E3F3C5C4G3C6D6D7");
	CHECK_INT(edax_get_bookmove_with_position_by_moves(line, movelist, &position), -1); // deeper than the book
	edax_book_off();
	CHECK_INT(edax_get_bookmove_with_position(movelist, &position), -1);
	CHECK_INT(movelist->n_moves, 0);
	edax_book_on();

	section("hints with book");
	edax_init();
	edax_hint(4, hintlist);
	CHECK_INT(hintlist->n_hints, 4);
	for (i = 1; i <= hintlist->n_hints; ++i) print_hint("hint", hintlist->hint + i);
	CHECK_INT(hintlist->hint[1].book_move, 1);
	edax_hint_prepare(NULL);
	for (n = 0; n < 10; ++n) {
		edax_hint_next(&hint);
		print_hint("hint next", &hint);
		if (hint.move == LIBEDAX_NOMOVE) break;
	}
	CHECK_INT(n, 4);

	section("best paths");
	b = start;
	memset(&position, 0, sizeof position);
	edax_book_count_bestpath(&b, &position);
	print_position("count bestpath", &position);
	CHECK(position.n_player_bestpaths >= 1 && position.n_opponent_bestpaths >= 1);
	CHECK(b.player == start.player && b.opponent == start.opponent);
	edax_book_stats_clean(); // the original libedax needs it before counting another way
	memset(&position, 0, sizeof position);
	edax_book_count_board_bestpath(&b, &position, LIBEDAX_BESTPATH_BEST, LIBEDAX_BESTPATH_BEST, 0);
	print_position("count board bestpath (best, best)", &position);
	CHECK(position.n_player_bestpaths >= 1 && position.n_opponent_bestpaths >= 1);
	CHECK(position.flag & LIBEDAX_FLAG_BESTPATH_BLACK);
	edax_book_stats_clean();
	memset(&position, 0, sizeof position);
	edax_book_count_board_bestpath(&b, &position, -64, LIBEDAX_BESTPATH_BEST, 0);
	print_position("count board bestpath (-64, best)", &position);
	edax_book_stats_clean();
	memset(&position, 0, sizeof position);
	edax_book_count_board_bestpath(&b, &position, LIBEDAX_BESTPATH_BEST, -64, 0);
	print_position("count board bestpath (best, -64)", &position);
	edax_book_stop_count_bestpath();
	edax_book_stats_clean();
	memset(&position, 0, sizeof position);
	edax_book_show(&position);
	CHECK_INT(position.n_player_bestpaths, 0);
	b.player = 1; b.opponent = 2; // not in the book
	memset(&position, 0, sizeof position);
	edax_book_count_bestpath(&b, &position);
	CHECK_INT(position.n_player_bestpaths, 1);
	CHECK_INT(position.n_opponent_bestpaths, 1);

	section("book learning");
	edax_init();
	play("f5d6");
	edax_book_deviate(1, 1);
	edax_book_info(&info);
	printf("after deviate 1 1: %d positions\n", info.n_nodes);
	n = info.n_nodes;
	if (!original) {
		deviate2(1, 1);
		deviate3(1, 1);
		edax_book_info(&info);
		printf("after deviate2 & deviate3 1 1: %d positions\n", info.n_nodes);
		CHECK(info.n_nodes >= n);
	}
	edax_book_enhance(4, 4);
	edax_book_fill(1);
	edax_book_fix();
	edax_book_negamax();
	edax_book_correct();
	edax_book_feed_hash();
	edax_book_info(&info);
	printf("after enhance, fill, fix, negamax, correct: %d positions\n", info.n_nodes);
	n = info.n_nodes;

	edax_book_add_board_pre_process();
	b.player = 0x0000000810000000ULL; b.opponent = 0x0000001008000000ULL;
	edax_book_add_board(&b); // already in the book
	edax_book_add_board_post_process();
	edax_book_info(&info);
	CHECK_INT(info.n_nodes, n);

	section("book files");
	edax_book_save("libtest-book2.dat");
	CHECK(file_size("libtest-book2.dat") > 100);
	edax_book_export("libtest-book.txt");
	CHECK(file_size("libtest-book.txt") > 100);
	edax_book_new(4, 12);
	edax_book_info(&info);
	CHECK_INT(info.n_nodes, 1);
	edax_book_load("libtest-book2.dat");
	edax_book_info(&info);
	CHECK_INT(info.n_nodes, n);
	edax_book_load("libtest-no-such-book.dat");
	edax_book_import("libtest-book.txt");
	edax_book_info(&info);
	CHECK_INT(info.n_nodes, n);
	edax_book_new(4, 12);
	edax_book_merge("libtest-book2.dat");
	edax_book_info(&info);
	CHECK_INT(info.n_nodes, n);
	edax_init();
	play("f5d6");
	edax_book_subtree();
	edax_book_info(&info);
	printf("after subtree: %d positions\n", info.n_nodes);
	CHECK(info.n_nodes <= n);
	edax_book_load("libtest-book2.dat");
	edax_book_prune();
	edax_book_info(&info);
	printf("after prune: %d positions\n", info.n_nodes);
	CHECK(info.n_nodes <= n && info.n_nodes >= 1);
	edax_book_new(2, 4);
	edax_book_play();
	edax_book_info(&info);
	printf("after book new 2 4 & play: %d positions\n", info.n_nodes);
	CHECK(info.n_nodes > 1);
	edax_book_deepen();

	section("game database");
	write_file("libtest-base.txt", "F5D6C3D3C4F4F6F3E6E7\nF5F6E6F4E3C5C4E7C6E2\nF5D6C3D3C4F4F6F3E6E7\n");
	edax_book_new(2, 8);
	edax_book_add("libtest-base.txt");
	edax_book_info(&info);
	printf("after book add: %d positions\n", info.n_nodes);
	CHECK(info.n_nodes >= 12);
	edax_book_check("libtest-base.txt");
	edax_book_extract("libtest-extract.txt");
	CHECK(file_size("libtest-extract.txt") > 0);
	edax_base_convert("libtest-base.txt", "libtest-base.ggf");
	CHECK(file_size("libtest-base.ggf") > 0);
	edax_base_unique("libtest-base.txt", "libtest-unique.txt");
	CHECK(file_size("libtest-unique.txt") > 0 && file_size("libtest-unique.txt") < file_size("libtest-base.txt"));
	edax_base_problem("libtest-base.txt", 52, "libtest-problem.obf");
	CHECK(file_size("libtest-problem.obf") > 0);
	edax_base_tofen("libtest-base.txt", 52, "libtest-problem.fen");
	CHECK(file_size("libtest-problem.fen") > 0);
	write_file("libtest-short.txt", "F5F6E6F4G5E7E3F3C5C4G3C6D6D7C3C2B5D3B4E2F2H3F1D1D2G4G6C1E1B6C7D8F7H4F8G7\n");
	edax_base_complete("libtest-short.txt");
	edax_base_correct("libtest-short.txt", 4);
	CHECK(file_size("libtest-short.txt") > 0);

	section("bench");
	memset(&bench, 0, sizeof bench);
	edax_bench_get_result(&bench.result);
	CHECK_INT(bench.result.positions, 0);
	edax_bench(&bench.result, 2);
	printf("bench: %d positions, %llu nodes\n", bench.result.positions, bench.result.n_nodes);
	CHECK_INT(bench.result.positions, 2);
	CHECK(bench.result.n_nodes > 0);

	section("stop & termination");
	edax_stop();
	edax_disable_book_verbose();
	libedax_terminate();
	if (!original) libedax_terminate(); // ignored
	CHECK_INT(edax_is_game_over(), 0);
	CHECK(file_size("libtest-book.dat") > 0); // the new book is saved

	section("second initialization");
	libedax_initialize(3, args);
	edax_init();
	play("f5");
	CHECK_STR(moves(), "F5");
	edax_book_info(&info);
	printf("book: level %d depth %d positions %d\n", info.options.level, 61 - info.options.n_empties, info.n_nodes);
	CHECK(info.n_nodes >= 12);
	libedax_terminate();

	free(movelist);
	free(hintlist);
	printf("\n%d checks, %d failures\n", n_checks, n_failures);
	return n_failures ? 1 : 0;
}
