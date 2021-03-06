// Copyright (c) 2018 Shannon Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
#ifndef KV_IMPL_INCLUDE_H
#define KV_IMPL_INCLUDE_H
#include <deque>
#include <set>
#include <mutex>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include "swift/shannon_db.h"
#include "src/venice_kv.h"
#include "src/snapshot.h"
#include "src/column_family.h"
#include "src/req_id_que.h"

namespace shannon {
class KVImpl : public DB {
 public:
  KVImpl(const DBOptions& options, const std::string& dbname,const std::string& device);

  virtual ~KVImpl();
  virtual Status Put(const WriteOptions&, const Slice& key, const Slice& value);
  virtual Status Delete(const WriteOptions&, const Slice& key);
  virtual Status Write(const WriteOptions& options, WriteBatch* updates);
  virtual Status Read(const ReadOptions& options, ReadBatch* batch,
		      std::vector<std::pair<shannon::Status, std::string>>* values);
  virtual Status WriteNonatomic(const WriteOptions& options, WriteBatchNonatomic* updates);
  virtual Status Get(const ReadOptions& options, const Slice& key, std::string* value);
  virtual Status KeyExist(const ReadOptions& options, const Slice& key);
  virtual Status IngestExternFile(char *sst_filename, int verify,
                   std::vector<ColumnFamilyHandle*>* handles) override;
  virtual Status IngestExternFile(char *sst_filename, int verify) override;
  virtual Status BuildTable(const std::string& filename, std::vector<ColumnFamilyHandle*> &handles, std::vector<Iterator*> &iterators);
  virtual Status BuildSstFile(const std::string& dirname, const std::string& filename, ColumnFamilyHandle *handle, Iterator *iter, uint64_t file_size);
  virtual const Snapshot* GetSnapshot();
  virtual Status ReleaseSnapshot(const Snapshot* snapshot);
  virtual Iterator* NewIterator(const ReadOptions&);
  // ColumnFamily Interface
  virtual Status CreateColumnFamily(const ColumnFamilyOptions& options,
                   const std::string& column_family_name,
                   ColumnFamilyHandle** handle) override;
  virtual Status CreateColumnFamilies(const ColumnFamilyOptions& options,
                   const std::vector<std::string>& column_family_names,
                   std::vector<ColumnFamilyHandle*>* handles) override;
  virtual Status CreateColumnFamilies(
                   const std::vector<ColumnFamilyDescriptor>& column_families,
                   std::vector<ColumnFamilyHandle*>* handles) override;
  virtual Status DropColumnFamily(ColumnFamilyHandle* column_family) override;
  virtual Status DropColumnFamilies(
                   const std::vector<ColumnFamilyHandle*>& column_families) override;
  virtual Status DestroyColumnFamilyHandle(ColumnFamilyHandle* column_family) override;
  virtual Status Put(const WriteOptions& options,
                   ColumnFamilyHandle* column_family, const Slice& key,
                   const Slice& value) override;
  virtual Status Delete(const WriteOptions& options,
                   ColumnFamilyHandle* column_family, const Slice& key) override;
  virtual Status Get(const ReadOptions& options,
                   ColumnFamilyHandle* column_family, const Slice& key,
                   std::string* value) override;
  virtual Status KeyExist(const ReadOptions& options,
                   ColumnFamilyHandle* column_family, const Slice& key) override;
  virtual Iterator* NewIterator(const ReadOptions& options,
                   ColumnFamilyHandle* column_family) override;
  virtual Status NewIterators(const ReadOptions& options,
                   const std::vector<ColumnFamilyHandle*>& column_families,
                   std::vector<Iterator*>* iterators) override;
  virtual ColumnFamilyHandle* DefaultColumnFamily() const override;

  virtual Status GetAsync(const ReadOptions& options, const Slice& key,
                          char* val_buf, const int32_t buf_len,
                          int32_t* val_len, CallBackPtr* cb);
  virtual Status GetAsync(const ReadOptions& options,
                          ColumnFamilyHandle* column_family, const Slice& key,
                          char* val_buf, const int32_t buf_len,
                          int32_t* val_len, CallBackPtr* cb);
  virtual Status PutAsync(const WriteOptions&, const Slice& key,
                          const Slice& value, CallBackPtr* cb);
  virtual Status PutAsync(const WriteOptions& options,
                          ColumnFamilyHandle* column_family, const Slice& key,
                          const Slice& value, CallBackPtr* cb);
  virtual Status DeleteAsync(const WriteOptions&, const Slice& key,
                             CallBackPtr* cb);
  virtual Status DeleteAsync(const WriteOptions& options,
                             ColumnFamilyHandle* column_family,
                             const Slice& key, CallBackPtr* cb);
  virtual Status PollCompletion(int32_t* num_events, const uint64_t timeout_us);

  virtual Status status() const {
      return status_;
  }
  virtual Env* GetEnv() const override;
  virtual Status CompactRange(const CompactRangeOptions& options,
                              ColumnFamilyHandle* column_family,
                              const Slice* begin, const Slice* end) override;
  virtual bool GetProperty(ColumnFamilyHandle* column_family,
                             const Slice& property, std::string* value) override;
  virtual SequenceNumber GetLatestSequenceNumber() const override;
  virtual Status DisableFileDeletions() override;
  virtual Status GetLiveFiles(std::vector<std::string>& vec,
                              uint64_t* manifest_file_size,
                              bool flush_memtable = true) override;
  virtual Status EnableFileDeletions(bool force = true) override;
  virtual Status GetSortedWalFiles(VectorLogPtr& files) override;
  virtual Options GetOptions(ColumnFamilyHandle* column_family) const override;
  virtual const std::string& GetName() const override;
  virtual const ColumnFamilyHandle* GetColumnFamilyHandle(
                              int32_t column_family_id) const override;
  virtual int32_t GetIndex() const override;
  void SetCfOptoions(const Options & options){
            cf_options_ = options;
  }
 protected:
  Env* const env_;
 private:
  friend class DB;
  friend class KVIter;
  Status status_;
  int fd_;
  int db_;
  //Env* const env_;
  ColumnFamilyOptions cf_options_ ;
  const DBOptions options_;  // options_.comparator == &internal_comparator_
  const std::string dbname_;
  const std::string device_;
  ColumnFamilyHandleImpl* default_cf_handle_; // default column_family handle
  bool is_default_open_;
  std::vector<ColumnFamilyHandle*> handles_;
  Status Open();
  Status Open(const DBOptions& db_options,
          const std::vector<ColumnFamilyDescriptor>& column_families,
          std::vector<ColumnFamilyHandle*>* handles);
  void Close();
  KVImpl(const KVImpl&);
  void operator=(const KVImpl&);

  // aio support
  Status OpenAio();
  Status CloseAio();
  int req_size_;
  std::vector<CallBackPtr*> cb_mp_;
  std::vector<venice_kv> cmds_;
  std::vector<int*> val_lens_;
  ReqIdQue req_id_que_;
  int epollfd_dev_ = -1;
  struct epoll_event watch_events_;
  struct epoll_event list_of_events_[1];
  struct uapi_aioctx aioctx_;
};

}
#endif
