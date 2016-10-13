//===-- RISCVAsmBackend.cpp - RISCV Assembler Backend ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/RISCVFixupKinds.h"
#include "MCTargetDesc/RISCVMCTargetDesc.h"
#include "llvm/ADT/APInt.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
class RISCVAsmBackend : public MCAsmBackend {
  uint8_t OSABI;
  bool Is64Bit;

public:
  RISCVAsmBackend(uint8_t OSABI, bool Is64Bit)
      : MCAsmBackend(), OSABI(OSABI), Is64Bit(Is64Bit) {}
  ~RISCVAsmBackend() override {}

  void applyFixup(const MCFixup &Fixup, char *Data, unsigned DataSize,
                  uint64_t Value, bool IsPCRel) const override;

  MCObjectWriter *createObjectWriter(raw_pwrite_stream &OS) const override;

  bool fixupNeedsRelaxation(const MCFixup &Fixup, uint64_t Value,
                            const MCRelaxableFragment *DF,
                            const MCAsmLayout &Layout) const override {
    return false;
  }

  unsigned getNumFixupKinds() const override {
    return RISCV::NumTargetFixupKinds;
  }

  const MCFixupKindInfo &getFixupKindInfo(MCFixupKind Kind) const override {
    const static MCFixupKindInfo Infos[RISCV::NumTargetFixupKinds] = {
      // This table *must* be in the order that the fixup_* kinds are defined in
      // RISCVFixupKinds.h.
      //
      // name                    offset bits  flags
      { "fixup_riscv_hi20",       12,     20,  0 },
      { "fixup_riscv_lo12_i",     20,     12,  0 },
      { "fixup_riscv_lo12_s",      0,     32,  0 },
      { "fixup_riscv_pcrel_hi20", 12,     20,  MCFixupKindInfo::FKF_IsPCRel }
    };


    if (Kind < FirstTargetFixupKind)
      return MCAsmBackend::getFixupKindInfo(Kind);

    assert(unsigned(Kind - FirstTargetFixupKind) < getNumFixupKinds() &&
           "Invalid kind!");
    return Infos[Kind - FirstTargetFixupKind];
  }

  bool mayNeedRelaxation(const MCInst &Inst) const override { return false; }

  void relaxInstruction(const MCInst &Inst, const MCSubtargetInfo &STI,
                        MCInst &Res) const override {

    llvm_unreachable("RISCVAsmBackend::relaxInstruction() unimplemented");
  }

  bool writeNopData(uint64_t Count, MCObjectWriter *OW) const override;
};

bool RISCVAsmBackend::writeNopData(uint64_t Count, MCObjectWriter *OW) const {
  // Once support for the compressed instruction set is added, we will be able
  // to conditionally support 16-bit NOPs
  if ((Count % 4) != 0)
    return false;

  // The canonical nop on RISC-V is addi x0, x0, 0
  for (uint64_t i = 0; i < Count; i += 4)
    OW->write32(0x13);

  return true;
}

static uint64_t adjustFixupValue(unsigned Kind, uint64_t Value) {
  switch (Kind) {
  default:
    llvm_unreachable("Unknown fixup kind!");
  case FK_Data_1:
  case FK_Data_2:
  case FK_Data_4:
  case FK_Data_8:
    return Value;
  case RISCV::fixup_riscv_lo12_i:
    return Value & 0xfff;
  case RISCV::fixup_riscv_lo12_s:
    return (((Value >> 5) & 0x7f) << 25) | ((Value & 0x1f) << 7);
  case RISCV::fixup_riscv_hi20:
  case RISCV::fixup_riscv_pcrel_hi20:
    // Add 1 if bit 11 is 1, to compensate for low 12 bits being negative.
    return ((Value + 0x800) >> 12) & 0xfffff;
  }
}

void RISCVAsmBackend::applyFixup(const MCFixup &Fixup, char *Data,
                                 unsigned DataSize, uint64_t Value,
                                 bool IsPCRel) const {
  MCFixupKind Kind = Fixup.getKind();
  unsigned NumBytes = (getFixupKindInfo(Kind).TargetSize + 7) / 8;
  if (!Value)
    return; // Doesn't change encoding.
  MCFixupKindInfo Info = getFixupKindInfo(Fixup.getKind());
  // Apply any target-specific value adjustments.
  Value = adjustFixupValue(Fixup.getKind(), Value);

  // Shift the value into position.
  Value <<= Info.TargetOffset;

  unsigned Offset = Fixup.getOffset();
  assert(Offset + NumBytes <= DataSize && "Invalid fixup offset!");

  // For each byte of the fragment that the fixup touches, mask in the
  // bits from the fixup value.
  for (unsigned i = 0; i != 4; ++i) {
    unsigned Idx = i;
    Data[Offset + i] |= uint8_t((Value >> (Idx * 8)) & 0xff);
  }
  return;
}

MCObjectWriter *
RISCVAsmBackend::createObjectWriter(raw_pwrite_stream &OS) const {
  return createRISCVELFObjectWriter(OS, OSABI, Is64Bit);
}

} // end anonymous namespace

MCAsmBackend *llvm::createRISCVAsmBackend(const Target &T,
                                          const MCRegisterInfo &MRI,
                                          const Triple &TT, StringRef CPU,
                                          const MCTargetOptions &Options) {
  uint8_t OSABI = MCELFObjectTargetWriter::getOSABI(TT.getOS());
  return new RISCVAsmBackend(OSABI, TT.isArch64Bit());
}
