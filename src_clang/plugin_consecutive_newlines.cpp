//------------------------------------------------------------------------------
// Clang plugin to check if no goto statements are used.
//
// Once the .so is built, it can be loaded by Clang. For example:
//
// $ eli/clang-llvm/bin/clang++ -fsyntax-only \
//    -Xclang -load -Xclang eli/llvm-clang-samples/build/plugin_consecutive_newlines.so \
//    -Xclang -plugin -Xclang consecutive_newlines \
//    test-files/test_consecutive_newlines.cc 
//------------------------------------------------------------------------------
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

class CheckGotoAction : public  PluginASTAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) {
    
    auto& sourceManager = CI.getSourceManager();
    auto mainFile = sourceManager.getMainFileID();
    bool error = false;
    StringRef source = sourceManager.getBufferData(mainFile, &error);
    
    if (!error) {
      llvm::Regex tooMuchNewLines("(\n[ \t\r\v\f]*){4}");
      SmallVector<StringRef, 1> matches;
      if (tooMuchNewLines.match(source, &matches)) {
        int offset = matches[0].begin() - source.begin();
        auto location = sourceManager.getLocForStartOfFile(mainFile).getLocWithOffset(offset);
        //matches[0]
        DiagnosticsEngine &D = CI.getDiagnostics();
        unsigned DiagID = D.getCustomDiagID(DiagnosticsEngine::Error,
                                            "Too much new lines.");
        D.Report(location, DiagID);
      } 
    }
    
    return nullptr;
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
    X("consecutive_newlines", "Permits more than 2 consecutive empty lines.");
