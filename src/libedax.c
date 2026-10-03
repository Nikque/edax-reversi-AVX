/**
 * @file libedax.c
 *
 * @brief Edax api (libedax): Edax as a library.
 *
 * This file is only compiled when the library is built (LIB_BUILD): it is included
 * by all.c after the other sources, and does not change the edax program.
 *
 * The functions do the same as the commands of the Edax protocol (see edax.c). The data
 * returned to the caller are copied from the structures of Edax to the structures of
 * the api (see libedax.h), which keep the layout of the original libedax.
 *
 * @date 2018 - 2026
 * @author lavox (api), sensuikan1973, Nikque (port to Edax 4.5.5)
 */

#include "libedax.h"

#include "cassio.h"
#include "event.h"
#include "histogram.h"
#include "options.h"
#include "opening.h"
#include "obftest.h"
#include "perft.h"
#include "play.h"
#include "search.h"
#include "util.h"
#include "ui.h"

#include <limits.h>

extern bool book_verbose;

/* the structures of the api which are copied as a whole must have the layout of the ones of Edax */
typedef char libedax_board_check[(sizeof (LibedaxBoard) == sizeof (Board)) ? 1 : -1];
typedef char libedax_link_check[(sizeof (LibedaxLink) == sizeof (Link)) ? 1 : -1];
typedef char libedax_line_check[(LIBEDAX_LINE_SIZE == GAME_SIZE) ? 1 : -1];
typedef char libedax_move_check[(LIBEDAX_PASS == PASS && LIBEDAX_NOMOVE == NOMOVE) ? 1 : -1];

/** the user interface (NULL when the library is not initialized) */
static UI *g_ui = NULL;

/** the options before any setting is read (restored by each initialization) */
static Options lib_default_options;
static bool lib_is_started = false;

/** result of the running bench */
static struct {
	Lock lock;
	LibedaxBenchResult *result;
} lib_bench;

/**
 * A command which changes the book is running. edax_stop() does not stop it: a stopped search
 * would give its unfinished result to the book (the book commands do not look at the stop).
 */
static struct {
	Lock lock;
	bool running;
} lib_book_change;

/**
 * @brief Tell if a command which changes the book is running.
 * @param running New state.
 * @return previous state.
 */
static bool lib_book_change_set(const bool running)
{
	bool previous;

	lock(&lib_book_change); // edax_stop() checks the state and stops under this lock
	previous = lib_book_change.running;
	lib_book_change.running = running;
	unlock(&lib_book_change);
	return previous;
}

/*
 * Conversion to the structures of the api
 */

/**
 * @brief Copy a move.
 * @param dst Move of the api.
 * @param src Move.
 */
static void lib_move_set(LibedaxMove *dst, const Move *src)
{
	dst->flipped = src->flipped;
	dst->x = src->x;
	dst->score = src->score;
	dst->cost = src->cost;
	dst->next = NULL;
}

/**
 * @brief Set an empty list of moves.
 * @param dst List of moves of the api.
 */
static void lib_movelist_set_empty(LibedaxMoveList *dst)
{
	dst->move[0].next = NULL;
	dst->n_moves = 0;
}

/**
 * @brief Copy a list of moves.
 *
 * The moves keep their place in the array, and their order in the list.
 *
 * @param dst List of moves of the api.
 * @param src List of moves.
 */
static void lib_movelist_set(LibedaxMoveList *dst, const MoveList *src)
{
	const Move *m;
	LibedaxMove *previous = dst->move;
	int i, n = src->n_moves;

	if (n > LIBEDAX_MOVELIST_SIZE - 1) n = LIBEDAX_MOVELIST_SIZE - 1;
	for (i = 1; i <= n; ++i) lib_move_set(dst->move + i, src->move + i);
	for (m = src->move[0].next; m; m = m->next) {
		i = (int) (m - src->move);
		if (1 <= i && i <= n) previous = previous->next = dst->move + i;
	}
	previous->next = NULL;
	dst->n_moves = n;
}

/**
 * @brief Copy a sequence of moves.
 * @param dst Line of the api.
 * @param src Line.
 */
static void lib_line_set(LibedaxLine *dst, const Line *src)
{
	memcpy(dst->move, src->move, LIBEDAX_LINE_SIZE);
	dst->n_moves = src->n_moves;
	dst->color = src->color;
}

/**
 * @brief Set a hint from a book move.
 * @param hint Hint of the api.
 * @param move Book move.
 * @param pv Book line.
 */
static void lib_hint_set_book(LibedaxHint *hint, const Move *move, const Line *pv)
{
	hint->depth = 0;
	hint->selectivity = 0;
	hint->move = move->x;
	hint->score = move->score;
	hint->upper = 0;
	hint->lower = 0;
	lib_line_set(hint->pv, pv);
	hint->time = 0;
	hint->n_nodes = 0;
	hint->book_move = true;
}

/**
 * @brief Set a hint from a search result.
 * @param hint Hint of the api.
 * @param result Search result.
 */
static void lib_hint_set_result(LibedaxHint *hint, const Result *result)
{
	hint->depth = result->depth;
	hint->selectivity = result->selectivity;
	hint->move = result->move;
	hint->score = result->score;
	hint->upper = result->bound[result->move].upper;
	hint->lower = result->bound[result->move].lower;
	lib_line_set(hint->pv, &result->pv);
	hint->time = result->time;
	hint->n_nodes = result->n_nodes;
	hint->book_move = false;
}

/**
 * @brief Set an empty hint.
 * @param hint Hint of the api.
 */
static void lib_hint_set_nomove(LibedaxHint *hint)
{
	hint->depth = 0;
	hint->selectivity = 0;
	hint->move = NOMOVE;
	hint->score = 0;
	hint->upper = 0;
	hint->lower = 0;
	hint->time = 0;
	hint->n_nodes = 0;
	hint->book_move = false;
}

/*
 * Count of the best paths of the book.
 *
 * The original libedax kept two counts and a flag in each position of the book. Here they are
 * kept beside the book, in arrays allocated when the paths are counted: the positions of the
 * book stay as small as in the edax program.
 */

/** counts of a position: [0] for player, [1] for opponent; 0 = not counted yet */
typedef unsigned short LibBestpathCount[2];

static struct {
	LibBestpathCount *count;      /**< counts of each position */
	unsigned char *black;         /**< turn of each position when it was counted (edax_book_count_board_bestpath) */
	unsigned int *first;          /**< index of the first position of each bucket */
	const PositionArray *array;   /**< book for which the arrays were allocated... */
	int n;                        /**< ...its number of buckets... */
	unsigned int n_nodes;         /**< ...and of positions */
	int mode, p_lower, o_lower;   /**< what was counted */
	volatile Stop stop;           /**< stop the count */
} lib_bestpath;

/**
 * @brief Forget the counts.
 *
 * Called whenever the book may change.
 */
static void lib_bestpath_free(void)
{
	free(lib_bestpath.count); lib_bestpath.count = NULL;
	free(lib_bestpath.black); lib_bestpath.black = NULL;
	free(lib_bestpath.first); lib_bestpath.first = NULL;
}

/**
 * @brief Check if the counts are the ones of this book.
 * @param book Opening book.
 * @return true if they are.
 */
static bool lib_bestpath_is_valid(const Book *book)
{
	return lib_bestpath.count != NULL && lib_bestpath.array == book->array
		&& lib_bestpath.n == book->n && lib_bestpath.n_nodes == book->n_nodes;
}

/**
 * @brief Prepare the counts.
 *
 * The counts already done are kept if they were done the same way.
 *
 * @param book Opening book.
 * @param mode 0: edax_book_count_bestpath, 1: edax_book_count_board_bestpath.
 * @param p_lower lower limit for player.
 * @param o_lower lower limit for opponent.
 * @return false if memory is exhausted.
 */
static bool lib_bestpath_prepare(const Book *book, const int mode, const int p_lower, const int o_lower)
{
	unsigned int i, n = 0;

	if (lib_bestpath_is_valid(book) && lib_bestpath.mode == mode
	 && lib_bestpath.p_lower == p_lower && lib_bestpath.o_lower == o_lower) return true;

	lib_bestpath_free();
	lib_bestpath.first = (unsigned int*) malloc(book->n * sizeof *lib_bestpath.first);
	if (lib_bestpath.first) {
		for (i = 0; i < (unsigned int) book->n; ++i) {
			lib_bestpath.first[i] = n;
			n += (unsigned int) book->array[i].n;
		}
		lib_bestpath.count = (LibBestpathCount*) calloc(n ? n : 1, sizeof *lib_bestpath.count);
		lib_bestpath.black = (unsigned char*) calloc(n ? n : 1, 1);
	}
	if (lib_bestpath.count == NULL || lib_bestpath.black == NULL) {
		lib_bestpath_free();
		warn("cannot allocate the counts of the best paths\n");
		return false;
	}
	lib_bestpath.array = book->array;
	lib_bestpath.n = book->n;
	lib_bestpath.n_nodes = book->n_nodes;
	lib_bestpath.mode = mode;
	lib_bestpath.p_lower = p_lower;
	lib_bestpath.o_lower = o_lower;
	return true;
}

/** @return the index of the counts of a position. */
static inline size_t lib_bestpath_index(const Book *book, const Position *p)
{
	const unsigned long long i = board_get_hash_code(&p->board) & (book->n - 1);
	return (size_t) lib_bestpath.first[i] + (size_t) (p - book->array[i].positions);
}

/**
 * @brief Add the count of a move to the counts of a position.
 * @param n_player count for player.
 * @param n_opponent count for opponent.
 * @param n_next_player count for player after the move.
 * @param n_next_opponent count for opponent after the move.
 */
static inline void lib_bestpath_add(unsigned short *n_player, unsigned short *n_opponent, const unsigned short n_next_player, const unsigned short n_next_opponent)
{
	*n_player = MIN(*n_player, n_next_opponent);
	if (n_next_player <= USHRT_MAX - *n_opponent) *n_opponent += n_next_player;
	else *n_opponent = USHRT_MAX;
}

/**
 * @brief count the number of best paths in book.
 *
 * @param book Opening book.
 * @param board Starting position.
 * @param n_player the number of best paths for player (out parameter).
 * @param n_opponent the number of best paths for opponent (out parameter).
 */
static void lib_count_bestpath(Book *book, Board *board, unsigned short *n_player, unsigned short *n_opponent)
{
	MoveList movelist;
	Move *move;
	unsigned short n_next_player, n_next_opponent;
	Position *position = book_probe(book, board);

	if (position) {
		unsigned short *count = lib_bestpath.count[lib_bestpath_index(book, position)];

		if (count[0] != 0 && count[1] != 0) {
			*n_player = count[0];
			*n_opponent = count[1];
		} else {
			const int bestscore = position->score.value;

			position_get_moves(position, board, &movelist);
			*n_player = USHRT_MAX;
			*n_opponent = 0;
			foreach_move(move, movelist) {
				if (move->score != bestscore) continue;
				if (lib_bestpath.stop) break;

				board_update(board, move);
				lib_count_bestpath(book, board, &n_next_player, &n_next_opponent);
				lib_bestpath_add(n_player, n_opponent, n_next_player, n_next_opponent);
				board_restore(board, move);
			}
			if (lib_bestpath.stop) return;
			if (*n_opponent == 0) *n_player = *n_opponent = 1;
			count[0] = *n_player;
			count[1] = *n_opponent;
		}
	} else {
		*n_player = *n_opponent = 1;
	}
}

/**
 * @brief count the number of "broad" best paths in book.
 *
 * @param book Opening book.
 * @param board Starting position.
 * @param p_lower lower limit for player (BESTPATH_BEST: best moves only).
 * @param o_lower lower limit for opponent (BESTPATH_BEST: best moves only).
 * @param turn turn of the position.
 * @param n_player the number of best paths for player (out parameter).
 * @param n_opponent the number of best paths for opponent (out parameter).
 */
static void lib_count_board_bestpath(Book *book, Board *board, const int p_lower, const int o_lower, const int turn, unsigned short *n_player, unsigned short *n_opponent)
{
	MoveList movelist;
	Move *move;
	unsigned short n_next_player, n_next_opponent;
	Position *position = book_probe(book, board);

	if (position) {
		const size_t i = lib_bestpath_index(book, position);
		unsigned short *count = lib_bestpath.count[i];

		if (count[0] != 0 && count[1] != 0 && lib_bestpath.black[i] == (turn == BLACK)) {
			*n_player = count[0];
			*n_opponent = count[1];
		} else {
			const int lower = (p_lower == LIBEDAX_BESTPATH_BEST ? position->score.value : p_lower);

			position_get_moves(position, board, &movelist);
			*n_player = USHRT_MAX;
			*n_opponent = 0;
			foreach_move(move, movelist) {
				if (move->score < lower) continue;
				if (lib_bestpath.stop) break;

				board_update(board, move);
				lib_count_board_bestpath(book, board, o_lower, p_lower, 1 - turn, &n_next_player, &n_next_opponent);
				lib_bestpath_add(n_player, n_opponent, n_next_player, n_next_opponent);
				board_restore(board, move);
			}
			if (lib_bestpath.stop) return;
			if (*n_opponent == 0) *n_player = *n_opponent = 1;
			count[0] = *n_player;
			count[1] = *n_opponent;
			lib_bestpath.black[i] = (turn == BLACK);
		}
	} else {
		*n_player = *n_opponent = 1;
	}
}

/** links of the last positions given to the caller: enough for a position and the positions after each of its moves */
#define LIB_N_LINKS 64
static LibedaxLink lib_links[LIB_N_LINKS][MAX_MOVE + 2];
static int lib_i_links = 0;

/**
 * @brief Copy a position of the book.
 * @param dst Position of the api.
 * @param p Position.
 * @param book Opening book.
 */
static void lib_position_set(LibedaxPosition *dst, const Position *p, const Book *book)
{
	const Link *l = position_links(p);
	LibedaxLink *link = lib_links[lib_i_links = (lib_i_links + 1) % LIB_N_LINKS];
	int i;

	dst->board[0].player = p->board.player;
	dst->board[0].opponent = p->board.opponent;
	dst->leaf.score = p->leaf.score;
	dst->leaf.move = p->leaf.move;

	dst->flag = 0;
	if (position_is_done(p, book)) dst->flag |= LIBEDAX_FLAG_DONE;
	if (position_is_todo(p, book)) dst->flag |= LIBEDAX_FLAG_TODO;
	dst->n_player_bestpaths = dst->n_opponent_bestpaths = 0;
	if (lib_bestpath_is_valid(book)) {
		const size_t k = lib_bestpath_index(book, p);
		dst->n_player_bestpaths = lib_bestpath.count[k][0];
		dst->n_opponent_bestpaths = lib_bestpath.count[k][1];
		if (lib_bestpath.black[k]) dst->flag |= LIBEDAX_FLAG_BESTPATH_BLACK;
	}

	for (i = 0; i < p->n_link; ++i) {
		link[i].score = l[i].score;
		link[i].move = l[i].move;
	}
	dst->link = link;

	dst->n_wins = p->n_wins;
	dst->n_draws = p->n_draws;
	dst->n_losses = p->n_losses;
	dst->n_lines = p->n_lines;
	dst->score.value = p->score.value;
	dst->score.lower = p->score.lower;
	dst->score.upper = p->score.upper;
	dst->n_link = p->n_link;
	dst->level = p->level;
}

/**
 * @brief Get the moves and the position of a board from the book.
 *
 * @param book Opening book.
 * @param board Board.
 * @param movelist List of moves of the api (out parameter).
 * @param position Position of the api (out parameter).
 * @return symetry. If -1, it means "no book".
 */
static int lib_book_get_moves_with_position(Book *book, const Board *board, LibedaxMoveList *movelist, LibedaxPosition *position)
{
	const Position *p = book_probe(book, board);

	if (p) {
		MoveList moves;
		const int sym = position_get_moves(p, board, &moves);
		lib_movelist_set(movelist, &moves);
		lib_position_set(position, p, book);
		return sym;
	}
	return -1;
}

/*
 * Initialization
 */

/**
 * @brief default search oberver for libedax.
 * @param result Search Result.
 */
static void libedax_observer(Result *result)
{
	(void) result; // do nothing
}

/**
 * @brief initialize libedax api.
 * @param ui user interface.
 */
static void ui_init_libedax(UI *ui)
{
	Play *play = ui->play;

	book_verbose = false;
	play_init(play, &ui->book);
	play->search.options.header = NULL;
	play->search.options.separator = NULL;
	ui->book.search = &play->search;
	if (!book_load(&ui->book, options.book_file) && ui->book.array == NULL) {
		book_new(&ui->book, options.level, 60 - get_book_depth(options.level));
		ui->book.need_saving = false; // keep the damaged input file untouched
	}
	book_set_startup_depth(&ui->book);
	play->search.id = 1;
	search_set_observer(&play->search, libedax_observer);
	ui->mode = options.mode;
	play->type = ui->type;
}

/**
 * @brief free resources used by libedax api.
 * @param ui user interface.
 */
static void ui_free_libedax(UI *ui)
{
	if (ui->book.need_saving) book_save(&ui->book, options.book_file);
	book_free(&ui->book);
	play_free(ui->play);
	book_verbose = false;
}

/**
 * @brief Refuse a name of the book file which is too long.
 *
 * The book commands save their progress to the book file name with an extension (".store",
 * ".dev2", ...), in buffers of FILENAME_MAX characters.
 *
 * @param previous Name to restore (a copy, freed or kept by this function), or NULL for the default name.
 */
static void lib_check_book_file(char *previous)
{
	if (options.book_file && strlen(options.book_file) > FILENAME_MAX - 8) {
		warn("the name of the book file is too long: ignored\n");
		free(options.book_file);
		options.book_file = previous; // NULL: options_bound() sets the default name
	} else {
		free(previous);
	}
}

/**
 * @brief edax init function for library use.
 *
 * @param argc Number of arguments.
 * @param argv Command line arguments.
 */
LIBEDAX_API void libedax_initialize(int argc, char **argv)
{
	int i, r;

	if (g_ui != NULL) return;

	if (lib_is_started) {
		options = lib_default_options;
	} else {
		lib_default_options = options;
		lock_init(&lib_bench);
		lock_init(&lib_book_change);
		lib_is_started = true;
	}

	// options.n_task default to system cpu number
	options.n_task = get_cpu_number();

	// options from edax.ini & config.ini
	options_parse_defaults("edax.ini");
	options_parse_defaults("config.ini");

	// set verbosity
	options.verbosity = 0;

	// allocate ui
	g_ui = (UI*) mm_malloc(sizeof *g_ui);	// Eval in Search in Play in UI
	if (g_ui == NULL) fatal_error("Cannot allocate a user interface.\n");
	memset(g_ui, 0, sizeof *g_ui);
	g_ui->type = UI_LIBEDAX;
	g_ui->init = ui_init_libedax;
	g_ui->free = ui_free_libedax;
	g_ui->loop = NULL;

	// parse arguments
	if (argv == NULL) argc = 0;
	for (i = 1; i < argc; i++) {
		const char *arg = argv[i];
		if (arg == NULL) continue;
		while (*arg == '-') ++arg;
		if (strcmp(arg, "v") == 0 || strcmp(arg, "version") == 0) version();
		else if ((r = options_read(arg, i + 1 < argc ? argv[i + 1] : NULL)) > 0) i += r - 1;
		else warn("unknown or incomplete option \"%s\" ignored\n", argv[i]);
	}
	lib_check_book_file(NULL);
	options_bound();

	// initialize
	bit_init();
	edge_stability_init();
	statistics_init();
	eval_open(options.eval_file);
	search_global_init();

	g_ui->init(g_ui);
}

/**
 * @brief edax terminate function for library use.
 */
LIBEDAX_API void libedax_terminate(void)
{
	if (g_ui == NULL) return;

	if (g_ui->free) g_ui->free(g_ui);
	lib_bestpath_free();

	// the evaluation function stays loaded: it is reused by the next initialization
	options_free();
	options.ggs_host = options.ggs_login = options.ggs_password = options.ggs_port = NULL;
	options.game_file = options.ui_log_file = options.search_log_file = options.ggs_log_file = NULL;
	options.name = options.book_file = options.eval_file = NULL;

	mm_free(g_ui);
	g_ui = NULL;
}

/**
 * @brief auto go with regard to mode.
 */
static void lib_auto_go(void)
{
	Play *play;
	int repeat = options.repeat;

	if (g_ui == NULL) return;
	play = g_ui->play;
	for (;;) {
		if (!play_is_game_over(play) && (g_ui->mode == !play->player || g_ui->mode == 2)) {
			play_go(play, true);
			if (g_ui->mode != 2) play_ponder(play);
		} else {
			/* automatic rules after a game over*/
			if (play_is_game_over(play)) {
				if (options.auto_store) {
					const bool running = lib_book_change_set(true);
					lib_bestpath_free();
					play_store(play);
					lib_book_change_set(running);
				}
				if (options.auto_swap && g_ui->mode < 2) g_ui->mode = !g_ui->mode;
				if (options.repeat && repeat > 1) {
					--repeat;
					play_new(play);
					continue;
				}
				if (options.auto_quit) return;
				if (options.auto_start) {
					play_new(play);
					continue;
				}
			}
			return;
		}
	}
}

/*
 * Game
 */

/**
 * @brief init command.
 */
LIBEDAX_API void edax_init(void)
{
	Play *play;
	if (g_ui == NULL) return;
	play = g_ui->play;
	// new game from standard position
	board_init(&play->initial_board);
	play->initial_player = BLACK;
	play_force_init(play, "F5");
	play_new(play);
}

/**
 * @brief new command.
 */
LIBEDAX_API void edax_new(void)
{
	if (g_ui == NULL) return;
	// new game from personnalized position
	play_new(g_ui->play);
}

/**
 * @brief load command.
 * @param file file name to open.
 */
LIBEDAX_API void edax_load(const char *file)
{
	if (g_ui == NULL || file == NULL) return;
	// open a saved game
	play_load(g_ui->play, file);
}

/**
 * @brief save command.
 * @param file file name to save.
 */
LIBEDAX_API void edax_save(const char *file)
{
	if (g_ui == NULL || file == NULL) return;
	// save a game
	play_save(g_ui->play, file);
}

/**
 * @brief undo command.
 */
LIBEDAX_API void edax_undo(void)
{
	Play *play;
	if (g_ui == NULL) return;
	play = g_ui->play;
	// undo last move
	play_undo(play);
	if (g_ui->mode == 0 || g_ui->mode == 1) play_undo(play);
}

/**
 * @brief redo command.
 */
LIBEDAX_API void edax_redo(void)
{
	Play *play;
	if (g_ui == NULL) return;
	play = g_ui->play;
	// redo last move
	play_redo(play);
	if (g_ui->mode == 0 || g_ui->mode == 1) play_redo(play);
}

/**
 * @brief mode command.
 * @param mode mode to set.
 */
LIBEDAX_API void edax_mode(const int mode)
{
	if (g_ui == NULL) return;
	g_ui->mode = mode;
	lib_auto_go();
}

/**
 * @brief setboard command.
 * @param board board to set.
 */
LIBEDAX_API void edax_setboard(const char *board)
{
	if (g_ui == NULL || board == NULL) return;
	// set a new initial position
	play_set_board(g_ui->play, board);
}

/**
 * @brief setboard command with board object.
 * @param board board to set (player: black discs, opponent: white discs).
 * @param turn player to play.
 */
LIBEDAX_API void edax_setboard_from_obj(const LibedaxBoard *board, const int turn)
{
	Play *play;
	if (g_ui == NULL || board == NULL) return;
	play = g_ui->play;

	// set a new initial position
	play_stop_pondering(play);
	play->initial_board.player = board->player;
	play->initial_board.opponent = board->opponent;
	if (turn == BLACK) {
		play->initial_player = BLACK;
	} else if (turn == WHITE) {
		board_swap_players(&play->initial_board);
		play->initial_player = WHITE;
	} else {
		play->initial_player = EMPTY;
	}
	if (play->initial_board.player & play->initial_board.opponent) { /* bad board */
		play->initial_board.opponent &= (~play->initial_board.player);
	}
	if (play->initial_player == EMPTY) { /* bad turn */
		play->initial_player = (board_count_empties(&play->initial_board) & 1);
		if (play->initial_player == WHITE) board_swap_players(&play->initial_board);
	}
	play_force_init(play, "");
	play_new(play);
}

/**
 * @brief vmirror command.
 */
LIBEDAX_API void edax_vmirror(void)
{
	if (g_ui == NULL) return;
	// vertical mirror
	play_symetry(g_ui->play, 2);
}

/**
 * @brief hmirror command.
 */
LIBEDAX_API void edax_hmirror(void)
{
	if (g_ui == NULL) return;
	// horizontal mirror
	play_symetry(g_ui->play, 1);
}

/**
 * @brief rotate command.
 * @param angle angle for rotation.
 */
LIBEDAX_API void edax_rotate(const int angle)
{
	Play *play;
	int a = angle % 360;
	if (g_ui == NULL) return;
	play = g_ui->play;
	// rotate
	if (a < 0) a += 360;
	switch (a) {
	case 90:
		play_symetry(play, 5);
		break;
	case 180:
		play_symetry(play, 3);
		break;
	case 270:
		play_symetry(play, 6);
		break;
	default:
		break;
	}
}

/**
 * @brief symetry command.
 * @param sym symetry.
 */
LIBEDAX_API void edax_symetry(const int sym)
{
	Play *play;
	if (g_ui == NULL) return;
	play = g_ui->play;
	// direct symetry...
	if (sym >= 0 && sym < 16) {
		if (sym & 8) play->player ^= 1;
		play_symetry(play, sym & 7);
	}
}

/**
 * @brief play command.
 * @param moves moves (changed to lower case).
 */
LIBEDAX_API void edax_play(char *moves)
{
	if (g_ui == NULL || moves == NULL) return;
	// play a serie of moves
	string_to_lowercase(moves);
	play_game(g_ui->play, moves);

	lib_auto_go();
}

/**
 * @brief force command.
 * @param moves moves (changed to lower case).
 */
LIBEDAX_API void edax_force(char *moves)
{
	if (g_ui == NULL || moves == NULL) return;
	// force edax to play an opening
	string_to_lowercase(moves);
	play_force_init(g_ui->play, moves);
}

/**
 * @brief Test edax speed (see obf_speed), giving the progress to edax_bench_get_result.
 * @param search Search.
 * @param n Number of problems (-1: 1 minute).
 */
static void lib_obf_speed(Search *search, const int n)
{
	int i;
	unsigned long long t = real_clock();
	unsigned long long T = 0, n_nodes = 0;
	const int level = options.level;
	Random r;
	OBF obf;

	obf.n_moves = 0;
	obf.best_score = -SCORE_INF;

	random_seed(&r, 42);
	options.level = 60;
	search_set_observer(search, search_observer);
	search->options.verbosity = (options.verbosity == 1 ? 0 : options.verbosity);
	options.width -= 4;

	for (i = 0; n == - 1 ? real_clock() - t < 60000 : i < n; ++i) {
		const int ply = MAX(30, 40 - i / 5);
		obf.player = ply & 1;
		board_rand(&obf.board, ply, &r);
		obf_search(search, &obf, i + 1);
		T += search_time(search);
		n_nodes += search_count_nodes(search);

		lock(&lib_bench);
		if (lib_bench.result) {
			lib_bench.result->T = T;
			lib_bench.result->n_nodes = n_nodes;
			lib_bench.result->positions = i + 1;
		}
		unlock(&lib_bench);
	}

	printf("%d positions solved: ", i);
	if (n_nodes) printf("%llu nodes in ", n_nodes);
	time_print(T, false, stdout);
	if (T > 0 && n_nodes > 0) printf(" (%8.0f nodes/s).", 1000.0 * n_nodes / T);
	putchar('\n');

	options.level = level;
	options.width += 4;
	search_set_observer(search, libedax_observer);
}

/**
 * @brief bench (a serie of low level tests) command.
 * @param result result (out parameter).
 * @param n number of problems (-1: 1 minute).
 */
LIBEDAX_API void edax_bench(LibedaxBenchResult *result, int n)
{
	if (result == NULL) return;
	result->T = 0;
	result->n_nodes = 0;
	result->positions = 0;
	BOUND(n, -1, 100, "n_problems");
	if (g_ui == NULL) return;
	play_stop_pondering(g_ui->play); // the bench uses the search of the game

	lock(&lib_bench);
	lib_bench.result = result;
	unlock(&lib_bench);

	lib_obf_speed(&g_ui->play->search, n);

	lock(&lib_bench);
	lib_bench.result = NULL;
	unlock(&lib_bench);
}

/**
 * @brief get the result of the running bench (from another thread).
 * @param result result (out parameter, unchanged if no bench is running).
 */
LIBEDAX_API void edax_bench_get_result(LibedaxBenchResult *result)
{
	if (!lib_is_started || result == NULL) return;
	lock(&lib_bench);
	if (lib_bench.result != NULL) {
		result->T = lib_bench.result->T;
		result->n_nodes = lib_bench.result->n_nodes;
		result->positions = lib_bench.result->positions;
	}
	unlock(&lib_bench);
}

/**
 * @brief go command.
 */
LIBEDAX_API void edax_go(void)
{
	Play *play;
	if (g_ui == NULL) return;
	play = g_ui->play;
	// go think!
	if (play_is_game_over(play)) return;
	play_go(play, true);

	lib_auto_go();
}

/**
 * @brief hint command.
 * @param n number of hints.
 * @param hintlist result (out parameter).
 */
LIBEDAX_API void edax_hint(const int n_hints, LibedaxHintList *hintlist)
{
	Play *play;
	Search *search;
	MoveList book_moves;
	Move *m;
	Line pv;
	LibedaxHint *hint;
	int n = MAX(n_hints, 0); // a negative number asked for all the book moves

	if (hintlist == NULL) return;
	hint = hintlist->hint + 1;
	hintlist->n_hints = 0;
	if (g_ui == NULL) return;
	play = g_ui->play;
	search = &play->search;

	if (play_is_game_over(play)) return;

	play_stop_pondering(play);

	play->state = IS_THINKING;

	search->options.verbosity = options.verbosity;
	search_set_board(search, &play->board, play->player);
	search_set_level(search, options.level, search->eval.n_empties);
	if (n > search->movelist.n_moves) n = search->movelist.n_moves;
	if (n > LIBEDAX_MOVELIST_SIZE - 1) n = LIBEDAX_MOVELIST_SIZE - 1;

	if (options.book_allowed && book_get_moves(play->book, &play->board, &book_moves)) {
		foreach_move (m, book_moves) if (n) {
			--n;
			line_init(&pv, play->player);
			book_get_line(play->book, &play->board, m, &pv);
			lib_hint_set_book(hint, m, &pv);
			++hint;
			++(hintlist->n_hints);

			movelist_exclude(&search->movelist, m->x);
		}
	}

	while (n-- > 0) {
		if (options.play_type == EDAX_TIME_PER_MOVE) search_set_move_time(search, options.time);
		else search_set_game_time(search, play->time[play->player].left);
		if (n) search->options.multipv_depth = 60;
		search_run(search);
		search->options.multipv_depth = MULTIPV_DEPTH;
		lib_hint_set_result(hint, search->result);
		++hint;
		++(hintlist->n_hints);

		if (search->stop != STOP_END) break;
		movelist_exclude(&search->movelist, search->result->move);
	}
}

/**
 * @brief get book moves.
 * @param move_list result (out parameter).
 */
LIBEDAX_API void edax_get_bookmove(LibedaxMoveList *move_list)
{
	Play *play;
	MoveList moves;

	if (move_list == NULL) return;
	lib_movelist_set_empty(move_list);
	if (g_ui == NULL) return;
	play = g_ui->play;
	if (play_is_game_over(play)) return;

	play_stop_pondering(play);

	play->state = IS_THINKING;

	if (options.book_allowed && book_get_moves(play->book, &play->board, &moves)) lib_movelist_set(move_list, &moves);
}

/**
 * @brief get book moves.
 * @param move_list result (out parameter).
 * @param position result (out parameter).
 * @return symetry. If -1, it means "no book" or "game over".
 */
LIBEDAX_API int edax_get_bookmove_with_position(LibedaxMoveList *move_list, LibedaxPosition *position)
{
	Play *play;

	if (move_list == NULL || position == NULL) return -1;
	lib_movelist_set_empty(move_list);
	if (g_ui == NULL) return -1;
	play = g_ui->play;
	if (play_is_game_over(play)) return -1;

	play_stop_pondering(play);

	play->state = IS_THINKING;

	if (options.book_allowed) {
		return lib_book_get_moves_with_position(play->book, &play->board, move_list, position);
	} else {
		return -1;
	}
}

/**
 * @brief get book moves of the position reached by a sequence of moves from the starting position.
 * @param moves moves (or an opening name).
 * @param move_list result (out parameter).
 * @param position result (out parameter).
 * @return symetry. If -1, it means "no book" or "game over".
 */
LIBEDAX_API int edax_get_bookmove_with_position_by_moves(const char *moves, LibedaxMoveList *move_list, LibedaxPosition *position)
{
	Board board;
	Move move;
	char *lower;
	const char *string, *next;
	int sym = -1;

	if (move_list == NULL || position == NULL) return -1;
	lib_movelist_set_empty(move_list);
	if (g_ui == NULL || moves == NULL) return -1;

	lower = string_duplicate(moves);
	if (lower == NULL) return -1;
	string_to_lowercase(lower);
	string = lower;

	// play the moves (as play_game)
	board_init(&board);
	next = opening_get_line(string);
	if (next) string = next;
	while ((next = parse_move(string, &board, &move)) != string || move.x == PASS) {
		string = next;
		board_update(&board, &move);
	}
	free(lower);

	if (board_is_game_over(&board)) return -1;
	if (options.book_allowed) {
		sym = lib_book_get_moves_with_position(g_ui->play->book, &board, move_list, position);
	}
	return sym;
}

/**
 * @brief hint command.
 * Call edax_hint_next after calling this function.
 * @param exclude_list moves not to search (or NULL).
 */
LIBEDAX_API void edax_hint_prepare(LibedaxMoveList *exclude_list)
{
	Play *play;
	Search *search;
	const LibedaxMove *m;

	if (g_ui == NULL) return;
	play = g_ui->play;
	search = &play->search;

	if (play_is_game_over(play)) return;

	play_stop_pondering(play);

	play->state = IS_THINKING;

	search->options.verbosity = options.verbosity;
	search_set_board(search, &play->board, play->player);
	search_set_level(search, options.level, search->eval.n_empties);
	if (options.depth >= 0) search->options.depth = MIN(options.depth, search->eval.n_empties);
	if (options.selectivity >= 0) search->options.selectivity = options.selectivity;

	if (exclude_list) {
		for (m = exclude_list->move[0].next; m; m = m->next) {
			movelist_exclude(&search->movelist, m->x);
		}
	}
}

/**
 * @brief get next hint.
 *
 * Evaluate the best move among the moves not given yet.
 *
 * @param hint result (out parameter).
 * @param multipv_depth_max search all the moves as principal variations.
 */
static void lib_hint_next(LibedaxHint *hint, const bool multipv_depth_max)
{
	Play *play;
	Search *search;
	MoveList book_moves;
	Move *m;
	Line pv;

	if (hint == NULL) return;
	lib_hint_set_nomove(hint);

	if (g_ui == NULL) return;
	play = g_ui->play;
	search = &play->search;

	if (play_is_game_over(play)) return;
	if (movelist_is_empty(&search->movelist)) return;

	if (options.book_allowed && book_get_moves(play->book, &play->board, &book_moves)) {
		foreach_move (m, book_moves) {
			if (movelist_exclude(&search->movelist, m->x) != NULL) {
				line_init(&pv, play->player);
				book_get_line(play->book, &play->board, m, &pv);
				lib_hint_set_book(hint, m, &pv);
				return;
			}
		}
	}

	if (options.play_type == EDAX_TIME_PER_MOVE) search_set_move_time(search, options.time);
	else search_set_game_time(search, play->time[play->player].left);
	if (multipv_depth_max) search->options.multipv_depth = 60;
	search_run(search);
	if (multipv_depth_max) search->options.multipv_depth = MULTIPV_DEPTH;

	lib_hint_set_result(hint, search->result);

	movelist_exclude(&search->movelist, search->result->move);
}

/**
 * @brief hint command.
 * Gets hint one by one. If there are no more hints, hint will be NOMOVE.
 * Call edax_hint_prepare before calling this function.
 * @param hint result (out parameter).
 */
LIBEDAX_API void edax_hint_next(LibedaxHint *hint)
{
	lib_hint_next(hint, true);
}

/**
 * @brief hint command.
 * Gets hint one by one. If there are no more hints, hint will be NOMOVE.
 * Call edax_hint_prepare before calling this function.
 * This command is for analyze use.
 * @param hint result (out parameter).
 */
LIBEDAX_API void edax_hint_next_no_multipv_depth(LibedaxHint *hint)
{
	lib_hint_next(hint, false);
}

/**
 * @brief stop command.
 */
LIBEDAX_API void edax_stop(void)
{
	if (g_ui == NULL) return;
	// stop thinking
	g_ui->mode = 3;
	lock(&lib_book_change);
	if (!lib_book_change.running) play_stop(g_ui->play);
	unlock(&lib_book_change);
}

/**
 * @brief version command.
 */
LIBEDAX_API void edax_version(void)
{
	version();
}

/**
 * @brief user move command.
 * @param move user move.
 * @return 1 if the move has been legally played, otherwise 0.
 */
LIBEDAX_API int edax_move(const char *move)
{
	if (g_ui == NULL || move == NULL) return 0;
	// user move
	if (!play_user_move(g_ui->play, move)) return 0;

	lib_auto_go();
	return 1;
}

/**
 * @brief dump options.
 */
LIBEDAX_API void edax_options_dump(void)
{
	options_dump(stdout);
}

/**
 * @brief opening command.
 * @return opening name.
 */
LIBEDAX_API const char* edax_opening(void)
{
	const char *name;
	if (g_ui == NULL) return NULL;
	// opening name
	name = play_show_opening_name(g_ui->play, opening_get_english_name);
	if (name == NULL) name = "?";
	return name;
}

/**
 * @brief ouverture command.
 * @return opening name in french.
 */
LIBEDAX_API const char* edax_ouverture(void)
{
	const char *name;
	if (g_ui == NULL) return NULL;
	// opening name in french
	name = play_show_opening_name(g_ui->play, opening_get_french_name);
	if (name == NULL) name = "?";
	return name;
}

/*
 * Opening book
 */

/**
 * @brief pre-process of book command.
 * @return the opening book.
 */
static Book* lib_book_begin(void)
{
	Play *play = g_ui->play;
	Book *book = play->book;

	play_stop_pondering(play); // the book commands use the search of the game
	book->search = &play->search;
	book->search->options.verbosity = book->options.verbosity;
	book->failed = false; // see book_add()
	return book;
}

/**
 * @brief post-process of book command.
 * @param book opening book.
 */
static void lib_book_end(Book *book)
{
	book->options.verbosity = book->search->options.verbosity;
	book->search->options.verbosity = options.verbosity;
	lib_book_change_set(false);
}

/**
 * @brief pre-process of a book command which may change the book.
 * @return the opening book.
 */
static Book* lib_book_begin_change(void)
{
	Book *book = lib_book_begin(); // stops the pondering, which reads the book

	lib_book_change_set(true);
	lib_bestpath_free();
	return book;
}

/**
 * @brief book store command.
 */
LIBEDAX_API void edax_book_store(void)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin_change();

	// store the last played game
	play_store(g_ui->play);

	lib_book_end(book);
}

/**
 * @brief Number of games that edax_book_store_games() learns at the same time.
 *
 * @return the book-store-tasks setting (auto: the number of threads); 1: a game after the other.
 */
LIBEDAX_API int edax_book_store_tasks(void)
{
	if (g_ui == NULL) return 1;
	return book_store_task_count();
}

/**
 * @brief Play games and store them into the book.
 *
 * For each game: play its first moves from the initial position, let Edax play both sides to
 * the end, then store the game into the book. It is what edax_init, edax_play, edax_go (until
 * the game is over) and edax_book_store do for a game, without what they print.
 * - With book-store-tasks = 1, the games are learned one after the other, exactly as these
 *   functions do. The current game is the last game.
 * - With book-store-tasks > 1 (or auto), as many games are played at the same time, each one
 *   with n-tasks / book-store-tasks threads, with the book as it was before the call; then
 *   the positions of all the games are searched at the same time and added to the book, which
 *   is linked, negamaxed and saved (to <book-file>.store) once. The current game is not changed.
 *
 * @param games Games, one per line ('\n'): the first moves of the game ("f5d6c3"), or
 * "<book randomness>,<moves>" ("2,f5d6c3"). Without randomness, the book-randomness setting is used.
 * Empty lines, lines that start with '#' and lines with "//" are not games.
 * @param status A character for each line (out parameter): '1' learned, '0' not learned (illegal move,
 * or not a game), then a '\0'. A buffer of (number of lines + 1) characters, or NULL.
 * @return number of learned games.
 */
LIBEDAX_API int edax_book_store_games(const char *games, char *status)
{
	Book *book;
	char *text, *line, *next, **moves;
	int *randomness, *result, *index;
	int i, n = 0, n_lines = 1, n_learned = 0;

	if (status) status[0] = '\0';
	if (g_ui == NULL || games == NULL) return 0;

	for (i = 0; games[i]; ++i) if (games[i] == '\n') ++n_lines;
	text = (char*) malloc(strlen(games) + 1);
	moves = (char**) malloc(n_lines * sizeof *moves);
	randomness = (int*) malloc(n_lines * sizeof *randomness);
	result = (int*) malloc(n_lines * sizeof *result);
	index = (int*) malloc(n_lines * sizeof *index);
	if (text && moves && randomness && result && index) {
		strcpy(text, games);
		for (i = 0, line = text; line; line = next, ++i) {
			next = strchr(line, '\n');
			if (next) *next++ = '\0';
			if (next == NULL && *line == '\0') break; // nothing after the last '\n'
			if (status) { status[i] = '0'; status[i + 1] = '\0'; }
			randomness[n] = options.book_randomness;
			moves[n] = play_learn_parse(line, randomness + n);
			if (moves[n] && *moves[n]) index[n++] = i;
		}

		if (n > 0) {
			book = lib_book_begin_change();
			n_learned = play_learn_games(g_ui->play, (const char *const*) moves, randomness, n, result);
			lib_book_end(book);
			if (status) for (i = 0; i < n; ++i) if (result[i] == 0) status[index[i]] = '1';
		}
	}
	free(text); free(moves); free(randomness); free(result); free(index);
	return n_learned;
}

/**
 * @brief book on command.
 */
LIBEDAX_API void edax_book_on(void)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin();

	// turn book usage on
	options.book_allowed = true;

	lib_book_end(book);
}

/**
 * @brief book off command.
 */
LIBEDAX_API void edax_book_off(void)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin();

	// turn book usage off
	options.book_allowed = false;

	lib_book_end(book);
}

/**
 * @brief book randomness command.
 * @param randomness randomness.
 */
LIBEDAX_API void edax_book_randomness(const int randomness)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin();

	// set book randomness
	options.book_randomness = randomness;

	lib_book_end(book);
}

/**
 * @brief book depth command.
 * @param depth depth.
 */
LIBEDAX_API void edax_book_depth(const int depth)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin();

	// set book depth (until which to learn)
	book->options.n_empties = 61 - depth;

	lib_book_end(book);
}

/**
 * @brief book new command.
 * @param level level.
 * @param depth depth.
 */
LIBEDAX_API void edax_book_new(const int level, const int depth)
{
	Book *book;
	if (g_ui == NULL) return;
	if (level < 0 || level > 60) { // such a level would read outside the table of the levels
		warn("book new: level %d is out of range; current book retained\n", level);
		return;
	}
	book = lib_book_begin_change();

	// create a new empty book
	book_free(book);
	book_new(book, level, 61 - depth);

	lib_book_end(book);
}

/**
 * @brief book load command.
 * @param book_file book file name to load.
 */
LIBEDAX_API void edax_book_load(const char *book_file)
{
	Book *book;
	Book next = {0};
	if (g_ui == NULL) return;
	book = lib_book_begin_change();

	// load an opening book (binary format) from the disc
	next.search = book->search;
	if (book_load(&next, book_file)) {
		book_free(book);
		*book = next;
	} else {
		book_free(&next);
		warn("Book %s was not loaded; current book retained\n", book_file);
	}

	lib_book_end(book);
}

/**
 * @brief book save command.
 * @param book_file book file name to save.
 */
LIBEDAX_API void edax_book_save(const char *book_file)
{
	Book *book;
	if (g_ui == NULL || book_file == NULL) return;
	book = lib_book_begin();

	// save an opening book (binary format) to the disc
	book_save(book, book_file);

	lib_book_end(book);
}

/**
 * @brief book import command.
 * @param import_file file name to import.
 */
LIBEDAX_API void edax_book_import(const char *import_file)
{
	Book *book;
	FILE *f;
	if (g_ui == NULL || import_file == NULL) return;

	// book_import() replaces the book by a new one when the file cannot be opened: keep the current book
	f = fopen(import_file, "r");
	if (f == NULL) {
		warn("Book %s was not imported; current book retained\n", import_file);
		return;
	}
	fclose(f);
	book = lib_book_begin_change();

	// import an opening book (text format)
	book_free(book);
	book_import(book, import_file);
	book_link(book);
	book_fix(book);
	book_negamax(book);
	book_sort(book);

	lib_book_end(book);
}

/**
 * @brief book export command.
 * @param export_file file name to export.
 */
LIBEDAX_API void edax_book_export(const char *export_file)
{
	Book *book;
	if (g_ui == NULL || export_file == NULL) return;
	book = lib_book_begin();

	// export an opening book (text format)
	book_export(book, export_file);

	lib_book_end(book);
}

/**
 * @brief book merge command.
 * @param book_file file name to merge.
 */
LIBEDAX_API void edax_book_merge(const char *book_file)
{
	Book *book;
	char file[FILENAME_MAX];
	if (g_ui == NULL || book_file == NULL) return;
	book = lib_book_begin_change();

	// merge an opening book to the current one
	if (book_merge_file(book, book_file)) { // the source book is streamed, not loaded
		book_link_parallel(book); // rebuild links before validating imported positions
		book_fix(book);
		book_negamax(book);
		book_sort_parallel(book);
		if (options.book_merge_auto_save) {
			// keep need_saving: the exit save to the book file behaves as before
			const bool need_saving = book->need_saving;
			if (strlen(options.book_file) + sizeof ".mrg" > sizeof file) {
				warn("Book file name too long; merged book was not saved\n");
			} else {
				file_add_ext(options.book_file, ".mrg", file);
				if (book_save(book, file)) printf("Merged book saved to %s\n", file);
				book->need_saving = need_saving;
			}
		}
	} else warn("Book %s was not merged\n", book_file);

	lib_book_end(book);
}

/**
 * @brief book fix command.
 */
LIBEDAX_API void edax_book_fix(void)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin_change();

	// fix an opening book
	book_fix(book); // do nothing (or edax is buggy)
	book_link(book); // links nodes
	book_negamax(book); // negamax nodes
	book_sort(book); // sort moves

	lib_book_end(book);
}

/**
 * @brief book negamax command.
 */
LIBEDAX_API void edax_book_negamax(void)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin_change();

	// negamax an opening book
	book_negamax(book); // negamax nodes
	book_sort(book); // sort moves

	lib_book_end(book);
}

/**
 * @brief book correct command.
 */
LIBEDAX_API void edax_book_correct(void)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin_change();

	// check and correct solved positions of the book
	book_correct_solved(book); // do nothing (or edax is buggy)
	book_fix(book); // do nothing (or edax is buggy)
	book_link(book); // links nodes
	book_negamax(book); // negamax nodes
	book_sort(book); // sort moves

	lib_book_end(book);
}

/**
 * @brief book prune command.
 */
LIBEDAX_API void edax_book_prune(void)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin_change();

	// prune an opening book
	book_prune(book); // remove unreachable lines.
	book_fix(book); // do nothing (or edax is buggy)
	book_link(book); // links nodes
	book_negamax(book); // negamax nodes
	book_sort(book); // sort moves

	lib_book_end(book);
}

/**
 * @brief book subtree command.
 */
LIBEDAX_API void edax_book_subtree(void)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin_change();

	// subtree an opening book
	book_subtree(book, &g_ui->play->board); // remove unreachable lines.
	book_fix(book); // do nothing (or edax is buggy)
	book_link(book); // links nodes
	book_negamax(book); // negamax nodes
	book_sort(book); // sort moves

	lib_book_end(book);
}

/**
 * @brief book stats clean command: clean the done/todo flags and the counts of the best paths.
 */
LIBEDAX_API void edax_book_stats_clean(void)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin_change();

	// clean book stats
	book_clean(book);

	lib_book_end(book);
}

/**
 * @brief book show command.
 * @param position position information (out parameter, unchanged if the position is not in the book).
 */
LIBEDAX_API void edax_book_show(LibedaxPosition *position)
{
	Book *book;
	const Position *p;
	if (g_ui == NULL || position == NULL) return;
	book = lib_book_begin();

	// show the current position as stored in the book
	p = book_probe(book, &g_ui->play->board);
	if (p) lib_position_set(position, p, book);

	lib_book_end(book);
}

/**
 * @brief book info command.
 * @param info book information (out parameter).
 */
LIBEDAX_API void edax_book_info(LibedaxBook *info)
{
	Book *book;
	if (g_ui == NULL || info == NULL) return;
	book = lib_book_begin();

	// show book general information
	memset(info, 0, sizeof *info);
	info->date.year = book->date.year;
	info->date.month = book->date.month;
	info->date.day = book->date.day;
	info->date.hour = book->date.hour;
	info->date.minute = book->date.minute;
	info->date.second = book->date.second;
	info->options.level = book->options.level;
	info->options.n_empties = book->options.n_empties;
	info->options.midgame_error = book->options.midgame_error;
	info->options.endcut_error = book->options.endcut_error;
	info->options.verbosity = book->options.verbosity;
	info->stats.n_nodes = (int) MIN(book->stats.n_nodes, (long long) INT_MAX);
	info->stats.n_links = (int) MIN(book->stats.n_links, (long long) INT_MAX);
	info->stats.n_todo = (int) MIN(book->stats.n_todo, (long long) INT_MAX);
	info->n = book->n;
	info->n_nodes = (int) MIN(book->n_nodes, (unsigned int) INT_MAX);

	lib_book_end(book);
}

/**
 * @brief count the number of best paths in book.
 * @param board board to count the number of best paths.
 * @param position the number of best paths (out parameter).
 */
LIBEDAX_API void edax_book_count_bestpath(LibedaxBoard *board, LibedaxPosition *position)
{
	Book *book;
	Board b;
	const Position *p;
	unsigned short n_player, n_opponent;

	if (g_ui == NULL || board == NULL || position == NULL) return;
	book = g_ui->play->book;
	if (!lib_bestpath_prepare(book, 0, 0, 0)) return;

	b.player = board->player;
	b.opponent = board->opponent;
	lib_bestpath.stop = RUNNING;
	lib_count_bestpath(book, &b, &n_player, &n_opponent);

	p = book_probe(book, &b);
	if (p) lib_position_set(position, p, book);
	else position->n_player_bestpaths = position->n_opponent_bestpaths = 1;
}

/**
 * @brief count the number of best paths in book.
 * @param board board to count the number of best paths.
 * @param position the number of best paths (out parameter).
 * @param p_lower lower limit for player (BESTPATH_BEST: best moves only).
 * @param o_lower lower limit for opponent (BESTPATH_BEST: best moves only).
 * @param turn turn.
 */
LIBEDAX_API void edax_book_count_board_bestpath(LibedaxBoard *board, LibedaxPosition *position, const int p_lower, const int o_lower, const int turn)
{
	Book *book;
	Board b;
	const Position *p;
	unsigned short n_player, n_opponent;

	if (g_ui == NULL || board == NULL || position == NULL) return;
	book = g_ui->play->book;
	// the counts depend on the limits of the two colors
	if (!lib_bestpath_prepare(book, 1, turn == BLACK ? p_lower : o_lower, turn == BLACK ? o_lower : p_lower)) return;

	b.player = board->player;
	b.opponent = board->opponent;
	lib_bestpath.stop = RUNNING;
	lib_count_board_bestpath(book, &b, p_lower, o_lower, turn, &n_player, &n_opponent);

	p = book_probe(book, &b);
	if (p) lib_position_set(position, p, book);
	else position->n_player_bestpaths = position->n_opponent_bestpaths = 1;
}

/**
 * @brief stop counting the number of best paths in book (from another thread).
 */
LIBEDAX_API void edax_book_stop_count_bestpath(void)
{
	lib_bestpath.stop = STOP_PONDERING;
}

/**
 * @brief book verbose command.
 * @param book_verbosity book verbosity.
 */
LIBEDAX_API void edax_book_verbose(const int book_verbosity)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin();

	// set book verbosity
	book->options.verbosity = book_verbosity;
	book->search->options.verbosity = book->options.verbosity;

	lib_book_end(book);
}

/**
 * @brief book add command.
 * @param base_file base file to add.
 */
LIBEDAX_API void edax_book_add(const char *base_file)
{
	Book *book;
	Base base;
	if (g_ui == NULL || base_file == NULL) return;
	book = lib_book_begin_change();

	// add positions from a game database
	base_init(&base);
	base_load(&base, base_file);
	book_add_base(book, &base);
	base_free(&base);

	lib_book_end(book);
}

/**
 * @brief book check command.
 * @param base_file base file to check.
 */
LIBEDAX_API void edax_book_check(const char *base_file)
{
	Book *book;
	Base base;
	if (g_ui == NULL || base_file == NULL) return;
	book = lib_book_begin();

	// check positions from a game database
	base_init(&base);
	base_load(&base, base_file);
	book_check_base(book, &base);
	base_free(&base);

	lib_book_end(book);
}

/**
 * @brief book extract command.
 * @param base_file base file to extract.
 */
LIBEDAX_API void edax_book_extract(const char *base_file)
{
	Book *book;
	Base base;
	if (g_ui == NULL || base_file == NULL) return;
	book = lib_book_begin();

	// extract pv to a game database
	base_init(&base);
	book_extract_skeleton(book, &base);
	base_save(&base, base_file);
	base_free(&base);

	lib_book_end(book);
}

/**
 * @brief book deviate command.
 * @param relative_error relative error.
 * @param absolute_error absolute error.
 */
LIBEDAX_API void edax_book_deviate(int relative_error, int absolute_error)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin_change();

	// add position using the "deviate algorithm"
	BOUND(relative_error, -129, 129, "relative error");
	BOUND(absolute_error, 0, 65, "absolute error");
	book_deviate(book, &g_ui->play->board, relative_error, absolute_error);

	lib_book_end(book);
}

/**
 * @brief book deviate2 command.
 * @param move_loss per-move loss limit.
 * @param total_loss cumulative loss limit for both players.
 */
LIBEDAX_API void edax_book_deviate2(int move_loss, int total_loss)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin_change();

	// add positions while limiting per-move and cumulative errors
	BOUND(move_loss, 0, 129, "per-move loss");
	BOUND(total_loss, 0, 7740, "cumulative loss");
	book_deviate2(book, &g_ui->play->board, move_loss, total_loss);

	lib_book_end(book);
}

/**
 * @brief book deviate3 command (as deviate2, including solved leaves).
 * @param move_loss per-move loss limit.
 * @param total_loss cumulative loss limit for both players.
 */
LIBEDAX_API void edax_book_deviate3(int move_loss, int total_loss)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin_change();

	BOUND(move_loss, 0, 129, "per-move loss");
	BOUND(total_loss, 0, 7740, "cumulative loss");
	book_deviate3(book, &g_ui->play->board, move_loss, total_loss);

	lib_book_end(book);
}

/**
 * @brief book enhance command.
 * @param midgame_error midgame error.
 * @param endcut_error endcut error.
 */
LIBEDAX_API void edax_book_enhance(int midgame_error, int endcut_error)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin_change();

	// add position using the "enhance algorithm"
	BOUND(midgame_error, 0, 129, "midgame error");
	BOUND(endcut_error, 0, 129, "endcut error");
	book_enhance(book, &g_ui->play->board, midgame_error, endcut_error);

	lib_book_end(book);
}

/**
 * @brief book fill command.
 * @param fill_depth fill depth.
 */
LIBEDAX_API void edax_book_fill(int fill_depth)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin_change();

	// add position by filling hole in the book
	BOUND(fill_depth, 1, 61, "fill depth");
	book_fill(book, fill_depth);

	lib_book_end(book);
}

/**
 * @brief book play command.
 */
LIBEDAX_API void edax_book_play(void)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin_change();

	// add positions by expanding positions with no-link
	book_play(book);

	lib_book_end(book);
}

/**
 * @brief book deepen command.
 * caution: Currently, this function does not work correctly.
 */
LIBEDAX_API void edax_book_deepen(void)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin_change();

	book_deepen(book);

	lib_book_end(book);
}

/**
 * @brief book feed-hash command.
 */
LIBEDAX_API void edax_book_feed_hash(void)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin();

	// add book positions to the hash table
	book_feed_hash(book, &g_ui->play->board, &g_ui->play->search);

	lib_book_end(book);
}

/**
 * @brief book add board preprocess.
 */
LIBEDAX_API void edax_book_add_board_pre_process(void)
{
	Book *book;
	if (g_ui == NULL) return;
	book = lib_book_begin_change();
	book_clean(book);
	lib_book_change_set(false); // edax_book_add_board sets it: the caller may never call the post-process
}

/**
 * @brief book add board postprocess.
 */
LIBEDAX_API void edax_book_add_board_post_process(void)
{
	if (g_ui == NULL) return;
	lib_book_end(g_ui->play->book);
}

/**
 * @brief book add board.
 * @param board board to add.
 */
LIBEDAX_API void edax_book_add_board(const LibedaxBoard *board)
{
	Board b;
	bool running;
	if (g_ui == NULL || board == NULL) return;
	play_stop_pondering(g_ui->play);
	running = lib_book_change_set(true); // also when edax_book_add_board_pre_process was not called
	lib_bestpath_free();
	b.player = board->player;
	b.opponent = board->opponent;
	book_add_board(g_ui->play->book, &b);
	lib_book_change_set(running);
}

/*
 * Game database
 */

/**
 * @brief base problem command.
 * @param base_file game database file.
 * @param n_empties number of empties.
 * @param problem_file problem_file to save.
 */
LIBEDAX_API void edax_base_problem(const char *base_file, const int n_empties, const char *problem_file)
{
	Base base;
	FILE *f;
	if (base_file == NULL || problem_file == NULL) return;
	if ((f = fopen(problem_file, "a")) == NULL) { // base_to_problem() does not check it
		warn("Cannot open file %s\n", problem_file);
		return;
	}
	fclose(f);
	base_init(&base);

	// extract problem from a game base
	base_load(&base, base_file);
	base_to_problem(&base, n_empties, problem_file);

	base_free(&base);
}

/**
 * @brief base tofen command.
 * @param base_file game database file.
 * @param n_empties number of empties.
 * @param problem_file problem_file to save.
 */
LIBEDAX_API void edax_base_tofen(const char *base_file, const int n_empties, const char *problem_file)
{
	Base base;
	FILE *f;
	if (base_file == NULL || problem_file == NULL) return;
	if ((f = fopen(problem_file, "a")) == NULL) { // base_to_FEN() does not check it
		warn("Cannot open file %s\n", problem_file);
		return;
	}
	fclose(f);
	base_init(&base);

	// extract FEN
	base_load(&base, base_file);
	base_to_FEN(&base, n_empties, problem_file);

	base_free(&base);
}

/**
 * @brief base correct command.
 * @param base_file game database file.
 * @param n_empties number of empties.
 */
LIBEDAX_API void edax_base_correct(const char *base_file, const int n_empties)
{
	Base base;
	if (g_ui == NULL || base_file == NULL) return;
	play_stop_pondering(g_ui->play);
	base_init(&base);

	// correct erroneous games
	if (base_load(&base, base_file)) { // a file which was not loaded is kept as it is
		base_analyze(&base, &g_ui->play->search, n_empties, true);
		remove(base_file);
		base_save(&base, base_file);
	}

	base_free(&base);
}

/**
 * @brief base complete command.
 * @param base_file game database file.
 */
LIBEDAX_API void edax_base_complete(const char *base_file)
{
	Base base;
	if (g_ui == NULL || base_file == NULL) return;
	play_stop_pondering(g_ui->play);
	base_init(&base);

	// terminate unfinished base
	if (base_load(&base, base_file)) { // a file which was not loaded is kept as it is
		base_complete(&base, &g_ui->play->search);
		remove(base_file);
		base_save(&base, base_file);
	}

	base_free(&base);
}

/**
 * @brief base convert command.
 * @param base_file_from input game database file.
 * @param base_file_to output game database file.
 */
LIBEDAX_API void edax_base_convert(const char *base_file_from, const char *base_file_to)
{
	Base base;
	if (base_file_from == NULL || base_file_to == NULL) return;
	base_init(&base);

	// convert a base to another format
	base_load(&base, base_file_from);
	base_save(&base, base_file_to);

	base_free(&base);
}

/**
 * @brief base unique command.
 * @param base_file_from input game database file.
 * @param base_file_to output game database file.
 */
LIBEDAX_API void edax_base_unique(const char *base_file_from, const char *base_file_to)
{
	Base base;
	if (base_file_from == NULL || base_file_to == NULL) return;
	base_init(&base);

	// make a base unique by removing identical games
	base_load(&base, base_file_from);
	base_unique(&base);
	base_save(&base, base_file_to);

	base_free(&base);
}

/*
 * Options & state
 */

/**
 * @brief set (option) command.
 * @param option_name name of option.
 * @param val value to set.
 */
LIBEDAX_API void edax_set_option(const char *option_name, const char *val)
{
	Play *play;
	char *book_file;
	if (g_ui == NULL) return;
	play = g_ui->play;

	/* edax options */
	book_file = string_duplicate(options.book_file);
	if (options_read(option_name, val)) {
		lib_check_book_file(book_file);
		options_bound();
		// parallel search changes:
		if (search_count_tasks(&play->search) != options.n_task) {
			play_stop_pondering(play);
			search_set_task_number(&play->search, options.n_task);
		}
		lib_auto_go();
	} else {
		free(book_file);
	}
}

/**
 * @brief get moves of the current game.
 * @param str buffer of length 160 + 1 or more (out parameter).
 * @return moves (equals to str).
 */
LIBEDAX_API char* edax_get_moves(char *str)
{
	const Play *play;
	int i;
	int player = BLACK;

	if (g_ui == NULL || str == NULL) return NULL;
	play = g_ui->play;
	for (i = 0; i < play->i_game && i < 80; ++i) {
		move_to_string(play->game[i].x, player, str + 2 * i);
		player = !player;
	}
	str[2 * i] = '\0';
	return str;
}

/**
 * @brief check if the current game is over.
 * @return 1 if game is over, otherwise 0.
 */
LIBEDAX_API int edax_is_game_over(void)
{
	if (g_ui == NULL) return 0;
	return play_is_game_over(g_ui->play) ? 1 : 0;
}

/**
 * @brief check if the current player can move.
 * @return 1 if the current player can move, otherwise 0.
 */
LIBEDAX_API int edax_can_move(void)
{
	const Board *board;
	if (g_ui == NULL) return 0;
	board = &g_ui->play->board;
	return can_move(board->player, board->opponent) ? 1 : 0;
}

/**
 * @brief get last move.
 * @param move last move (out parameter; NOMOVE if no move was played).
 */
LIBEDAX_API void edax_get_last_move(LibedaxMove *move)
{
	const Move *last;
	if (g_ui == NULL || move == NULL) return;

	last = play_get_last_move(g_ui->play);
	if (last) {
		lib_move_set(move, last);
	} else {
		move->flipped = 0;
		move->x = NOMOVE;
		move->score = 0;
		move->cost = 0;
		move->next = NULL;
	}
}

/**
 * @brief get current board.
 * @param board current board (out parameter).
 */
LIBEDAX_API void edax_get_board(LibedaxBoard *board)
{
	const Play *play;
	if (g_ui == NULL || board == NULL) return;
	play = g_ui->play;

	board->player = play->board.player;
	board->opponent = play->board.opponent;
}

/**
 * @brief get current player.
 * @return current player (0: BLACK, 1: WHITE).
 */
LIBEDAX_API int edax_get_current_player(void)
{
	if (g_ui == NULL) return -1;
	return g_ui->play->player;
}

/**
 * @brief get current number of discs.
 * @param color player's color (0: BLACK, 1: WHITE).
 * @return number of discs.
 */
LIBEDAX_API int edax_get_disc(const int color)
{
	const Play *play;
	if (g_ui == NULL) return -1;
	play = g_ui->play;
	return color == play->player ? bit_count(play->board.player) : bit_count(play->board.opponent);
}

/**
 * @brief get current number of legal moves.
 * @param color player's color (0: BLACK, 1: WHITE).
 * @return number of legal moves.
 */
LIBEDAX_API int edax_get_mobility_count(const int color)
{
	const Play *play;
	if (g_ui == NULL) return -1;
	play = g_ui->play;
	return color == play->player ?
		get_mobility(play->board.player, play->board.opponent) :
		get_mobility(play->board.opponent, play->board.player);
}

/**
 * @brief print play.
 */
LIBEDAX_API void edax_play_print(void)
{
	if (g_ui == NULL) return;
	play_print(g_ui->play, stdout);
}

/**
 * @brief enable book_verbose to get stdout by bprint.
 */
LIBEDAX_API void edax_enable_book_verbose(void)
{
	if (g_ui == NULL) return;
	book_verbose = true;
}

/**
 * @brief disable book_verbose not to get stdout by bprint.
 */
LIBEDAX_API void edax_disable_book_verbose(void)
{
	if (g_ui == NULL) return;
	book_verbose = false;
}

/**
 * @brief Check if current player should pass.
 * @param board board.
 * @return 1 if the player must pass, otherwise 0.
 */
LIBEDAX_API int edax_board_is_pass(const LibedaxBoard *board)
{
	Board b;
	if (g_ui == NULL || board == NULL) return 0;
	b.player = board->player;
	b.opponent = board->opponent;
	return board_is_pass(&b) ? 1 : 0;
}

/**
 * @brief Get square color.
 * @param board board.
 * @param x square.
 * @return 0 = player, 1 = opponent, 2 = empty.
 */
LIBEDAX_API int edax_board_get_square_color(const LibedaxBoard *board, const int x)
{
	Board b;
	if (g_ui == NULL || board == NULL) return -1;
	if (x < A1 || x > H8) return 2; // not a square: empty, without shifting by more than 63 bits
	b.player = board->player;
	b.opponent = board->opponent;
	return board_get_square_color(&b, x);
}

/*
 * CPU
 */
#if defined(_M_X64) || defined(__x86_64__)
	#if defined(_MSC_VER)
		#include <intrin.h>
		#include <immintrin.h>
		static void lib_cpuid(const unsigned int leaf, unsigned int r[4])
		{
			int v[4];
			__cpuidex(v, (int) leaf, 0);
			r[0] = (unsigned int) v[0]; r[1] = (unsigned int) v[1]; r[2] = (unsigned int) v[2]; r[3] = (unsigned int) v[3];
		}
		static unsigned long long lib_xgetbv(void) { return _xgetbv(0); }
	#else
		#include <cpuid.h>
		static void lib_cpuid(const unsigned int leaf, unsigned int r[4])
		{
			__cpuid_count(leaf, 0, r[0], r[1], r[2], r[3]);
		}
		static unsigned long long lib_xgetbv(void)
		{
			unsigned int a, d;
			__asm__ ("xgetbv" : "=a" (a), "=d" (d) : "c" (0));
			return ((unsigned long long) d << 32) | a;
		}
	#endif
#endif

/**
 * @brief Level of the CPU, to choose the library built for it.
 *
 * The library is built for several levels of x86-64 CPUs (see the makefiles): a program can
 * load the generic one, which runs on any of them, ask this level, and load the fastest
 * library that the CPU can run. (This function only uses instructions of any x86-64 CPU.)
 *
 * @return 4: x86-64-v4 (AVX-512), 3: x86-64-v3 (AVX2), 2: x86-64-v2 (POPCNT), 1: x86-64,
 *         0: not an x86-64 CPU.
 */
LIBEDAX_API int libedax_cpu_level(void)
{
#if defined(_M_X64) || defined(__x86_64__)
	const unsigned int v2 = (1u << 0) | (1u << 9) | (1u << 13) | (1u << 19) | (1u << 20) | (1u << 23); // SSE3 SSSE3 CX16 SSE4.1 SSE4.2 POPCNT
	const unsigned int v3 = (1u << 12) | (1u << 22) | (1u << 27) | (1u << 28) | (1u << 29); // FMA MOVBE OSXSAVE AVX F16C
	const unsigned int v3_7 = (1u << 3) | (1u << 5) | (1u << 8); // BMI1 AVX2 BMI2
	const unsigned int v4_7 = (1u << 16) | (1u << 17) | (1u << 28) | (1u << 30) | (1u << 31); // AVX512 F DQ CD BW VL
	unsigned int r[4], max_leaf, ecx_1, ebx_7;
	unsigned long long xcr0;

	lib_cpuid(0, r);
	max_leaf = r[0];
	if (max_leaf < 7) return 1;
	lib_cpuid(1, r);
	ecx_1 = r[2];
	if ((ecx_1 & v2) != v2) return 1;
	if ((ecx_1 & v3) != v3) return 2;
	lib_cpuid(7, r);
	ebx_7 = r[1];
	lib_cpuid(0x80000001u, r);
	xcr0 = lib_xgetbv();
	if ((ebx_7 & v3_7) != v3_7 || !(r[2] & (1u << 5)) || (xcr0 & 0x6) != 0x6) return 2; // LZCNT; the OS saves the ymm registers
	if ((ebx_7 & v4_7) != v4_7 || (xcr0 & 0xe6) != 0xe6) return 3; // the OS saves the zmm registers
	return 4;
#else
	return 0;
#endif
}

/*
 * Bit & board utilities.
 *
 * The original libedax exported bit_count, first_bit, last_bit, get_moves and can_move from
 * Edax. Here most of them are macros or inlined functions, so they are exported from the
 * functions below, under their original names. The bit functions do not use the tables of
 * Edax: as with the original libedax, they can be called before the initialization.
 */
#define LIB_STRING_(x) #x
#define LIB_STRING(x) LIB_STRING_(x)

static int lib_bit_count(unsigned long long b)
{
	b = b - ((b >> 1) & 0x5555555555555555ULL);
	b = (b & 0x3333333333333333ULL) + ((b >> 2) & 0x3333333333333333ULL);
	b = (b + (b >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
	return (int) ((b * 0x0101010101010101ULL) >> 56);
}

static int lib_first_bit(unsigned long long b)
{
	return lib_bit_count((b & (~b + 1)) - 1);
}

static int lib_last_bit(unsigned long long b)
{
	b |= b >> 1; b |= b >> 2; b |= b >> 4; b |= b >> 8; b |= b >> 16; b |= b >> 32;
	return lib_bit_count(b) - 1;
}

#if defined(_MSC_VER)

	#ifdef _M_IX86
		#define LIB_EXPORT(name) __pragma(comment(linker, "/EXPORT:" #name "=_libedax_" #name))
	#else
		#define LIB_EXPORT(name) __pragma(comment(linker, "/EXPORT:" #name "=libedax_" #name))
	#endif
	#define LIB_UTILITY(type, name, args) LIB_EXPORT(name) type libedax_##name args

	LIB_UTILITY(int, bit_count, (unsigned long long b)) { return lib_bit_count(b); }
	LIB_UTILITY(int, first_bit, (unsigned long long b)) { return lib_first_bit(b); }
	LIB_UTILITY(int, last_bit, (unsigned long long b)) { return lib_last_bit(b); }
	LIB_UTILITY(unsigned long long, get_moves, (const unsigned long long P, const unsigned long long O)) { return get_moves(P, O); }
	LIB_UTILITY(bool, can_move, (const unsigned long long P, const unsigned long long O)) { return can_move(P, O); }

#elif defined(__GNUC__)

	// a function of Edax with the same name is already exported: only the macros need a function
	#define LIB_UTILITY(type, name, args) \
		LIBEDAX_API type libedax_##name args __asm__(LIB_STRING(__USER_LABEL_PREFIX__) #name); \
		type libedax_##name args

	LIB_UTILITY(int, bit_count, (unsigned long long b)) { return lib_bit_count(b); }	// (a macro or an inlined function in Edax)
	#ifdef first_bit
	LIB_UTILITY(int, first_bit, (unsigned long long b)) { return lib_first_bit(b); }
	LIB_UTILITY(int, last_bit, (unsigned long long b)) { return lib_last_bit(b); }
	#endif
	#ifdef get_moves
	LIB_UTILITY(unsigned long long, get_moves, (const unsigned long long P, const unsigned long long O)) { return get_moves(P, O); }
	#endif

#endif
