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
    
class EqualsHandler : public MatchFinder::MatchCallback {
public:
  EqualsHandler() {}

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

class CheckGotoAction : public  PluginASTAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) {
    
    auto& sourceManager = CI.getSourceManager();
    auto mainFile = sourceManager.getMainFileID();
    bool error = false;
    StringRef source = sourceManager.getBufferData(mainFile, &error);
    
    if (!error) {
      llvm::errs() << source;
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
    
    equalsHandler = llvm::make_unique<EqualsHandler>();
    constexpr char floatType[] = "float";
    
    finder.addMatcher(
      varDecl(
        isExpansionInMainFile(), 
        hasType(asString(floatType))
      ).bind(VAR_DECL_BINDING),
      equalsHandler.get()
    );
    finder.addMatcher(
      functionDecl(
        isExpansionInMainFile(),
        returns(asString(floatType))
      ).bind(FUNC_DECL_BINDING),
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
    X("no_float", "Suggests double type to be used instead of float.");
