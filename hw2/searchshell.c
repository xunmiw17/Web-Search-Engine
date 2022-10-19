/*
 * Copyright ©2022 Hal Perkins.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Fall Quarter 2022 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

// Feature test macro for strtok_r (c.f., Linux Programming Interface p. 63)
#define _XOPEN_SOURCE 600
#define WORD_LIMIT 1024
#define CHARACTER_LIMIT 10240

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "libhw1/CSE333.h"
#include "./CrawlFileTree.h"
#include "./DocTable.h"
#include "./MemIndex.h"

//////////////////////////////////////////////////////////////////////////////
// Helper function declarations, constants, etc

// Give user information about the correct usage of the program.
static void Usage(void);
// Reads a string from a given stream. Returns 0 if we reach end-of-file, otherwise
// returns 1.
static int GetNextLine(FILE* f, char** ret_str);


//////////////////////////////////////////////////////////////////////////////
// Main
int main(int argc, char** argv) {
  if (argc != 2) {
    Usage();
  }

  // Implement searchshell!  We're giving you very few hints
  // on how to do it, so you'll need to figure out an appropriate
  // decomposition into functions as well as implementing the
  // functions.  There are several major tasks you need to build:
  //
  //  - Crawl from a directory provided by argv[1] to produce and index
  //  - Prompt the user for a query and read the query from stdin, in a loop
  //  - Split a query into words (check out strtok_r)
  //  - Process a query against the index and print out the results
  //
  // When searchshell detects end-of-file on stdin (cntrl-D from the
  // keyboard), searchshell should free all dynamically allocated
  // memory and any other allocated resources and then exit.
  //
  // Note that you should make sure the fomatting of your
  // searchshell output exactly matches our solution binaries
  // to get full points on this part.

  char* dir_path = argv[1];
  printf("Indexing '%s'\n", dir_path);

  DocTable* doc_table;
  MemIndex* mem_index;

  // Crawls the given directory and builds an index
  if (!CrawlFileTree(dir_path, &doc_table, &mem_index)) {
    Usage();
  }

  char* query;
  char** words;
  char* token;
  char* saveptr;
  int i;

  while (1) {
    int query_len = 0;

    query = (char*) malloc(sizeof(char) * CHARACTER_LIMIT);
    words = (char**) malloc(sizeof(char*) * WORD_LIMIT);

    printf("enter query:\n");
    int result = GetNextLine(stdin, &query);
    // User terminates the program
    if (result == 0) {
      printf("shutting down...\n");
      break;
    }

    // Converts all characteres of query into lowercase
    i = 0;
    while (query[i] != '\0') {
      query[i] = tolower(query[i]);
      i++;
    }

    // Splits the query into words
    token = strtok_r(query, " ", &saveptr);
    while (token != NULL) {
      words[query_len] = token;
      query_len++;
      token = strtok_r(NULL, " ", &saveptr);
    }

    char *p = strchr(words[query_len - 1], '\n');
    if (p)
      *p = '\0';

    // Processes the query
    LinkedList* search_list = MemIndex_Search(mem_index, words, query_len);
    if (search_list == NULL) {
      free(query);
      free(words);
      continue;
    }
    LLIterator* ll_it = LLIterator_Allocate(search_list);
    // Prints each of the search result, in order of rank from high to low
    while (LLIterator_IsValid(ll_it)) {
      SearchResult* search_result;
      LLIterator_Get(ll_it, (LLPayload_t*) &search_result);
      DocID_t doc_id = search_result->doc_id;
      int rank = search_result->rank;

      char* doc_name = DocTable_GetDocName(doc_table, doc_id);
      printf("  %s (%d)\n", doc_name, rank);

      LLIterator_Next(ll_it);
    }
    LLIterator_Free(ll_it);
    free(query);
    free(words);
  }

  // Free up allocated memory
  DocTable_Free(doc_table);
  MemIndex_Free(mem_index);
  free(query);
  free(words);
  return EXIT_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////////
// Helper function definitions

static void Usage(void) {
  fprintf(stderr, "Usage: ./searchshell <docroot>\n");
  fprintf(stderr,
          "where <docroot> is an absolute or relative " \
          "path to a directory to build an index under.\n");
  exit(EXIT_FAILURE);
}

static int GetNextLine(FILE *f, char **ret_str) {
  char* buf = fgets(*ret_str, CHARACTER_LIMIT, f);
  if (buf == NULL) {
    return 0;
  }
  return 1;
}
