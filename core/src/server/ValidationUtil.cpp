// Copyright (C) 2019-2020 Zilliz. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance
// with the License. You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied. See the License for the specific language governing permissions and limitations under the License.

#include "server/ValidationUtil.h"
#include "db/Constants.h"
#include "db/Utils.h"
#include "knowhere/index/vector_index/ConfAdapter.h"
#include "knowhere/index/vector_index/helpers/IndexParameter.h"
#include "utils/Log.h"
#include "utils/StringHelpFunctions.h"
#include "value/config/ServerConfig.h"

#include <fiu/fiu-local.h>
#include <algorithm>
#include <limits>
#include <set>
#include <string>

namespace milvus {
namespace server {

namespace {

Status
CheckParameterRange(const milvus::json& json_params, const std::string& param_name, int64_t min, int64_t max,
                    bool min_close = true, bool max_closed = true) {
    if (json_params.find(param_name) == json_params.end()) {
        std::string msg = "Parameter list must contain: ";
        msg += param_name;
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_ARGUMENT, msg);
    }

    try {
        int64_t value = json_params[param_name];
        bool min_err = min_close ? value < min : value <= min;
        bool max_err = max_closed ? value > max : value >= max;
        if (min_err || max_err) {
            std::string msg = "Invalid " + param_name + " value: " + std::to_string(value) + ". Valid range is " +
                              (min_close ? "[" : "(") + std::to_string(min) + ", " + std::to_string(max) +
                              (max_closed ? "]" : ")");
            LOG_SERVER_ERROR_ << msg;
            return Status(SERVER_INVALID_ARGUMENT, msg);
        }
    } catch (std::exception& e) {
        std::string msg = "Invalid " + param_name + ": ";
        msg += e.what();
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_ARGUMENT, msg);
    }

    return Status::OK();
}

Status
CheckParameterExistence(const milvus::json& json_params, const std::string& param_name) {
    if (json_params.find(param_name) == json_params.end()) {
        std::string msg = "Parameter list must contain: ";
        msg += param_name;
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_ARGUMENT, msg);
    }

    try {
        int64_t value = json_params[param_name];
        if (value < 0) {
            std::string msg = "Invalid " + param_name + " value: " + std::to_string(value);
            LOG_SERVER_ERROR_ << msg;
            return Status(SERVER_INVALID_ARGUMENT, msg);
        }
    } catch (std::exception& e) {
        std::string msg = "Invalid " + param_name + ": ";
        msg += e.what();
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_ARGUMENT, msg);
    }

    return Status::OK();
}

}  // namespace

Status
ValidateCollectionName(std::string& collection_name) {
    // trim side-blank of tag, only compare valid characters
    // for example: " ab cd " is treated as "ab cd"
    StringHelpFunctions::TrimStringBlank(collection_name);

    // Collection name shouldn't be empty.
    if (collection_name.empty()) {
        std::string msg = "Collection name should not be empty.";
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_COLLECTION_NAME, msg);
    }

    std::string invalid_msg = "Invalid collection name: " + collection_name + ". ";
    // Collection name size shouldn't exceed engine::MAX_NAME_LENGTH.
    if (collection_name.size() > engine::MAX_NAME_LENGTH) {
        std::string msg = invalid_msg + "The length of a collection name must be less than " +
                          std::to_string(engine::MAX_NAME_LENGTH) + " characters.";
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_COLLECTION_NAME, msg);
    }

    // Collection name first character should be underscore or character.
    char first_char = collection_name[0];
    if (first_char != '_' && std::isalpha(first_char) == 0) {
        std::string msg = invalid_msg + "The first character of a collection name must be an underscore or letter.";
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_COLLECTION_NAME, msg);
    }

    int64_t table_name_size = collection_name.size();
    for (int64_t i = 1; i < table_name_size; ++i) {
        char name_char = collection_name[i];
        if (name_char != '_' && name_char != '$' && std::isalnum(name_char) == 0) {
            std::string msg = invalid_msg + "Collection name can only contain numbers, letters, and underscores.";
            LOG_SERVER_ERROR_ << msg;
            return Status(SERVER_INVALID_COLLECTION_NAME, msg);
        }
    }

    return Status::OK();
}

Status
ValidateFieldName(std::string& field_name) {
    // trim side-blank of tag, only compare valid characters
    // for example: " ab cd " is treated as "ab cd"
    StringHelpFunctions::TrimStringBlank(field_name);

    // Field name shouldn't be empty.
    if (field_name.empty()) {
        std::string msg = "Field name should not be empty.";
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_FIELD_NAME, msg);
    }

    std::string invalid_msg = "Invalid field name: " + field_name + ". ";
    // Field name size shouldn't exceed engine::MAX_NAME_LENGTH.
    if (field_name.size() > engine::MAX_NAME_LENGTH) {
        std::string msg = invalid_msg + "The length of a field name must be less than " +
                          std::to_string(engine::MAX_NAME_LENGTH) + " characters.";
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_FIELD_NAME, msg);
    }

    // Field name first character should be underscore or character.
    char first_char = field_name[0];
    if (first_char != '_' && std::isalpha(first_char) == 0) {
        std::string msg = invalid_msg + "The first character of a field name must be an underscore or letter.";
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_FIELD_NAME, msg);
    }

    int64_t field_name_size = field_name.size();
    for (int64_t i = 1; i < field_name_size; ++i) {
        char name_char = field_name[i];
        if (name_char != '_' && std::isalnum(name_char) == 0) {
            std::string msg = invalid_msg + "Field name cannot only contain numbers, letters, and underscores.";
            LOG_SERVER_ERROR_ << msg;
            return Status(SERVER_INVALID_FIELD_NAME, msg);
        }
    }

    return Status::OK();
}

Status
ValidateVectorIndexType(std::string& index_type, bool is_binary) {
    // Index name shouldn't be empty.
    if (index_type.empty()) {
        std::string msg = "Index type should not be empty.";
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_FIELD_NAME, msg);
    }

    // string case insensitive
    std::transform(index_type.begin(), index_type.end(), index_type.begin(), ::toupper);

    static std::set<std::string> s_vector_index_type = {
        knowhere::IndexEnum::INVALID,
        knowhere::IndexEnum::INDEX_FAISS_IDMAP,
        knowhere::IndexEnum::INDEX_FAISS_IVFFLAT,
        knowhere::IndexEnum::INDEX_FAISS_IVFPQ,
        knowhere::IndexEnum::INDEX_FAISS_IVFSQ8,
#ifdef MILVUS_GPU_VERSION
        knowhere::IndexEnum::INDEX_FAISS_IVFSQ8H,
#endif
        knowhere::IndexEnum::INDEX_NSG,
        knowhere::IndexEnum::INDEX_HNSW,
        knowhere::IndexEnum::INDEX_ANNOY,
        knowhere::IndexEnum::INDEX_RHNSWFlat,
        knowhere::IndexEnum::INDEX_RHNSWPQ,
        knowhere::IndexEnum::INDEX_RHNSWSQ,
        knowhere::IndexEnum::INDEX_NGTPANNG,
        knowhere::IndexEnum::INDEX_NGTONNG,
    };

    static std::set<std::string> s_binary_index_types = {
        knowhere::IndexEnum::INDEX_FAISS_BIN_IDMAP,
        knowhere::IndexEnum::INDEX_FAISS_BIN_IVFFLAT,
    };

    std::set<std::string>& index_types = is_binary ? s_binary_index_types : s_vector_index_type;
    if (index_types.find(index_type) == index_types.end()) {
        std::string msg = "Invalid index type: " + index_type;
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_INDEX_TYPE, msg);
    }

    return Status::OK();
}

Status
ValidateStructuredIndexType(std::string& index_type) {
    // Index name shouldn't be empty.
    if (index_type.empty()) {
        std::string msg = "Index type should not be empty.";
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_FIELD_NAME, msg);
    }

    // string case insensitive
    std::transform(index_type.begin(), index_type.end(), index_type.begin(), ::toupper);

    static std::set<std::string> s_index_types = {
        engine::DEFAULT_STRUCTURED_INDEX,
    };

    if (s_index_types.find(index_type) == s_index_types.end()) {
        std::string msg = "Invalid index type: " + index_type;
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_INDEX_TYPE, msg);
    }

    return Status::OK();
}

Status
ValidateDimension(int64_t dim, bool is_binary) {
    if (dim <= 0 || dim > engine::MAX_DIMENSION) {
        std::string msg = "Invalid dimension: " + std::to_string(dim) + ". Should be in range 1 ~ " +
                          std::to_string(engine::MAX_DIMENSION) + ".";
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_VECTOR_DIMENSION, msg);
    }

    if (is_binary && (dim % 8) != 0) {
        std::string msg = "Invalid dimension: " + std::to_string(dim) + ". Should be multiple of 8.";
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_VECTOR_DIMENSION, msg);
    }

    return Status::OK();
}

Status
ValidateIndexParams(const milvus::json& index_params, int64_t dimension, const std::string& index_type) {
    if (engine::utils::IsFlatIndexType(index_type)) {
        return Status::OK();
    } else if (index_type == knowhere::IndexEnum::INDEX_FAISS_IVFFLAT ||
               index_type == knowhere::IndexEnum::INDEX_FAISS_IVFSQ8 ||
               index_type == knowhere::IndexEnum::INDEX_FAISS_IVFSQ8H ||
               index_type == knowhere::IndexEnum::INDEX_FAISS_BIN_IVFFLAT) {
        auto status = CheckParameterRange(index_params, knowhere::IndexParams::nlist, 1, 65536);
        if (!status.ok()) {
            return status;
        }
    } else if (index_type == knowhere::IndexEnum::INDEX_FAISS_IVFPQ) {
        auto status = CheckParameterRange(index_params, knowhere::IndexParams::nlist, 1, 65536);
        if (!status.ok()) {
            return status;
        }

        status = CheckParameterExistence(index_params, knowhere::IndexParams::m);
        if (!status.ok()) {
            return status;
        }

        if (index_params.find(knowhere::IndexParams::nbits) != index_params.end()) {
            status = CheckParameterRange(index_params, knowhere::IndexParams::nbits, 1, 16);
            if (!status.ok()) {
                return status;
            }
        }

        // special check for 'm' parameter
        int64_t m_value = index_params[knowhere::IndexParams::m];
        if (!milvus::knowhere::IVFPQConfAdapter::CheckCPUPQParams(dimension, m_value)) {
            std::string msg = "Invalid m, dimension cannot be divided by m ";
            LOG_SERVER_ERROR_ << msg;
            return Status(SERVER_INVALID_ARGUMENT, msg);
        }
    } else if (index_type == knowhere::IndexEnum::INDEX_NSG) {
        auto status = CheckParameterRange(index_params, knowhere::IndexParams::search_length, 10, 300);
        if (!status.ok()) {
            return status;
        }
        status = CheckParameterRange(index_params, knowhere::IndexParams::out_degree, 5, 300);
        if (!status.ok()) {
            return status;
        }
        status = CheckParameterRange(index_params, knowhere::IndexParams::candidate, 50, 1000);
        if (!status.ok()) {
            return status;
        }
        status = CheckParameterRange(index_params, knowhere::IndexParams::knng, 5, 300);
        if (!status.ok()) {
            return status;
        }
    } else if (index_type == knowhere::IndexEnum::INDEX_HNSW || index_type == knowhere::IndexEnum::INDEX_RHNSWFlat ||
               index_type == knowhere::IndexEnum::INDEX_RHNSWPQ || index_type == knowhere::IndexEnum::INDEX_RHNSWSQ ||
               index_type == knowhere::IndexEnum::INDEX_RHNSWFlat) {
        auto status = CheckParameterRange(index_params, knowhere::IndexParams::M, 4, 64);
        if (!status.ok()) {
            return status;
        }
        status = CheckParameterRange(index_params, knowhere::IndexParams::efConstruction, 8, 512);
        if (!status.ok()) {
            return status;
        }

        if (index_type == knowhere::IndexEnum::INDEX_RHNSWPQ) {
            status = CheckParameterExistence(index_params, knowhere::IndexParams::PQM);
            if (!status.ok()) {
                return status;
            }
            if (index_params.find(knowhere::IndexParams::nbits) != index_params.end()) {
                status = CheckParameterRange(index_params, knowhere::IndexParams::nbits, 1, 16);
                if (!status.ok()) {
                    return status;
                }
            }

            // special check for 'PQM' parameter
            int64_t pqm_value = index_params[knowhere::IndexParams::PQM];
            if (!milvus::knowhere::IVFPQConfAdapter::CheckCPUPQParams(dimension, pqm_value)) {
                std::string msg = "Invalid m, dimension cannot be divided by m ";
                LOG_SERVER_ERROR_ << msg;
                return Status(SERVER_INVALID_ARGUMENT, msg);
            }
        }
    } else if (index_type == knowhere::IndexEnum::INDEX_ANNOY) {
        auto status = CheckParameterRange(index_params, knowhere::IndexParams::n_trees, 1, 1024);
        if (!status.ok()) {
            return status;
        }
    }

    return Status::OK();
}

Status
ValidateSegmentRowLimit(int64_t segment_row_limit) {
    int64_t min = config.engine.build_index_threshold();
    int max = engine::MAX_SEGMENT_ROW_COUNT;
    if (segment_row_limit < min || segment_row_limit > max) {
        std::string msg = "Invalid segment row limit: " + std::to_string(segment_row_limit) + ". " +
                          "Should be in range " + std::to_string(min) + " ~ " + std::to_string(max) + ".";
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_SEGMENT_ROW_COUNT, msg);
    }
    return Status::OK();
}

Status
ValidateIndexMetricType(const std::string& metric_type, const std::string& index_type) {
    if (engine::utils::IsFlatIndexType(index_type)) {
        // pass
    } else if (index_type == knowhere::IndexEnum::INDEX_FAISS_BIN_IVFFLAT) {
        // binary
        if (metric_type != knowhere::Metric::HAMMING && metric_type != knowhere::Metric::JACCARD &&
            metric_type != knowhere::Metric::TANIMOTO) {
            std::string msg = "Index metric type " + metric_type + " does not match index type " + index_type;
            LOG_SERVER_ERROR_ << msg;
            return Status(SERVER_INVALID_ARGUMENT, msg);
        }
    } else {
        // float
        if (metric_type != knowhere::Metric::L2 && metric_type != knowhere::Metric::IP) {
            std::string msg = "Index metric type " + metric_type + " does not match index type " + index_type;
            LOG_SERVER_ERROR_ << msg;
            return Status(SERVER_INVALID_ARGUMENT, msg);
        }
    }

    return Status::OK();
}

Status
ValidateSearchMetricType(const std::string& metric_type, bool is_binary) {
    if (is_binary) {
        // binary
        if (metric_type == knowhere::Metric::L2 || metric_type == knowhere::Metric::IP) {
            std::string msg = "Cannot search binary entities with index metric type " + metric_type;
            LOG_SERVER_ERROR_ << msg;
            return Status(SERVER_INVALID_ARGUMENT, msg);
        }
    } else {
        // float
        if (metric_type == knowhere::Metric::HAMMING || metric_type == knowhere::Metric::JACCARD ||
            metric_type == knowhere::Metric::TANIMOTO) {
            std::string msg = "Cannot search float entities with index metric type " + metric_type;
            LOG_SERVER_ERROR_ << msg;
            return Status(SERVER_INVALID_ARGUMENT, msg);
        }
    }

    return Status::OK();
}

Status
ValidateSearchTopk(int64_t top_k) {
    if (top_k <= 0 || top_k > QUERY_MAX_TOPK) {
        std::string msg =
            "Invalid topk: " + std::to_string(top_k) + ". " + "The topk must be within the range of 1 ~ 16384.";
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_TOPK, msg);
    }

    return Status::OK();
}

Status
ValidatePartitionTag(std::string& partition_tag, bool strict_check) {
    // trim side-blank of tag, only compare valid characters
    // for example: " ab cd " is treated as "ab cd"
    StringHelpFunctions::TrimStringBlank(partition_tag);

    std::string invalid_msg = "Invalid partition tag: " + partition_tag + ". ";
    if (partition_tag.empty()) {
        std::string msg = invalid_msg + "Partition tag should not be empty.";
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_PARTITION_TAG, msg);
    }

    // max length of partition tag
    if (partition_tag.length() > engine::MAX_NAME_LENGTH) {
        std::string msg = invalid_msg + "The length of a partition tag must be less than " +
                          std::to_string(engine::MAX_NAME_LENGTH) + " characters.";
        LOG_SERVER_ERROR_ << msg;
        return Status(SERVER_INVALID_PARTITION_TAG, msg);
    }

    // for CreatePartition, a partition tag must be numbers, letters, and underscores
    // but for Search/HasPartition, user can check partition by regex pattern, such as p_(.*) match p_1, p_50...
    if (strict_check) {
        // Partition tag first character should be underscore or character.
        char first_char = partition_tag[0];
        if (first_char != '_' && std::isalnum(first_char) == 0) {
            std::string msg = invalid_msg + "The first character of a partition tag must be an underscore or letter.";
            LOG_SERVER_ERROR_ << msg;
            return Status(SERVER_INVALID_PARTITION_TAG, msg);
        }

        int64_t tag_size = partition_tag.size();
        for (int64_t i = 1; i < tag_size; ++i) {
            char name_char = partition_tag[i];
            if (name_char != '_' && name_char != '$' && std::isalnum(name_char) == 0) {
                std::string msg = invalid_msg + "Partition tag can only contain numbers, letters, and underscores.";
                LOG_SERVER_ERROR_ << msg;
                return Status(SERVER_INVALID_PARTITION_TAG, msg);
            }
        }
    }

    return Status::OK();
}

Status
ValidatePartitionTags(std::vector<std::string>& partition_tags, bool strict_check) {
    std::vector<std::string> valid_tags;
    for (std::string& tag : partition_tags) {
        auto status = ValidatePartitionTag(tag, strict_check);
        if (!status.ok()) {
            return status;
        }
        valid_tags.emplace_back(tag);
    }
    partition_tags.swap(valid_tags);

    return Status::OK();
}

Status
ValidateInsertDataSize(const InsertParam& insert_param) {
    int64_t chunk_size = 0;
    for (auto& pair : insert_param.fields_data_) {
        for (auto& data : pair.second) {
            chunk_size += data.second;
        }
    }

    if (chunk_size > engine::MAX_INSERT_DATA_SIZE) {
        std::string msg = "The amount of data inserted each time cannot exceed " +
                          std::to_string(engine::MAX_INSERT_DATA_SIZE / engine::MB) + " MB";
        return Status(SERVER_INVALID_ROWRECORD_ARRAY, msg);
    }

    return Status::OK();
}

Status
ValidateCompactThreshold(double threshold) {
    if (threshold > 1.0 || threshold < 0.0) {
        std::string msg = "Invalid compact threshold: " + std::to_string(threshold) + ". Should be in range [0.0, 1.0]";
        return Status(SERVER_INVALID_ROWRECORD_ARRAY, msg);
    }

    return Status::OK();
}

}  // namespace server
}  // namespace milvus
