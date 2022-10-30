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

#include "./QueryProcessor.h"

#include <iostream>
#include <algorithm>
#include <list>
#include <string>
#include <vector>

extern "C" {
  #include "./libhw1/CSE333.h"
}

using std::list;
using std::sort;
using std::string;
using std::vector;

namespace hw3 {

QueryProcessor::QueryProcessor(const list<string>& index_list, bool validate) {
  // Stash away a copy of the index list.
  index_list_ = index_list;
  array_len_ = index_list_.size();
  Verify333(array_len_ > 0);

  // Create the arrays of DocTableReader*'s. and IndexTableReader*'s.
  dtr_array_ = new DocTableReader* [array_len_];
  itr_array_ = new IndexTableReader* [array_len_];

  // Populate the arrays with heap-allocated DocTableReader and
  // IndexTableReader object instances.
  list<string>::const_iterator idx_iterator = index_list_.begin();
  for (int i = 0; i < array_len_; i++) {
    FileIndexReader fir(*idx_iterator, validate);
    dtr_array_[i] = fir.NewDocTableReader();
    itr_array_[i] = fir.NewIndexTableReader();
    idx_iterator++;
  }
}

QueryProcessor::~QueryProcessor() {
  // Delete the heap-allocated DocTableReader and IndexTableReader
  // object instances.
  Verify333(dtr_array_ != nullptr);
  Verify333(itr_array_ != nullptr);
  for (int i = 0; i < array_len_; i++) {
    delete dtr_array_[i];
    delete itr_array_[i];
  }

  // Delete the arrays of DocTableReader*'s and IndexTableReader*'s.
  delete[] dtr_array_;
  delete[] itr_array_;
  dtr_array_ = nullptr;
  itr_array_ = nullptr;
}

// This structure is used to store a index-file-specific query result.
typedef struct {
  DocID_t doc_id;  // The document ID within the index file.
  int     rank;    // The rank of the result so far.
} IdxQueryResult;

vector<QueryProcessor::QueryResult>
QueryProcessor::ProcessQuery(const vector<string>& query) const {
  Verify333(query.size() > 0);

  // STEP 1.
  // (the only step in this file)
  vector<QueryProcessor::QueryResult> final_result;
  vector<QueryProcessor::QueryResult> intermediate_result;

  // For each index file given, compute the query result
  for (int i = 0; i < array_len_; i++) {
    DocTableReader* doc_table_reader = dtr_array_[i];
    IndexTableReader* index_table_reader = itr_array_[i];

    // Process the first word of the query
    string first_word = query[0];
    DocIDTableReader* doc_id_table_reader =
                    index_table_reader->LookupWord(first_word);
    // If the index file does not contain the first word, we are done with this
    // index file and go to the next one
    if (doc_id_table_reader == nullptr) {
      continue;
    }
    // Get the documents which contain the word and produce the initial query
    // result
    list<DocIDElementHeader> doc_id_list = doc_id_table_reader->GetDocIDList();
    delete doc_id_table_reader;
    doc_id_table_reader = nullptr;

    for (auto header : doc_id_list) {
      DocID_t doc_id = header.doc_id;
      int32_t num_pos = header.num_positions;
      string doc_name;
      bool lookup = doc_table_reader->LookupDocID(doc_id, &doc_name);
      if (lookup) {
        QueryResult result;
        result.document_name = doc_name;
        result.rank = num_pos;
        intermediate_result.push_back(result);
      }
    }


    if (query.size() > 1) {
      bool nonexist = false;
      // Process the remaining words.
      for (vector<string>::size_type i = 1; i < query.size(); i++) {
        string word = query[i];
        doc_id_table_reader = index_table_reader->LookupWord(word);
        // If the index file does not contain the word, go to the next index
        // file
        if (doc_id_table_reader == nullptr) {
          nonexist = true;
          break;
        }
        // Get the documents which contain the word and produce the initial
        // query result
        doc_id_list = doc_id_table_reader->GetDocIDList();
        delete doc_id_table_reader;
        doc_id_table_reader = nullptr;

        // Iterate through the current result list. For each result, determine
        // if the current word is contained in the corresponding document. If
        // not, delete the result; otherwise increment the rank of it.
        for (auto it = intermediate_result.begin();
                  it < intermediate_result.end(); it++) {
          auto result = *it;
          bool has_match = false;
          // Search for the document that matches
          for (auto header : doc_id_list) {
            DocID_t doc_id = header.doc_id;
            string header_filename;
            if (doc_table_reader->LookupDocID(doc_id, &header_filename)) {
              // Found a match. Update the document's rank.
              if (header_filename.compare(result.document_name) == 0) {
                has_match = true;
                QueryResult new_result;
                new_result.rank = result.rank + header.num_positions;
                new_result.document_name = result.document_name;
                *it = new_result;
                break;
              }
            }
          }
          // There's no match. Remove the document from the result list.
          if (!has_match) {
            intermediate_result.erase(it);
            it--;
          }
        }
      }

      if (nonexist) {
        continue;
      }
    }

    // Push the intermediate result to the final result, and clear it
    for (auto result : intermediate_result) {
      final_result.push_back(result);
    }
    intermediate_result.clear();
  }

  // No word matches. Return an empty list.
  if (final_result.empty()) {
    return final_result;
  }

  // Sort the final results.
  sort(final_result.begin(), final_result.end());
  return final_result;
}

}  // namespace hw3
