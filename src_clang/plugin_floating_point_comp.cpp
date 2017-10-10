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
};

class CheckGotoAction : public  PluginASTAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) {
    equalsHandler = llvm::make_unique<EqualsHandler>();
    
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
      equalsHandler.get()
    );
    finder.addMatcher(
      binaryOperator(
        isExpansionInMainFile(),
        hasOperatorName(equalsSign), 
        hasLHS(hasType(asString(floatType)))
      ).bind(EQUALS_OP_BINDING),
      equalsHandler.get()
    );
    
    return finder.newASTConsumer();
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) {
    for (unsigned i = 0, e = args.size(); i != e; ++i) {
      llvm::errs() << "Goto arg = " << args[i] << "\n";

      // Example error handling.
      if (args[i] == "-an-error") {
        DiagnosticsEngine &D = CI.getDiagnostics();
        unsigned DiagID = D.getCustomDiagID(DiagnosticsEngine::Error,
                                            "invalid argument '%0'");
        D.Report(DiagID) << args[i];
        return false;
      }
    }
    if (args.size() && args[0] == "help")
      PrintHelp(llvm::errs());

    return true;
  }
  void PrintHelp(llvm::raw_ostream &ros) {
    ros << "Reports errors if a goto statement is found.\n";
  }

private:
  MatchFinder finder; 
  std::unique_ptr<EqualsHandler> equalsHandler;
};
  
}

static FrontendPluginRegistry::Add<CheckGotoAction>
    X("floating_point_comp", "check for goto statements");
