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

#include "source/fuzz/fuzzer_pass_duplicate_regions_with_selections.h"
#include "source/fuzz/fuzzer_util.h"
#include "source/fuzz/transformation_duplicate_region_with_selection.h"

namespace spvtools {
namespace fuzz {

FuzzerPassDuplicateRegionsWithSelections::
    FuzzerPassDuplicateRegionsWithSelections(
        opt::IRContext* ir_context,
        TransformationContext* transformation_context,
        FuzzerContext* fuzzer_context,
        protobufs::TransformationSequence* transformations)
    : FuzzerPass(ir_context, transformation_context, fuzzer_context,
                 transformations) {}

FuzzerPassDuplicateRegionsWithSelections::
    ~FuzzerPassDuplicateRegionsWithSelections() = default;

void FuzzerPassDuplicateRegionsWithSelections::Apply() {
  std::vector<opt::Function*> original_functions;
  for (auto& function : *GetIRContext()->module()) {
    original_functions.push_back(&function);
  }
  // Iterate over all of the functions in the module.
  for (auto& function : original_functions) {
    // Randomly decide whether to apply the transformation.
    if (!GetFuzzerContext()->ChoosePercentage(
            GetFuzzerContext()->GetChanceOfDuplicatingRegionWithSelection())) {
      continue;
    }
    std::vector<opt::BasicBlock*> start_blocks;
    for (auto& block : *function) {
      // We currently don't consider the first block to be the starting block.
      if (&block == &*function->begin()) {
        continue;
      }
      start_blocks.push_back(&block);
    }
    if (start_blocks.empty()) {
      continue;
    }
    // Randomly choose the entry block.
    auto entry_block =
        start_blocks[GetFuzzerContext()->RandomIndex(start_blocks)];
    auto dominator_analysis = GetIRContext()->GetDominatorAnalysis(function);
    auto postdominator_analysis =
        GetIRContext()->GetPostDominatorAnalysis(function);
    std::vector<opt::BasicBlock*> candidate_exit_blocks;
    for (auto postdominates_entry_block = entry_block;
         postdominates_entry_block != nullptr;
         postdominates_entry_block = postdominator_analysis->ImmediateDominator(
             postdominates_entry_block)) {
      // The candidate exit block must be dominated by the entry block and the
      // entry block must be post-dominated by the candidate exit block. Ignore
      // the block if it heads a selection construct or a loop construct.
      if (dominator_analysis->Dominates(entry_block,
                                        postdominates_entry_block) &&
          !postdominates_entry_block->GetLoopMergeInst()) {
        candidate_exit_blocks.push_back(postdominates_entry_block);
      }
    }
    if (candidate_exit_blocks.empty()) {
      continue;
    }
    // Randomly choose the exit block.
    auto exit_block = candidate_exit_blocks[GetFuzzerContext()->RandomIndex(
        candidate_exit_blocks)];

    auto region_blocks =
        TransformationDuplicateRegionWithSelection::GetRegionBlocks(
            GetIRContext(), entry_block, exit_block);

    // Construct |original_label_to_duplicate_label| by iterating over all block
    // in the region. Construct |original_id_to_duplicate_id| and
    // |original_id_to_phi_id| by iterating over all instructions in each block.
    std::map<uint32_t, uint32_t> original_label_to_duplicate_label;
    std::map<uint32_t, uint32_t> original_id_to_duplicate_id;
    std::map<uint32_t, uint32_t> original_id_to_phi_id;
    for (auto& block : region_blocks) {
      original_label_to_duplicate_label[block->id()] =
          GetFuzzerContext()->GetFreshId();
      for (auto& instr : *block) {
        if (instr.result_id()) {
          original_id_to_duplicate_id[instr.result_id()] =
              GetFuzzerContext()->GetFreshId();
          if ((&instr == &*exit_block->tail() ||
               fuzzerutil::IdIsAvailableBeforeInstruction(
                   GetIRContext(), &*exit_block->tail(), instr.result_id()))) {
            original_id_to_phi_id[instr.result_id()] =
                GetFuzzerContext()->GetFreshId();
          }
        }
      }
    }
    // Randomly decide between value "true" or "false"
    auto condition_value = GetFuzzerContext()->ChooseEven();

    // Make sure the transformation has access to a bool constant to be used
    // while creating conditional construct.
    auto condition_id = FindOrCreateBoolConstant(condition_value, true);
    TransformationDuplicateRegionWithSelection transformation =
        TransformationDuplicateRegionWithSelection(
            GetFuzzerContext()->GetFreshId(), condition_id,
            GetFuzzerContext()->GetFreshId(), entry_block->id(),
            exit_block->id(), std::move(original_label_to_duplicate_label),
            std::move(original_id_to_duplicate_id),
            std::move(original_id_to_phi_id));
    MaybeApplyTransformation(transformation);
  }
}
}  // namespace fuzz
}  // namespace spvtools
