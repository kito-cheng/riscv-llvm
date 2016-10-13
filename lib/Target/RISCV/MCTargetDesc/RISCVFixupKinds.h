//===-- RISCVFixupKinds.h - RISCV Specific Fixup Entries --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCV_MCTARGETDESC_RISCVFIXUPKINDS_H
#define LLVM_LIB_TARGET_RISCV_MCTARGETDESC_RISCVFIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

#undef RISCV

namespace llvm {
namespace RISCV {
enum Fixups {
  // fixup_riscv_hi20 - 20-bit fixup corresponding to hi(foo) for
  // instructions like lui
  fixup_riscv_hi20 = FirstTargetFixupKind,
  // fixup_riscv_lo12_i - 12-bit fixup corresponding to lo(foo) for
  // instructions like addi
  fixup_riscv_lo12_i,
  // fixup_riscv_lo12_s - 12-bit fixup corresponding to lo(foo) for
  // the S-type store instructions
  fixup_riscv_lo12_s,
  // fixup_riscv_pcrel_hi20 - 20-bit fixup corresponding to pcrel_hi(foo) for
  // instructions like auipc
  fixup_riscv_pcrel_hi20,

  // Marker
  LastTargetFixupKind,
  NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};
} // end namespace RISCV
} // end namespace llvm

#endif
