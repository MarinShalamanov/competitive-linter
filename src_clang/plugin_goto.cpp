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

#include "llvm/Support/raw_ostream.h"

using namespace clang;

namespace {

// TODO: Remove globals
DiagnosticsEngine* DiagnosticsEngineInstance;
    
class CheckGotoASTVisitor : public RecursiveASTVisitor<CheckGotoASTVisitor> {
public:
  CheckGotoASTVisitor() {}

  bool VisitStmt(Stmt *stmt) {
    const unsigned IDGoto = DiagnosticsEngineInstance->getCustomDiagID(
        clang::DiagnosticsEngine::Error,
        "Don't use goto statements."); 
    const unsigned IDHell = DiagnosticsEngineInstance->getCustomDiagID(
        clang::DiagnosticsEngine::Error,
        "You too. And don't use goto statements.");   
    if (isa<GotoStmt>(stmt)) {
        GotoStmt* Goto = cast<GotoStmt>(stmt);
        const char* LabelName = Goto->getLabel()->getStmt()->getName();
        unsigned DiagnosticsID = (strcmp(LabelName, "hell") == 0) ? IDHell : IDGoto;
        DiagnosticsEngineInstance->Report(stmt->getLocStart(), DiagnosticsID);
    }
    return true;
  }
};

class CheckGotoConsumer : public ASTConsumer {
public:
  CheckGotoConsumer() : Visitor() {}
    
  virtual bool HandleTopLevelDecl(DeclGroupRef DG) {
    for (auto b = DG.begin(), e = DG.end(); b != e; ++b) {
      Visitor.TraverseDecl(*b);
    }
    return true;
  }
private:
  CheckGotoASTVisitor Visitor;
};

class CheckGotoAction : public PluginASTAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) {
    DiagnosticsEngineInstance = &(CI.getDiagnostics());
    return llvm::make_unique<CheckGotoConsumer>();
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
};
}

static FrontendPluginRegistry::Add<CheckGotoAction>
    X("goto", "check for goto statements");
