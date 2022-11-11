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

#include "./HashTableReader.h"

#include <stdint.h>  // for uint32_t, etc.
#include <cstdio>    // for (FILE *).
#include <list>      // for std::list.

extern "C" {
  #include "libhw1/CSE333.h"
}
#include "./Utils.h"  // for FileDup().


using std::list;

namespace hw3 {

HashTableReader::HashTableReader(FILE* f, IndexFileOffset_t offset)
  : file_(f), offset_(offset) {
  // STEP 1.
  // fread() the bucket list header in this hashtable from its
  // "num_buckets" field, and convert to host byte order.
  Verify333(fseek(file_, offset_, SEEK_SET) == 0);
  int bytes_read = fread(&header_, sizeof(BucketListHeader), 1, file_);
  Verify333(bytes_read == 1);
  header_.ToHostFormat();
}

HashTableReader::~HashTableReader() {
  fclose(file_);
  file_ = nullptr;
}

list<IndexFileOffset_t>
HashTableReader::LookupElementPositions(HTKey_t hash_key) const {
  // Figure out which bucket the hash value is in.  We assume
  // hash values are mapped to buckets using the modulo (%) operator.
  int bucket_num = hash_key % header_.num_buckets;

  // Figure out the offset of the "bucket_rec" field for this bucket.
  IndexFileOffset_t bucket_rec_offset =
      offset_ + sizeof(BucketListHeader) + sizeof(BucketRecord) * bucket_num;

  // STEP 2.
  // Read the "chain len" and "bucket position" fields from the
  // bucket record, and convert from network to host order.
  BucketRecord bucket_rec;
  Verify333(fseek(file_, bucket_rec_offset, SEEK_SET) == 0);
  int num_read = fread(&bucket_rec, sizeof(BucketRecord), 1, file_);
  Verify333(num_read == 1);
  bucket_rec.ToHostFormat();


  // This will be our returned list of element positions.
  list<IndexFileOffset_t> ret_val;


  // STEP 3.
  // Read the "element positions" fields from the "bucket" header into
  // the returned list.  Be sure to insert into the list in the
  // correct order (i.e., append to the end of the list).
  IndexFileOffset_t bucket_pos = bucket_rec.position;
  for (int i = 0; i < bucket_rec.chain_num_elements; i++) {
    Verify333(fseek(file_, bucket_pos, SEEK_SET) == 0);
    ElementPositionRecord elt_pos_rec;
    Verify333(fread(&elt_pos_rec,
                    sizeof(ElementPositionRecord), 1, file_) == 1);
    elt_pos_rec.ToHostFormat();
    ret_val.push_back(elt_pos_rec.position);
    bucket_pos += sizeof(ElementPositionRecord);
  }

  // Return the list.
  return ret_val;
}

}  // namespace hw3
