//------------------------------------------------------------------------------
// Clang plugin to check if no goto statements are used.
//
// Once the .so is built, it can be loaded by Clang. For example:
//
// $ clang++ -fsyntax-only \
//      -Xclang -load -Xclang eli/llvm-clang-samples/build/plugin_goto.so \
//      -Xclang -plugin -Xclang goto test.cc
//------------------------------------------------------------------------------
#include <cstring>  

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include "llvm/Support/raw_ostream.h"

namespace {
  
using namespace clang;
using namespace ast_matchers;

constexpr char EQUALS_OP_BINDING[] = "floatingPointComp";
  
SmallString<128> getFloatEqualsReplecementHint(const BinaryOperator& comparison) {
  SmallString<128> str;
  llvm::raw_svector_ostream hintOS(str);
  LangOptions langOpts;
  langOpts.CPlusPlus = true;
  PrintingPolicy policy(langOpts);
  hintOS << "abs(";
  comparison.getLHS()->printPretty(hintOS, nullptr, policy);
  hintOS << " - (" ;
  comparison.getRHS()->printPretty(hintOS, nullptr, policy);
  hintOS << ")) < EPS";
  return str;
}
  
class EqualsHandler : public MatchFinder::MatchCallback {
public:
  EqualsHandler() {}

  virtual void run(const MatchFinder::MatchResult &result) { 
    
    auto& diagnostics = result.Context->getDiagnostics();
    const unsigned ID = diagnostics.getCustomDiagID(
      clang::DiagnosticsEngine::Error, "Here.");  
//    auto builder = diagnostics.Report(location, ID);
  }
};


/*
AST sample:

FunctionDecl <line:5:1, line:8:1> line:5:5 main 'int (void)'
  `-CompoundStmt <col:12, line:8:1>
    |-CallExpr <line:6:5, col:36> '_Bool'
    | |-ImplicitCastExpr <col:5, col:15> '_Bool (*)(_Bool)' <FunctionToPointerDecay>
    | | `-DeclRefExpr <col:5, col:15> '_Bool (_Bool)' lvalue CXXMethod 0x6add900 'sync_with_stdio' '_Bool (_Bool)'
    | `-CXXBoolLiteralExpr <col:31> '_Bool' false
    `-CXXMemberCallExpr <col:39, col:51> 'basic_ostream<char, struct std::char_traits<char> > *'
      |-MemberExpr <col:39, col:43> '<bound member function type>' .tie 0x6c0a100
      | `-ImplicitCastExpr <col:39> 'class std::basic_ios<char>' lvalue <UncheckedDerivedToBase (virtual basic_ios)>
      |   `-DeclRefExpr <col:39> 'std::istream':'class std::basic_istream<char>' lvalue Var 0x6cae3f8 'cin' 'std::istream':'class std::basic_istream<char>'
      `-ImplicitCastExpr </opt/compiler-explorer/clang-5.0.0/lib/clang/5.0.0/include/stddef.h:100:18> 'basic_ostream<char, struct std::char_traits<char> > *' <NullToPointer>
        `-GNUNullExpr <col:18> 'long'
*/
  
class MatchFinderASTConsumer : public ASTConsumer {
public:
  MatchFinderASTConsumer() {
      innerConsumer = finder.newASTConsumer();
    
      constexpr char floatType[] = "float";
      constexpr char doubleType[] = "double";
      constexpr char equalsSign[] = "==";

      // Match '==' operator with float/double LHS. We check only
      // the LHS because both sides of '==' will be implicitly casted
      // to the same type.
      finder.addMatcher(
        binaryOperator(
          isExpansionInMainFile(),
          hasOperatorName(equalsSign), 
          hasLHS(hasType(asString(doubleType)))
        ).bind(EQUALS_OP_BINDING),
        &equalsHandler
      );
      finder.addMatcher(
        binaryOperator(
          isExpansionInMainFile(),
          hasOperatorName(equalsSign), 
          hasLHS(hasType(asString(floatType)))
        ).bind(EQUALS_OP_BINDING),
        &equalsHandler
      );
  }
    
  void HandleTranslationUnit(ASTContext &context) override {
    finder.matchAST(context);
  }
  
private:
  MatchFinder finder;
  EqualsHandler equalsHandler;
  std::unique_ptr<ASTConsumer> innerConsumer;
};
  
class FloatingPointsCompAction : public  PluginASTAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(
    CompilerInstance &CI, llvm::StringRef) override {
    return llvm::make_unique<MatchFinderASTConsumer>();
  }
  
  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  } 
};
  
}

static FrontendPluginRegistry::Add<FloatingPointsCompAction>
    X("fast_cin", "Checks if the commands for the fast cin/cout are in the code.")   ;
