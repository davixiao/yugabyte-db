//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
//
// The following only applies to changes made to this file as part of YugaByte development.
//
// Portions Copyright (c) YugaByte, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <string.h>

#include <string>

#include "yb/util/flags.h"

#include "yb/rocksdb/env.h"
#include "yb/rocksdb/port/port.h"
#include "yb/rocksdb/statistics.h"
#include "yb/rocksdb/util/aligned_buffer.h"

DECLARE_int32(rocksdb_file_starting_buffer_size);

namespace yb {

class PriorityThreadPoolSuspender;

}

namespace rocksdb {

class Statistics;
class HistogramImpl;

std::unique_ptr<RandomAccessFile> NewReadaheadRandomAccessFile(
  std::unique_ptr<RandomAccessFile>&& file, size_t readahead_size);

class SequentialFileReader {
 private:
  std::unique_ptr<SequentialFile> file_;

 public:
  explicit SequentialFileReader(std::unique_ptr<SequentialFile>&& _file)
      : file_(std::move(_file)) {}

  SequentialFileReader(SequentialFileReader&& o) ROCKSDB_NOEXCEPT {
    *this = std::move(o);
  }

  SequentialFileReader& operator=(SequentialFileReader&& o) ROCKSDB_NOEXCEPT {
    file_ = std::move(o.file_);
    return *this;
  }

  SequentialFileReader(const SequentialFileReader&) = delete;
  SequentialFileReader& operator=(const SequentialFileReader&) = delete;

  Status Read(size_t n, Slice* result, uint8_t* scratch);

  Status Skip(uint64_t n);

  SequentialFile* file() { return file_.get(); }
};

class RandomAccessFileReader {
 private:
  std::unique_ptr<RandomAccessFile> file_;
  Env*            env_;
  Statistics*     stats_;
  uint32_t        hist_type_;
  HistogramImpl*  file_read_hist_;

 public:
  explicit RandomAccessFileReader(std::unique_ptr<RandomAccessFile>&& raf,
                                  Env* env = nullptr,
                                  Statistics* stats = nullptr,
                                  Histograms hist_type = HISTOGRAM_ENUM_MAX,
                                  HistogramImpl* file_read_hist = nullptr)
      : file_(std::move(raf)),
        env_(env),
        stats_(stats),
        hist_type_(hist_type),
        file_read_hist_(file_read_hist) {}

  RandomAccessFileReader(RandomAccessFileReader&& o) ROCKSDB_NOEXCEPT {
    *this = std::move(o);
  }

  RandomAccessFileReader& operator=(RandomAccessFileReader&& o) ROCKSDB_NOEXCEPT {
    file_ = std::move(o.file_);
    env_ = std::move(o.env_);
    stats_ = std::move(o.stats_);
    hist_type_ = std::move(o.hist_type_);
    file_read_hist_ = std::move(o.file_read_hist_);
    return *this;
  }

  RandomAccessFileReader(const RandomAccessFileReader&) = delete;
  RandomAccessFileReader& operator=(const RandomAccessFileReader&) = delete;

  Status Read(uint64_t offset, size_t n, Slice* result, char* scratch,
              Statistics* statistics = nullptr) const;
  Status ReadAndValidate(
      uint64_t offset, size_t n, Slice* result, char* scratch, const yb::ReadValidator& validator,
      Statistics* statistics = nullptr);

  RandomAccessFile* file() { return file_.get(); }
};

// Use posix write to write data to a file.
class WritableFileWriter {
 private:
  std::unique_ptr<WritableFile> writable_file_;
  AlignedBuffer           buf_;
  size_t                  max_buffer_size_;
  // Actually written data size can be used for truncate
  // not counting padding data
  uint64_t                filesize_;
  // This is necessary when we use unbuffered access
  // and writes must happen on aligned offsets
  // so we need to go back and write that page again
  uint64_t                next_write_offset_;
  bool                    pending_sync_;
  bool                    pending_fsync_;
  const bool              direct_io_;
  const bool              use_os_buffer_;
  uint64_t                last_sync_size_;
  uint64_t                bytes_per_sync_;
  RateLimiter*            rate_limiter_;
  // When the writer is used by the priority thread pool's task, this task could pass provided
  // suspender to the writer, so it will be used by writer to check whether task should be
  // paused after block is flushed.
  yb::PriorityThreadPoolSuspender* suspender_;

 public:
  WritableFileWriter(std::unique_ptr<WritableFile>&& file,
                     const EnvOptions& options,
                     yb::PriorityThreadPoolSuspender* suspender = nullptr)
      : writable_file_(std::move(file)),
        buf_(),
        max_buffer_size_(options.writable_file_max_buffer_size),
        filesize_(0),
        next_write_offset_(0),
        pending_sync_(false),
        pending_fsync_(false),
        direct_io_(writable_file_->UseDirectIO()),
        use_os_buffer_(writable_file_->UseOSBuffer()),
        last_sync_size_(0),
        bytes_per_sync_(options.bytes_per_sync),
        rate_limiter_(options.rate_limiter),
        suspender_(suspender) {

    buf_.Alignment(writable_file_->GetRequiredBufferAlignment());
    buf_.AllocateNewBuffer(FLAGS_rocksdb_file_starting_buffer_size);
  }

  WritableFileWriter(const WritableFileWriter&) = delete;

  WritableFileWriter& operator=(const WritableFileWriter&) = delete;

  ~WritableFileWriter();

  Status Append(const Slice& data);

  Status Flush();

  Status Close();

  Status Sync(bool use_fsync);

  // Sync only the data that was already Flush()ed. Safe to call concurrently
  // with Append() and Flush(). If !writable_file_->IsSyncThreadSafe(),
  // returns NotSupported status.
  Status SyncWithoutFlush(bool use_fsync);

  uint64_t GetFileSize() { return filesize_; }

  Status InvalidateCache(size_t offset, size_t length);

  WritableFile* writable_file() const { return writable_file_.get(); }

 private:
  // Used when os buffering is OFF and we are writing
  // DMA such as in Windows unbuffered mode
  Status WriteUnbuffered();
  // Normal write
  Status WriteBuffered(const char* data, size_t size);
  Status RangeSync(uint64_t offset, uint64_t nbytes);
  size_t RequestToken(size_t bytes, bool align);
  Status SyncInternal(bool use_fsync);
};

extern Status NewWritableFile(Env* env, const std::string& fname,
                              std::unique_ptr<WritableFile>* result,
                              const EnvOptions& options);

// Returns an error if file ends with check_size zeros. If check_size is larger than file size it is
// automatically reset to file size.
// Returns an error if check_size == 0.
// Returned error contains size of contiguous zeroed array placed at the end of the file.
Status CheckFileTailForZeros(
    Env* env, const EnvOptions& env_options, const std::string& file_path, size_t check_size);

}  // namespace rocksdb
