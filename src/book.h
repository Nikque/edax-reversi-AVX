/**
 * @file book.h
 *
 * Header file for opening book management
 *
 * @date 1998 - 2020
 * @author Richard Delorme
 * @version 4.4
 */

#ifndef EDAX_BOOK_H
#define EDAX_BOOK_H

#include "base.h"
#include "board.h"
#include "game.h"
#include "search.h"
#include "util.h"
#include <stdbool.h>

/**
 * struct Book
 * @brief The opening book.
 */
typedef struct Book {
	struct {
		short year;
		char month, day;
		char hour, minute, second;
	} date;
	struct {
		int level;
		int n_empties;
		int midgame_error;
		int endcut_error;
		int verbosity;
	} options;
	struct {
		long long n_nodes;
		long long n_links;
		long long n_todo;
	} stats;
	struct PositionArray *array;
	struct PositionStack* stack;
	void *pool;                  /**< positions stored at load time (see book_load) */
	int n;
	unsigned int n_nodes;       /**< position count (saved as a 32-bit unsigned count) */
	bool need_saving;
	bool failed;                 /**< a position could not be added (out of memory): learning stops */
	unsigned char epoch; /**< current done/todo epoch (see book_clean) */
	struct {
		unsigned long long *item; /**< (bucket << 32 | index) of positions marked todo since book_clean */
		long long n, size;
		bool valid;               /**< false: book_expand must scan the whole book */
	} todo_list;
	unsigned char *visit;        /**< visit marks of a deviate walk (one byte per position), NULL outside a walk */
	unsigned int *visit_first;   /**< first visit mark of each bucket */
	Random random;
	Search *search;
} Book;

/**
 * struct GameStat
 * @brief Game statistics
 */
typedef struct GameStats {
 	unsigned long long n_wins;       /**< game win count */
	unsigned long long n_draws;      /**< game draw count */
	unsigned long long n_losses;     /**< game loss count */
	unsigned long long n_lines;      /**< unterminated line count */
} GameStats;

void book_init(Book*);
void book_free(Book*);

void book_new(Book*, int, int);
bool book_load(Book*, const char*);
void book_set_startup_depth(Book*);
bool book_save(Book*, const char*);
bool book_save_progress(Book*, const char*);
void book_import(Book*, const char*);
void book_export(Book*, const char*);
void book_merge(Book*, const Book*);
bool book_merge_file(Book*, const char*);
void book_link_parallel(Book*);
void book_fix_parallel(Book*);
void book_sort_parallel(Book*);
void book_sort(Book *book);
void book_negamax(Book*);
void book_prune(Book*);
void book_deepen(Book*);
void book_correct_solved(Book*);
void book_link(Book*);
void book_fix(Book*);
void book_fill(Book *book, const int);
void book_deviate(Book*, Board*, const int, const int);
void book_deviate2(Book*, Board*, const int, const int);
void book_deviate3(Book*, Board*, const int, const int);
void book_enhance(Book*, Board*, const int, const int);
void book_subtree(Book*, const Board*);
void book_play(Book*);

void book_info(Book*);
void book_show(Book*, Board*);
void book_stats(Book *book);
bool book_get_moves(Book*, const Board*, MoveList*);
bool book_get_random_move(Book*, const Board*, Move*, const int);
void book_get_game_stats(Book*, const Board*, GameStats*);
void book_get_line(Book*, const Board*, const Move*, Line*);

void book_add_board(Book*, const Board*);
void book_add_game(Book*, const Game*);
void book_add_base(Book*, const Base*);
int book_store_task_count(void);
int book_store_thread_count(void);
Search** book_store_searches(const Book*, const int, const int);
void book_store_release(void);
bool book_plan_begin(Book*);
void book_plan_board(Book*, const Board*);
void book_plan_search(Book*);
void book_plan_end(Book*);
bool book_early_begin(Book*, const int);
void book_early_boards(const Board*, const int);
bool book_early_search(Search*);
void book_early_end(void);
bool book_get_random_move_with(Book*, const Board*, Move*, const int, Random*);
void book_print(const char*, ...);
void book_check_base(Book*, const Base*);

void book_extract_skeleton(Book*, Base*);
void book_extract_positions(Book*, const int, const int);

void book_feed_hash(const Book*, Board*, Search*);

#endif /* EDAX_BOOK_H */

