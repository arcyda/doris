// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "vparquet_column_reader.h"

#include <common/status.h>
#include <gen_cpp/parquet_types.h>
#include <vec/columns/columns_number.h>

#include "schema_desc.h"
#include "vparquet_column_chunk_reader.h"

namespace doris::vectorized {

Status ParquetColumnReader::create(FileReader* file, FieldSchema* field,
                                   const ParquetReadColumn& column,
                                   const tparquet::RowGroup& row_group,
                                   std::vector<RowRange>& row_ranges,
                                   std::unique_ptr<ParquetColumnReader>& reader) {
    if (field->type.type == TYPE_MAP || field->type.type == TYPE_STRUCT) {
        return Status::Corruption("not supported type");
    }
    if (field->type.type == TYPE_ARRAY) {
        return Status::Corruption("not supported array type yet");
    } else {
        VLOG_DEBUG << "field->physical_column_index: " << field->physical_column_index;
        tparquet::ColumnChunk chunk = row_group.columns[field->physical_column_index];
        ScalarColumnReader* scalar_reader = new ScalarColumnReader(column);
        scalar_reader->init_column_metadata(chunk);
        RETURN_IF_ERROR(scalar_reader->init(file, field, &chunk, row_ranges));
        reader.reset(scalar_reader);
    }
    return Status::OK();
}

void ParquetColumnReader::init_column_metadata(const tparquet::ColumnChunk& chunk) {
    auto chunk_meta = chunk.meta_data;
    int64_t chunk_start = chunk_meta.__isset.dictionary_page_offset
                                  ? chunk_meta.dictionary_page_offset
                                  : chunk_meta.data_page_offset;
    size_t chunk_len = chunk_meta.total_compressed_size;
    _metadata.reset(new ParquetColumnMetadata(chunk_start, chunk_len, chunk_meta));
}

void ParquetColumnReader::_skipped_pages() {}

Status ScalarColumnReader::init(FileReader* file, FieldSchema* field, tparquet::ColumnChunk* chunk,
                                std::vector<RowRange>& row_ranges) {
    BufferedFileStreamReader stream_reader(file, _metadata->start_offset(), _metadata->size());
    _row_ranges.reset(&row_ranges);
    _chunk_reader.reset(new ColumnChunkReader(&stream_reader, chunk, field));
    _chunk_reader->init();
    return Status::OK();
}

Status ScalarColumnReader::read_column_data(ColumnPtr& doris_column, DataTypePtr& type,
                                            size_t batch_size, size_t* read_rows, bool* eof) {
    if (_chunk_reader->remaining_num_values() <= 0) {
        // seek to next page header
        _chunk_reader->next_page();
        if (_row_ranges->size() != 0) {
            _skipped_pages();
        }
        // load data to decoder
        _chunk_reader->load_page_data();
    }
    size_t read_values = _chunk_reader->remaining_num_values() < batch_size
                                 ? _chunk_reader->remaining_num_values()
                                 : batch_size;
    *read_rows = read_values;
    WhichDataType which_type(type);
    switch (_metadata->t_metadata().type) {
    case tparquet::Type::INT32: {
        _chunk_reader->decode_values(doris_column, type, read_values);
        return Status::OK();
    }
    case tparquet::Type::INT64: {
        // todo: test int64
        return Status::OK();
    }
    default:
        return Status::Corruption("unsupported parquet data type");
    }
    return Status::OK();
}

void ScalarColumnReader::close() {}
}; // namespace doris::vectorized