//===--- SmartContractHeaderCheck.cpp - clang-tidy ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"

#include "../ClangTidyCheck.h"
#include "SmartContractHeaderCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <vector>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {
namespace {
class IncludeModernizePPCallbacks : public PPCallbacks {
public:
  explicit IncludeModernizePPCallbacks(ClangTidyCheck &Check,
                                       LangOptions LangOpts);

  void InclusionDirective(SourceLocation HashLoc, const Token &IncludeTok,
                          StringRef FileName, bool IsAngled,
                          CharSourceRange FilenameRange, const FileEntry *File,
                          StringRef SearchPath, StringRef RelativePath,
                          const Module *Imported,
                          SrcMgr::CharacteristicKind FileType) override;

private:
  ClangTidyCheck &Check;
  LangOptions LangOpts;
  llvm::StringMap<std::string> CStyledHeaderToCxx;
  llvm::StringSet<> DeleteHeaders;
};
} // namespace

void SmartContractHeaderCheck::registerPPCallbacks(
    const SourceManager &SM, Preprocessor *PP, Preprocessor *ModuleExpanderPP) {
  PP->addPPCallbacks(
      ::std::make_unique<IncludeModernizePPCallbacks>(*this, getLangOpts()));
}


IncludeModernizePPCallbacks::IncludeModernizePPCallbacks(ClangTidyCheck &Check,
                                                         LangOptions LangOpts)
    : Check(Check), LangOpts(LangOpts) {
  for (const auto &KeyValue :
       std::vector<std::pair<llvm::StringRef, std::string>>(
           {{"assert.h", "cassert"},
            {"complex.h", "complex"},
            {"ctype.h", "cctype"},
            {"errno.h", "cerrno"},
            {"float.h", "cfloat"},
            {"limits.h", "climits"},
            {"locale.h", "clocale"},
            {"math.h", "cmath"},
            {"setjmp.h", "csetjmp"},
            {"signal.h", "csignal"},
            {"stdarg.h", "cstdarg"},
            {"stddef.h", "cstddef"},
            {"stdio.h", "cstdio"},
            {"stdlib.h", "cstdlib"},
            {"string.h", "cstring"},
            {"time.h", "ctime"},
            {"wchar.h", "cwchar"},
            {"wctype.h", "cwctype"}})) {
    CStyledHeaderToCxx.insert(KeyValue);
  }
  // Add C++ 11 headers.
  if (LangOpts.CPlusPlus11) {
    for (const auto &KeyValue :
         std::vector<std::pair<llvm::StringRef, std::string>>(
             {{"fenv.h", "cfenv"},
              {"stdint.h", "cstdint"},
              {"inttypes.h", "cinttypes"},
              {"tgmath.h", "ctgmath"},
              {"uchar.h", "cuchar"}})) {
      CStyledHeaderToCxx.insert(KeyValue);
    }
  }
  for (const auto &Key :
       std::vector<std::string>({"stdalign.h", "stdbool.h", "iso646.h"})) {
    DeleteHeaders.insert(Key);
  }
}

void IncludeModernizePPCallbacks::InclusionDirective(
    SourceLocation HashLoc, const Token &IncludeTok, StringRef FileName,
    bool IsAngled, CharSourceRange FilenameRange, const FileEntry *File,
    StringRef SearchPath, StringRef RelativePath, const Module *Imported,
    SrcMgr::CharacteristicKind FileType) {
      if (CStyledHeaderToCxx.count(FileName) != 0) {
            std::string Replacement =
        (llvm::Twine("<") + CStyledHeaderToCxx[FileName] + ">").str();
    Check.diag(FilenameRange.getBegin(), "inclusion of invalid header "
                                         "'%0',which results in undefined behaviour in smart contract, consider removing it")
        << FileName << CStyledHeaderToCxx[FileName];
    }
}


} // namespace misc
} // namespace tidy
} // namespace clang

