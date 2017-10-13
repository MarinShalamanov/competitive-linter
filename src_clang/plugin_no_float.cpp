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

constexpr char VAR_DECL_BINDING[] = "vardecl";
constexpr char FUNC_DECL_BINDING[] = "funcdecl";
    
class DeclHandler : public MatchFinder::MatchCallback {
public:
  DeclHandler() {}

  virtual void run(const MatchFinder::MatchResult &result) { 
    auto& diagnostics = result.Context->getDiagnostics();
    const unsigned ID = diagnostics.getCustomDiagID(
      clang::DiagnosticsEngine::Error,
      "Use double for better precision.");  
    
    if (const VarDecl *variableDecl =
          result.Nodes.getNodeAs<VarDecl>(VAR_DECL_BINDING)) {  
      auto builder = diagnostics.Report(variableDecl->getLocStart(), ID);
      SourceRange sourceRange = 
        variableDecl->getTypeSourceInfo()->getTypeLoc().getSourceRange();   
      const auto hint = FixItHint::CreateReplacement(sourceRange, "double");
      builder.AddFixItHint(hint); 
    }
    
    if (const FunctionDecl *funcDecl =
          result.Nodes.getNodeAs<FunctionDecl>(FUNC_DECL_BINDING)) {  
      auto builder = diagnostics.Report(funcDecl->getLocStart(), ID);
      const auto hint = 
        FixItHint::CreateReplacement(funcDecl->getReturnTypeSourceRange(), "double");
      builder.AddFixItHint(hint); 
    }
  }
};

class MatchFinderASTConsumer : public ASTConsumer {
public:
  MatchFinderASTConsumer() {
    constexpr char floatType[] = "float";
    
    finder.addMatcher(
      varDecl(
        isExpansionInMainFile(), 
        hasType(asString(floatType))
      ).bind(VAR_DECL_BINDING),
      &callback
    );
    finder.addMatcher(
      functionDecl(
        isExpansionInMainFile(),
        returns(asString(floatType))
      ).bind(FUNC_DECL_BINDING),
      &callback
    );
  }
    
  void HandleTranslationUnit(ASTContext &context) override {
    finder.matchAST(context);
  }
  
private:
  MatchFinder finder;
  DeclHandler callback;
  std::unique_ptr<ASTConsumer> innerConsumer;
};
  
class CheckGotoAction : public  PluginASTAction {
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

static FrontendPluginRegistry::Add<CheckGotoAction>
    X("no_float", "Suggests double type to be used instead of float.");
