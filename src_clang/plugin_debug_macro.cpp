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

class CheckDebugMacroAction : public  PluginASTAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(
    CompilerInstance &CI, llvm::StringRef) override {
    
    auto& sourceManager = CI.getSourceManager();
    auto mainFile = sourceManager.getMainFileID();
    bool error = false;
    const StringRef source = sourceManager.getBufferData(mainFile, &error);
    
    if (!error) {
      llvm::Regex tooMuchNewLines("\\/\\/[^\\n]*cout");
      // Find all matches
      StringRef sourceSuffix = source;
      while(true){
        SmallVector<StringRef, 1> matches;
        if (tooMuchNewLines.match(sourceSuffix, &matches)) {
            int offset = matches[0].begin() - source.begin();
            auto location = sourceManager.getLocForStartOfFile(mainFile).getLocWithOffset(offset);

            DiagnosticsEngine &D = CI.getDiagnostics();
            unsigned DiagID = D.getCustomDiagID(
              DiagnosticsEngine::Error,
              "Use debugging macro instead. #define D(x) cout << \"line \" "
              "<< __LINE__ << \": \" << #x << \"=\" << x << endl;");
            D.Report(location, DiagID);
            sourceSuffix = source.substr(offset + matches[0].size());
        } else {
          break;
        } 
      }
    }
    
    return llvm::make_unique<ASTConsumer>();
  }
  
  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }
};
  
}

static FrontendPluginRegistry::Add<CheckDebugMacroAction>
    X("debug_macro", "Check for commmented lines with cout-s and suggests using debug macro instead.");
