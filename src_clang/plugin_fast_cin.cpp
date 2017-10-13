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

constexpr char MAIN_FUNCTION_BINDING[] = "main";

SmallString<128> exprToString(const Expr* expr) {
  SmallString<128> str;
  llvm::raw_svector_ostream hintOS(str);
  LangOptions langOpts;
  langOpts.CPlusPlus = true;
  PrintingPolicy policy(langOpts);
  expr->printPretty(hintOS, nullptr, policy);
  return str;
}
  
class MainFunctionHandler : public MatchFinder::MatchCallback {
public:
  MainFunctionHandler() {}

  virtual void run(const MatchFinder::MatchResult &result) { 
    if (const FunctionDecl * mainFunc =
          result.Nodes.getNodeAs<FunctionDecl>(MAIN_FUNCTION_BINDING)) {
      context = result.Context;
      diagnostics = &(context->getDiagnostics());
      sourceManager = result.SourceManager;
      
      firstStmtMissingID = diagnostics->getCustomDiagID(
              clang::DiagnosticsEngine::Error, 
              "Enable fast input/output. "
              "ios_base::sync_with_stdio(false); cin.tie(NULL);");
      
      secondStmtMissingID = diagnostics->getCustomDiagID(
            clang::DiagnosticsEngine::Error, 
            "Add cin.tie(NULL);");
      
      CompoundStmt* body = static_cast<CompoundStmt*>(mainFunc->getBody());
      auto it = body->body_begin();

      if (body->size() < 2) {
        diagnostics->Report(body->getLocStart(), firstStmtMissingID);
        return;
      }
      
      handleFirstStmt(*it) && handleSecondStmt(*(++it));
    }
  }
  
private:
  ASTContext* context; 
  DiagnosticsEngine* diagnostics; 
  SourceManager* sourceManager;
  unsigned firstStmtMissingID;
  unsigned secondStmtMissingID;
  
  bool handleFirstStmt(Stmt* first) {
    if (CallExpr* firstStmt = dyn_cast<CallExpr>(first)) {
      auto name = firstStmt->getDirectCallee()->getNameInfo().getName().getAsString();

      if (name != "sync_with_stdio") {
        diagnostics->Report(firstStmt->getLocStart(), firstStmtMissingID);
        return false;
      }

      Expr* arg = firstStmt->getArg(0);
      bool argumentEvaluation;
      if (!arg->EvaluateAsBooleanCondition(argumentEvaluation, *context) 
          || argumentEvaluation) {
        const unsigned ID = diagnostics->getCustomDiagID(
          clang::DiagnosticsEngine::Error, "Sync with stdio shuold be turned off.");

        SourceLocation loc = arg->getLocStart();
        if (!sourceManager->isInMainFile(loc)) {
          loc = firstStmt->getLocStart(); 
        }
        diagnostics->Report(loc, ID);
        return false;
      }
    } else {
      diagnostics->Report(first->getLocStart(), firstStmtMissingID);
      return false;
    }
    return true;
  }
  
  bool handleSecondStmt(Stmt* second) {
    if (CXXMemberCallExpr* secStmt = dyn_cast<CXXMemberCallExpr>(second)) {
      auto obj = exprToString(secStmt->getImplicitObjectArgument());
      auto method = secStmt->getMethodDecl()->getName();

      if (obj != "cin" || method != "tie" || secStmt->getNumArgs() != 1) {
        diagnostics->Report(second->getLocStart(), secondStmtMissingID);
        return false;
      }
      // TODO: Check if the argument evaluates to NULL.
    } else {
      diagnostics->Report(second->getLocStart(), secondStmtMissingID);
      return false;
    }
    return true;
  }
  
};
  
class MatchFinderASTConsumer : public ASTConsumer {
public:
  MatchFinderASTConsumer() {
      innerConsumer = finder.newASTConsumer();

      finder.addMatcher(
        functionDecl(
          isExpansionInMainFile(),
          hasName("main")
        ).bind(MAIN_FUNCTION_BINDING),
        &callback
      );
  }
    
  void HandleTranslationUnit(ASTContext &context) override {
    finder.matchAST(context);
  }
  
private:
  MatchFinder finder;
  MainFunctionHandler callback;
  std::unique_ptr<ASTConsumer> innerConsumer;
};
  
class CheckFastIOAction : public  PluginASTAction {
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

static FrontendPluginRegistry::Add<CheckFastIOAction>
    X("fast_cin", "Checks if the commands for the fast cin/cout are in the code.")   ;
