//=-- InstrProf.cpp - Instrumented profiling format support -----------------=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains support for clang's instrumentation based PGO and
// coverage.
//
//===----------------------------------------------------------------------===//

#include "llvm/ProfileData/InstrProf.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Compression.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/LEB128.h"
#include "llvm/Support/ManagedStatic.h"

using namespace llvm;

namespace {
std::string getInstrProfErrString(instrprof_error Err) {
  switch (Err) {
  case instrprof_error::success:
    return "Success";
  case instrprof_error::eof:
    return "End of File";
  case instrprof_error::unrecognized_format:
    return "Unrecognized instrumentation profile encoding format";
  case instrprof_error::bad_magic:
    return "Invalid instrumentation profile data (bad magic)";
  case instrprof_error::bad_header:
    return "Invalid instrumentation profile data (file header is corrupt)";
  case instrprof_error::unsupported_version:
    return "Unsupported instrumentation profile format version";
  case instrprof_error::unsupported_hash_type:
    return "Unsupported instrumentation profile hash type";
  case instrprof_error::too_large:
    return "Too much profile data";
  case instrprof_error::truncated:
    return "Truncated profile data";
  case instrprof_error::malformed:
    return "Malformed instrumentation profile data";
  case instrprof_error::unknown_function:
    return "No profile data available for function";
  case instrprof_error::hash_mismatch:
    return "Function control flow change detected (hash mismatch)";
  case instrprof_error::count_mismatch:
    return "Function basic block count change detected (counter mismatch)";
  case instrprof_error::counter_overflow:
    return "Counter overflow";
  case instrprof_error::value_site_count_mismatch:
    return "Function value site count change detected (counter mismatch)";
  case instrprof_error::compress_failed:
    return "Failed to compress data (zlib)";
  case instrprof_error::uncompress_failed:
    return "Failed to uncompress data (zlib)";
  }
  llvm_unreachable("A value of instrprof_error has no message.");
}

// FIXME: This class is only here to support the transition to llvm::Error. It
// will be removed once this transition is complete. Clients should prefer to
// deal with the Error value directly, rather than converting to error_code.
class InstrProfErrorCategoryType : public std::error_category {
  const char *name() const LLVM_NOEXCEPT override { return "llvm.instrprof"; }
  std::string message(int IE) const override {
    return getInstrProfErrString(static_cast<instrprof_error>(IE));
  }
};
} // end anonymous namespace

static ManagedStatic<InstrProfErrorCategoryType> ErrorCategory;

const std::error_category &llvm::instrprof_category() {
  return *ErrorCategory;
}

namespace llvm {

void SoftInstrProfErrors::addError(instrprof_error IE) {
  if (IE == instrprof_error::success)
    return;

  if (FirstError == instrprof_error::success)
    FirstError = IE;

  switch (IE) {
  case instrprof_error::hash_mismatch:
    ++NumHashMismatches;
    break;
  case instrprof_error::count_mismatch:
    ++NumCountMismatches;
    break;
  case instrprof_error::counter_overflow:
    ++NumCounterOverflows;
    break;
  case instrprof_error::value_site_count_mismatch:
    ++NumValueSiteCountMismatches;
    break;
  default:
    llvm_unreachable("Not a soft error");
  }
}

std::string InstrProfError::message() const {
  return getInstrProfErrString(Err);
}

char InstrProfError::ID = 0;

std::string getPGOFuncName(StringRef RawFuncName,
                           GlobalValue::LinkageTypes Linkage,
                           StringRef FileName,
                           uint64_t Version LLVM_ATTRIBUTE_UNUSED) {
  return GlobalValue::getGlobalIdentifier(RawFuncName, Linkage, FileName);
}

// Return the PGOFuncName. This function has some special handling when called
// in LTO optimization. The following only applies when calling in LTO passes
// (when \c InLTO is true): LTO's internalization privatizes many global linkage
// symbols. This happens after value profile annotation, but those internal
// linkage functions should not have a source prefix.
// To differentiate compiler generated internal symbols from original ones,
// PGOFuncName meta data are created and attached to the original internal
// symbols in the value profile annotation step
// (PGOUseFunc::annotateIndirectCallSites). If a symbol does not have the meta
// data, its original linkage must be non-internal.
std::string getPGOFuncName(const Function &F, bool InLTO, uint64_t Version) {
  if (!InLTO)
    return getPGOFuncName(F.getName(), F.getLinkage(), F.getParent()->getName(),
                          Version);

  // In LTO mode (when InLTO is true), first check if there is a meta data.
  if (MDNode *MD = getPGOFuncNameMetadata(F)) {
    StringRef S = cast<MDString>(MD->getOperand(0))->getString();
    return S.str();
  }

  // If there is no meta data, the function must be a global before the value
  // profile annotation pass. Its current linkage may be internal if it is
  // internalized in LTO mode.
  return getPGOFuncName(F.getName(), GlobalValue::ExternalLinkage, "");
}

StringRef getFuncNameWithoutPrefix(StringRef PGOFuncName, StringRef FileName) {
  if (FileName.empty())
    return PGOFuncName;
  // Drop the file name including ':'. See also getPGOFuncName.
  if (PGOFuncName.startswith(FileName))
    PGOFuncName = PGOFuncName.drop_front(FileName.size() + 1);
  return PGOFuncName;
}

// \p FuncName is the string used as profile lookup key for the function. A
// symbol is created to hold the name. Return the legalized symbol name.
std::string getPGOFuncNameVarName(StringRef FuncName,
                                  GlobalValue::LinkageTypes Linkage) {
  std::string VarName = getInstrProfNameVarPrefix();
  VarName += FuncName;

  if (!GlobalValue::isLocalLinkage(Linkage))
    return VarName;

  // Now fix up illegal chars in local VarName that may upset the assembler.
  const char *InvalidChars = "-:<>/\"'";
  size_t found = VarName.find_first_of(InvalidChars);
  while (found != std::string::npos) {
    VarName[found] = '_';
    found = VarName.find_first_of(InvalidChars, found + 1);
  }
  return VarName;
}

GlobalVariable *createPGOFuncNameVar(Module &M,
                                     GlobalValue::LinkageTypes Linkage,
                                     StringRef PGOFuncName) {

  // We generally want to match the function's linkage, but available_externally
  // and extern_weak both have the wrong semantics, and anything that doesn't
  // need to link across compilation units doesn't need to be visible at all.
  if (Linkage == GlobalValue::ExternalWeakLinkage)
    Linkage = GlobalValue::LinkOnceAnyLinkage;
  else if (Linkage == GlobalValue::AvailableExternallyLinkage)
    Linkage = GlobalValue::LinkOnceODRLinkage;
  else if (Linkage == GlobalValue::InternalLinkage ||
           Linkage == GlobalValue::ExternalLinkage)
    Linkage = GlobalValue::PrivateLinkage;

  auto *Value =
      ConstantDataArray::getString(M.getContext(), PGOFuncName, false);
  auto FuncNameVar =
      new GlobalVariable(M, Value->getType(), true, Linkage, Value,
                         getPGOFuncNameVarName(PGOFuncName, Linkage));

  // Hide the symbol so that we correctly get a copy for each executable.
  if (!GlobalValue::isLocalLinkage(FuncNameVar->getLinkage()))
    FuncNameVar->setVisibility(GlobalValue::HiddenVisibility);

  return FuncNameVar;
}

GlobalVariable *createPGOFuncNameVar(Function &F, StringRef PGOFuncName) {
  return createPGOFuncNameVar(*F.getParent(), F.getLinkage(), PGOFuncName);
}

void InstrProfSymtab::create(Module &M, bool InLTO) {
  for (Function &F : M) {
    // Function may not have a name: like using asm("") to overwrite the name.
    // Ignore in this case.
    if (!F.hasName())
      continue;
    const std::string &PGOFuncName = getPGOFuncName(F, InLTO);
    addFuncName(PGOFuncName);
    MD5FuncMap.emplace_back(Function::getGUID(PGOFuncName), &F);
  }

  finalizeSymtab();
}

Error collectPGOFuncNameStrings(const std::vector<std::string> &NameStrs,
                                bool doCompression, std::string &Result) {
  assert(NameStrs.size() && "No name data to emit");

  uint8_t Header[16], *P = Header;
  std::string UncompressedNameStrings =
      join(NameStrs.begin(), NameStrs.end(), getInstrProfNameSeparator());

  assert(StringRef(UncompressedNameStrings)
                 .count(getInstrProfNameSeparator()) == (NameStrs.size() - 1) &&
         "PGO name is invalid (contains separator token)");

  unsigned EncLen = encodeULEB128(UncompressedNameStrings.length(), P);
  P += EncLen;

  auto WriteStringToResult = [&](size_t CompressedLen, StringRef InputStr) {
    EncLen = encodeULEB128(CompressedLen, P);
    P += EncLen;
    char *HeaderStr = reinterpret_cast<char *>(&Header[0]);
    unsigned HeaderLen = P - &Header[0];
    Result.append(HeaderStr, HeaderLen);
    Result += InputStr;
    return Error::success();
  };

  if (!doCompression) {
    return WriteStringToResult(0, UncompressedNameStrings);
  }

  SmallString<128> CompressedNameStrings;
  zlib::Status Success =
      zlib::compress(StringRef(UncompressedNameStrings), CompressedNameStrings,
                     zlib::BestSizeCompression);

  if (Success != zlib::StatusOK)
    return make_error<InstrProfError>(instrprof_error::compress_failed);

  return WriteStringToResult(CompressedNameStrings.size(),
                             CompressedNameStrings);
}

StringRef getPGOFuncNameVarInitializer(GlobalVariable *NameVar) {
  auto *Arr = cast<ConstantDataArray>(NameVar->getInitializer());
  StringRef NameStr =
      Arr->isCString() ? Arr->getAsCString() : Arr->getAsString();
  return NameStr;
}

Error collectPGOFuncNameStrings(const std::vector<GlobalVariable *> &NameVars,
                                std::string &Result, bool doCompression) {
  std::vector<std::string> NameStrs;
  for (auto *NameVar : NameVars) {
    NameStrs.push_back(getPGOFuncNameVarInitializer(NameVar));
  }
  return collectPGOFuncNameStrings(
      NameStrs, zlib::isAvailable() && doCompression, Result);
}

Error readPGOFuncNameStrings(StringRef NameStrings, InstrProfSymtab &Symtab) {
  const uint8_t *P = reinterpret_cast<const uint8_t *>(NameStrings.data());
  const uint8_t *EndP = reinterpret_cast<const uint8_t *>(NameStrings.data() +
                                                          NameStrings.size());
  while (P < EndP) {
    uint32_t N;
    uint64_t UncompressedSize = decodeULEB128(P, &N);
    P += N;
    uint64_t CompressedSize = decodeULEB128(P, &N);
    P += N;
    bool isCompressed = (CompressedSize != 0);
    SmallString<128> UncompressedNameStrings;
    StringRef NameStrings;
    if (isCompressed) {
      StringRef CompressedNameStrings(reinterpret_cast<const char *>(P),
                                      CompressedSize);
      if (zlib::uncompress(CompressedNameStrings, UncompressedNameStrings,
                           UncompressedSize) != zlib::StatusOK)
        return make_error<InstrProfError>(instrprof_error::uncompress_failed);
      P += CompressedSize;
      NameStrings = StringRef(UncompressedNameStrings.data(),
                              UncompressedNameStrings.size());
    } else {
      NameStrings =
          StringRef(reinterpret_cast<const char *>(P), UncompressedSize);
      P += UncompressedSize;
    }
    // Now parse the name strings.
    SmallVector<StringRef, 0> Names;
    NameStrings.split(Names, getInstrProfNameSeparator());
    for (StringRef &Name : Names)
      Symtab.addFuncName(Name);

    while (P < EndP && *P == 0)
      P++;
  }
  Symtab.finalizeSymtab();
  return Error::success();
}

void InstrProfValueSiteRecord::merge(SoftInstrProfErrors &SIPE,
                                     InstrProfValueSiteRecord &Input,
                                     uint64_t Weight) {
  this->sortByTargetValues();
  Input.sortByTargetValues();
  auto I = ValueData.begin();
  auto IE = ValueData.end();
  for (auto J = Input.ValueData.begin(), JE = Input.ValueData.end(); J != JE;
       ++J) {
    while (I != IE && I->Value < J->Value)
      ++I;
    if (I != IE && I->Value == J->Value) {
      bool Overflowed;
      I->Count = SaturatingMultiplyAdd(J->Count, Weight, I->Count, &Overflowed);
      if (Overflowed)
        SIPE.addError(instrprof_error::counter_overflow);
      ++I;
      continue;
    }
    ValueData.insert(I, *J);
  }
}

void InstrProfValueSiteRecord::scale(SoftInstrProfErrors &SIPE,
                                     uint64_t Weight) {
  for (auto I = ValueData.begin(), IE = ValueData.end(); I != IE; ++I) {
    bool Overflowed;
    I->Count = SaturatingMultiply(I->Count, Weight, &Overflowed);
    if (Overflowed)
      SIPE.addError(instrprof_error::counter_overflow);
  }
}

// Merge Value Profile data from Src record to this record for ValueKind.
// Scale merged value counts by \p Weight.
void InstrProfRecord::mergeValueProfData(uint32_t ValueKind,
                                         InstrProfRecord &Src,
                                         uint64_t Weight) {
  uint32_t ThisNumValueSites = getNumValueSites(ValueKind);
  uint32_t OtherNumValueSites = Src.getNumValueSites(ValueKind);
  if (ThisNumValueSites != OtherNumValueSites) {
    SIPE.addError(instrprof_error::value_site_count_mismatch);
    return;
  }
  std::vector<InstrProfValueSiteRecord> &ThisSiteRecords =
      getValueSitesForKind(ValueKind);
  std::vector<InstrProfValueSiteRecord> &OtherSiteRecords =
      Src.getValueSitesForKind(ValueKind);
  for (uint32_t I = 0; I < ThisNumValueSites; I++)
    ThisSiteRecords[I].merge(SIPE, OtherSiteRecords[I], Weight);
}

void InstrProfRecord::merge(InstrProfRecord &Other, uint64_t Weight) {
  // If the number of counters doesn't match we either have bad data
  // or a hash collision.
  if (Counts.size() != Other.Counts.size()) {
    SIPE.addError(instrprof_error::count_mismatch);
    return;
  }

  for (size_t I = 0, E = Other.Counts.size(); I < E; ++I) {
    bool Overflowed;
    Counts[I] =
        SaturatingMultiplyAdd(Other.Counts[I], Weight, Counts[I], &Overflowed);
    if (Overflowed)
      SIPE.addError(instrprof_error::counter_overflow);
  }

  for (uint32_t Kind = IPVK_First; Kind <= IPVK_Last; ++Kind)
    mergeValueProfData(Kind, Other, Weight);
}

void InstrProfRecord::scaleValueProfData(uint32_t ValueKind, uint64_t Weight) {
  uint32_t ThisNumValueSites = getNumValueSites(ValueKind);
  std::vector<InstrProfValueSiteRecord> &ThisSiteRecords =
      getValueSitesForKind(ValueKind);
  for (uint32_t I = 0; I < ThisNumValueSites; I++)
    ThisSiteRecords[I].scale(SIPE, Weight);
}

void InstrProfRecord::scale(uint64_t Weight) {
  for (auto &Count : this->Counts) {
    bool Overflowed;
    Count = SaturatingMultiply(Count, Weight, &Overflowed);
    if (Overflowed)
      SIPE.addError(instrprof_error::counter_overflow);
  }
  for (uint32_t Kind = IPVK_First; Kind <= IPVK_Last; ++Kind)
    scaleValueProfData(Kind, Weight);
}

// Map indirect call target name hash to name string.
uint64_t InstrProfRecord::remapValue(uint64_t Value, uint32_t ValueKind,
                                     ValueMapType *ValueMap) {
  if (!ValueMap)
    return Value;
  switch (ValueKind) {
  case IPVK_IndirectCallTarget: {
    auto Result =
        std::lower_bound(ValueMap->begin(), ValueMap->end(), Value,
                         [](const std::pair<uint64_t, uint64_t> &LHS,
                            uint64_t RHS) { return LHS.first < RHS; });
   // Raw function pointer collected by value profiler may be from 
   // external functions that are not instrumented. They won't have
   // mapping data to be used by the deserializer. Force the value to
   // be 0 in this case.
    if (Result != ValueMap->end() && Result->first == Value)
      Value = (uint64_t)Result->second;
    else
      Value = 0;
    break;
  }
  }
  return Value;
}

void InstrProfRecord::addValueData(uint32_t ValueKind, uint32_t Site,
                                   InstrProfValueData *VData, uint32_t N,
                                   ValueMapType *ValueMap) {
  for (uint32_t I = 0; I < N; I++) {
    VData[I].Value = remapValue(VData[I].Value, ValueKind, ValueMap);
  }
  std::vector<InstrProfValueSiteRecord> &ValueSites =
      getValueSitesForKind(ValueKind);
  if (N == 0)
    ValueSites.emplace_back();
  else
    ValueSites.emplace_back(VData, VData + N);
}

#define INSTR_PROF_COMMON_API_IMPL
#include "llvm/ProfileData/InstrProfData.inc"

/*!
 * \brief ValueProfRecordClosure Interface implementation for  InstrProfRecord
 *  class. These C wrappers are used as adaptors so that C++ code can be
 *  invoked as callbacks.
 */
uint32_t getNumValueKindsInstrProf(const void *Record) {
  return reinterpret_cast<const InstrProfRecord *>(Record)->getNumValueKinds();
}

uint32_t getNumValueSitesInstrProf(const void *Record, uint32_t VKind) {
  return reinterpret_cast<const InstrProfRecord *>(Record)
      ->getNumValueSites(VKind);
}

uint32_t getNumValueDataInstrProf(const void *Record, uint32_t VKind) {
  return reinterpret_cast<const InstrProfRecord *>(Record)
      ->getNumValueData(VKind);
}

uint32_t getNumValueDataForSiteInstrProf(const void *R, uint32_t VK,
                                         uint32_t S) {
  return reinterpret_cast<const InstrProfRecord *>(R)
      ->getNumValueDataForSite(VK, S);
}

void getValueForSiteInstrProf(const void *R, InstrProfValueData *Dst,
                              uint32_t K, uint32_t S) {
  reinterpret_cast<const InstrProfRecord *>(R)->getValueForSite(Dst, K, S);
}

ValueProfData *allocValueProfDataInstrProf(size_t TotalSizeInBytes) {
  ValueProfData *VD =
      (ValueProfData *)(new (::operator new(TotalSizeInBytes)) ValueProfData());
  memset(VD, 0, TotalSizeInBytes);
  return VD;
}

static ValueProfRecordClosure InstrProfRecordClosure = {
    nullptr,
    getNumValueKindsInstrProf,
    getNumValueSitesInstrProf,
    getNumValueDataInstrProf,
    getNumValueDataForSiteInstrProf,
    nullptr,
    getValueForSiteInstrProf,
    allocValueProfDataInstrProf};

// Wrapper implementation using the closure mechanism.
uint32_t ValueProfData::getSize(const InstrProfRecord &Record) {
  InstrProfRecordClosure.Record = &Record;
  return getValueProfDataSize(&InstrProfRecordClosure);
}

// Wrapper implementation using the closure mechanism.
std::unique_ptr<ValueProfData>
ValueProfData::serializeFrom(const InstrProfRecord &Record) {
  InstrProfRecordClosure.Record = &Record;

  std::unique_ptr<ValueProfData> VPD(
      serializeValueProfDataFrom(&InstrProfRecordClosure, nullptr));
  return VPD;
}

void ValueProfRecord::deserializeTo(InstrProfRecord &Record,
                                    InstrProfRecord::ValueMapType *VMap) {
  Record.reserveSites(Kind, NumValueSites);

  InstrProfValueData *ValueData = getValueProfRecordValueData(this);
  for (uint64_t VSite = 0; VSite < NumValueSites; ++VSite) {
    uint8_t ValueDataCount = this->SiteCountArray[VSite];
    Record.addValueData(Kind, VSite, ValueData, ValueDataCount, VMap);
    ValueData += ValueDataCount;
  }
}

// For writing/serializing,  Old is the host endianness, and  New is
// byte order intended on disk. For Reading/deserialization, Old
// is the on-disk source endianness, and New is the host endianness.
void ValueProfRecord::swapBytes(support::endianness Old,
                                support::endianness New) {
  using namespace support;
  if (Old == New)
    return;

  if (getHostEndianness() != Old) {
    sys::swapByteOrder<uint32_t>(NumValueSites);
    sys::swapByteOrder<uint32_t>(Kind);
  }
  uint32_t ND = getValueProfRecordNumValueData(this);
  InstrProfValueData *VD = getValueProfRecordValueData(this);

  // No need to swap byte array: SiteCountArrray.
  for (uint32_t I = 0; I < ND; I++) {
    sys::swapByteOrder<uint64_t>(VD[I].Value);
    sys::swapByteOrder<uint64_t>(VD[I].Count);
  }
  if (getHostEndianness() == Old) {
    sys::swapByteOrder<uint32_t>(NumValueSites);
    sys::swapByteOrder<uint32_t>(Kind);
  }
}

void ValueProfData::deserializeTo(InstrProfRecord &Record,
                                  InstrProfRecord::ValueMapType *VMap) {
  if (NumValueKinds == 0)
    return;

  ValueProfRecord *VR = getFirstValueProfRecord(this);
  for (uint32_t K = 0; K < NumValueKinds; K++) {
    VR->deserializeTo(Record, VMap);
    VR = getValueProfRecordNext(VR);
  }
}

template <class T>
static T swapToHostOrder(const unsigned char *&D, support::endianness Orig) {
  using namespace support;
  if (Orig == little)
    return endian::readNext<T, little, unaligned>(D);
  else
    return endian::readNext<T, big, unaligned>(D);
}

static std::unique_ptr<ValueProfData> allocValueProfData(uint32_t TotalSize) {
  return std::unique_ptr<ValueProfData>(new (::operator new(TotalSize))
                                            ValueProfData());
}

Error ValueProfData::checkIntegrity() {
  if (NumValueKinds > IPVK_Last + 1)
    return make_error<InstrProfError>(instrprof_error::malformed);
  // Total size needs to be mulltiple of quadword size.
  if (TotalSize % sizeof(uint64_t))
    return make_error<InstrProfError>(instrprof_error::malformed);

  ValueProfRecord *VR = getFirstValueProfRecord(this);
  for (uint32_t K = 0; K < this->NumValueKinds; K++) {
    if (VR->Kind > IPVK_Last)
      return make_error<InstrProfError>(instrprof_error::malformed);
    VR = getValueProfRecordNext(VR);
    if ((char *)VR - (char *)this > (ptrdiff_t)TotalSize)
      return make_error<InstrProfError>(instrprof_error::malformed);
  }
  return Error::success();
}

Expected<std::unique_ptr<ValueProfData>>
ValueProfData::getValueProfData(const unsigned char *D,
                                const unsigned char *const BufferEnd,
                                support::endianness Endianness) {
  using namespace support;
  if (D + sizeof(ValueProfData) > BufferEnd)
    return make_error<InstrProfError>(instrprof_error::truncated);

  const unsigned char *Header = D;
  uint32_t TotalSize = swapToHostOrder<uint32_t>(Header, Endianness);
  if (D + TotalSize > BufferEnd)
    return make_error<InstrProfError>(instrprof_error::too_large);

  std::unique_ptr<ValueProfData> VPD = allocValueProfData(TotalSize);
  memcpy(VPD.get(), D, TotalSize);
  // Byte swap.
  VPD->swapBytesToHost(Endianness);

  Error E = VPD->checkIntegrity();
  if (E)
    return std::move(E);

  return std::move(VPD);
}

void ValueProfData::swapBytesToHost(support::endianness Endianness) {
  using namespace support;
  if (Endianness == getHostEndianness())
    return;

  sys::swapByteOrder<uint32_t>(TotalSize);
  sys::swapByteOrder<uint32_t>(NumValueKinds);

  ValueProfRecord *VR = getFirstValueProfRecord(this);
  for (uint32_t K = 0; K < NumValueKinds; K++) {
    VR->swapBytes(Endianness, getHostEndianness());
    VR = getValueProfRecordNext(VR);
  }
}

void ValueProfData::swapBytesFromHost(support::endianness Endianness) {
  using namespace support;
  if (Endianness == getHostEndianness())
    return;

  ValueProfRecord *VR = getFirstValueProfRecord(this);
  for (uint32_t K = 0; K < NumValueKinds; K++) {
    ValueProfRecord *NVR = getValueProfRecordNext(VR);
    VR->swapBytes(getHostEndianness(), Endianness);
    VR = NVR;
  }
  sys::swapByteOrder<uint32_t>(TotalSize);
  sys::swapByteOrder<uint32_t>(NumValueKinds);
}

void annotateValueSite(Module &M, Instruction &Inst,
                       const InstrProfRecord &InstrProfR,
                       InstrProfValueKind ValueKind, uint32_t SiteIdx,
                       uint32_t MaxMDCount) {
  uint32_t NV = InstrProfR.getNumValueDataForSite(ValueKind, SiteIdx);
  if (!NV)
    return;

  uint64_t Sum = 0;
  std::unique_ptr<InstrProfValueData[]> VD =
      InstrProfR.getValueForSite(ValueKind, SiteIdx, &Sum);

  ArrayRef<InstrProfValueData> VDs(VD.get(), NV);
  annotateValueSite(M, Inst, VDs, Sum, ValueKind, MaxMDCount);
}

void annotateValueSite(Module &M, Instruction &Inst,
                       ArrayRef<InstrProfValueData> VDs,
                       uint64_t Sum, InstrProfValueKind ValueKind,
                       uint32_t MaxMDCount) {
  LLVMContext &Ctx = M.getContext();
  MDBuilder MDHelper(Ctx);
  SmallVector<Metadata *, 3> Vals;
  // Tag
  Vals.push_back(MDHelper.createString("VP"));
  // Value Kind
  Vals.push_back(MDHelper.createConstant(
      ConstantInt::get(Type::getInt32Ty(Ctx), ValueKind)));
  // Total Count
  Vals.push_back(
      MDHelper.createConstant(ConstantInt::get(Type::getInt64Ty(Ctx), Sum)));

  // Value Profile Data
  uint32_t MDCount = MaxMDCount;
  for (auto &VD : VDs) {
    Vals.push_back(MDHelper.createConstant(
        ConstantInt::get(Type::getInt64Ty(Ctx), VD.Value)));
    Vals.push_back(MDHelper.createConstant(
        ConstantInt::get(Type::getInt64Ty(Ctx), VD.Count)));
    if (--MDCount == 0)
      break;
  }
  Inst.setMetadata(LLVMContext::MD_prof, MDNode::get(Ctx, Vals));
}

bool getValueProfDataFromInst(const Instruction &Inst,
                              InstrProfValueKind ValueKind,
                              uint32_t MaxNumValueData,
                              InstrProfValueData ValueData[],
                              uint32_t &ActualNumValueData, uint64_t &TotalC) {
  MDNode *MD = Inst.getMetadata(LLVMContext::MD_prof);
  if (!MD)
    return false;

  unsigned NOps = MD->getNumOperands();

  if (NOps < 5)
    return false;

  // Operand 0 is a string tag "VP":
  MDString *Tag = cast<MDString>(MD->getOperand(0));
  if (!Tag)
    return false;

  if (!Tag->getString().equals("VP"))
    return false;

  // Now check kind:
  ConstantInt *KindInt = mdconst::dyn_extract<ConstantInt>(MD->getOperand(1));
  if (!KindInt)
    return false;
  if (KindInt->getZExtValue() != ValueKind)
    return false;

  // Get total count
  ConstantInt *TotalCInt = mdconst::dyn_extract<ConstantInt>(MD->getOperand(2));
  if (!TotalCInt)
    return false;
  TotalC = TotalCInt->getZExtValue();

  ActualNumValueData = 0;

  for (unsigned I = 3; I < NOps; I += 2) {
    if (ActualNumValueData >= MaxNumValueData)
      break;
    ConstantInt *Value = mdconst::dyn_extract<ConstantInt>(MD->getOperand(I));
    ConstantInt *Count =
        mdconst::dyn_extract<ConstantInt>(MD->getOperand(I + 1));
    if (!Value || !Count)
      return false;
    ValueData[ActualNumValueData].Value = Value->getZExtValue();
    ValueData[ActualNumValueData].Count = Count->getZExtValue();
    ActualNumValueData++;
  }
  return true;
}

MDNode *getPGOFuncNameMetadata(const Function &F) {
  return F.getMetadata(getPGOFuncNameMetadataName());
}

void createPGOFuncNameMetadata(Function &F, StringRef PGOFuncName) {
  // Only for internal linkage functions.
  if (PGOFuncName == F.getName())
      return;
  // Don't create duplicated meta-data.
  if (getPGOFuncNameMetadata(F))
    return;
  LLVMContext &C = F.getContext();
  MDNode *N = MDNode::get(C, MDString::get(C, PGOFuncName));
  F.setMetadata(getPGOFuncNameMetadataName(), N);
}

} // end namespace llvm
