// Copyright (c) 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SPIRV_TOOLS_TRANSFORMATION_REPLACE_COPY_OBJECT_WITH_STORE_LOAD_H
#define SPIRV_TOOLS_TRANSFORMATION_REPLACE_COPY_OBJECT_WITH_STORE_LOAD_H

#include "source/fuzz/protobufs/spirvfuzz_protobufs.h"
#include "source/fuzz/transformation.h"
#include "source/fuzz/transformation_context.h"
#include "source/opt/ir_context.h"

namespace spvtools {
namespace fuzz {

class TransformationReplaceCopyObjectWithStoreLoad : public Transformation {
 public:
  explicit TransformationReplaceCopyObjectWithStoreLoad(
      const protobufs::TransformationReplaceCopyObjectWithStoreLoad& message);

  TransformationReplaceCopyObjectWithStoreLoad(
      uint32_t copy_object_result_id, uint32_t fresh_variable_id,
      uint32_t variable_storage_class, uint32_t variable_initializer_id);

  // - |message_.copy_object_result_id| must be a result id of an instruction
  //   OpStoreLoad.
  // - |message_.fresh_variable_id| must be a fresh id given to variable used by
  //   OpStore
  // - |message_.variable_storage_class| must be either StorageClassPrivate or
  //   StorageClassFunction
  // - |message_.initializer_id| must be a result id of some constant in the
  //   module. Its type must be equal to the pointee type of the variable that
  //   will be created.

  bool IsApplicable(
      opt::IRContext* ir_context,
      const TransformationContext& transformation_context) const override;

  // Stores |value_id| to |variable_id|, loads |variable_id| to
  // |value_synonym_id| and adds the fact that |value_synonym_id| and |value_id|
  // are synonymous.
  void Apply(opt::IRContext* ir_context,
             TransformationContext* transformation_context) const override;

  protobufs::Transformation ToMessage() const override;

 private:
  protobufs::TransformationReplaceCopyObjectWithStoreLoad message_;
};

}  // namespace fuzz
}  // namespace spvtools

#endif  // SPIRV_TOOLS_TRANSFORMATION_REPLACE_COPY_OBJECT_WITH_STORE_LOAD_H
