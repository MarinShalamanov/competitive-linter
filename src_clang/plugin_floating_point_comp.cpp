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
    if (const BinaryOperator *comparison =
          result.Nodes.getNodeAs<BinaryOperator>(EQUALS_OP_BINDING)) {
      auto& diagnostics = result.Context->getDiagnostics();
      const unsigned ID = diagnostics.getCustomDiagID(
        clang::DiagnosticsEngine::Error,
        "This floating point operation can lead to errors.");  
      auto builder = diagnostics.Report(comparison->getOperatorLoc(), ID);
      auto locEnd = Lexer::getLocForEndOfToken(comparison->getLocEnd(), 
                                               0, 
                                               *result.SourceManager, 
                                               LangOptions());
      SourceRange sourceRange(comparison->getLocStart(), locEnd);
      auto hintStr = getFloatEqualsReplecementHint(*comparison);
      const auto hint = FixItHint::CreateReplacement(sourceRange, hintStr);
      builder.AddFixItHint(hint); 
    }
  }
} equalsHandler;

MatchFinder finder; 

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
    X("floating_point_comp", "check for goto statements")   ;
