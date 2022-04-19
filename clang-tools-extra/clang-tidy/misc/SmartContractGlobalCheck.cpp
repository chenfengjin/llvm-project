//===--- SmartContractGlobalCheck.cpp - clang-tidy ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SmartContractGlobalCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {
namespace{
  AST_MATCHER(VarDecl, isLocalVariable) { return Node.isLocalVarDecl(); }

}
void SmartContractGlobalCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      varDecl(hasGlobalStorage(), unless(hasType(isConstQualified())),
              unless(isLocalVariable()), 
              unless(matchesName("::g[A-Z]")))
          .bind("global_var"),
      this);
  Finder->addMatcher(varDecl(hasGlobalStorage(), hasType(isConstQualified()),
                             unless(isLocalVariable()),
                             unless(matchesName("::(k[A-Z])|([A-Z][A-Z0-9])")))
                         .bind("global_const"),
                     this);
}

void SmartContractGlobalCheck::check(const MatchFinder::MatchResult &Result) {
  if (const auto *Decl = Result.Nodes.getNodeAs<VarDecl>("global_var")) {
    if (Decl->isStaticDataMember())
      return;
    diag(Decl->getLocation(),
         "non-const global variable '%0' must have a name which starts with "
         "'g[A-Z]'")
        << Decl->getName();// << generateFixItHint(Decl, false);
  }
  if (const auto *Decl = Result.Nodes.getNodeAs<VarDecl>("global_const")) {
    if (Decl->isStaticDataMember())
      return;
    diag(Decl->getLocation(),
         "const global variable '%0' must have a name which starts with "
         "an appropriate prefix")
        << Decl->getName() ;//<< generateFixItHint(Decl, true);
  }
}

} // namespace misc
} // namespace tidy
} // namespace clang
