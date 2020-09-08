#ifndef WASMATI_AST_BUILDER_H_
#define WASMATI_AST_BUILDER_H_

#include "src/cast.h"
#include "src/generate-names.h"
#include "src/graph.h"
#include "src/ir-util.h"
#include "src/ir.h"
#include "src/options.h"

#include <map>
using namespace wabt;

namespace wasmati {
struct AST {
    ModuleContext& mc;
    Graph& graph;

    std::map<const Expr*, Node*> exprNodes;
    std::map<const Block*, Node*> ifBlocks;
    std::map<const Func*, Node*> returnFunc;

    AST(ModuleContext& mc, Graph& graph) : mc(mc), graph(graph) {}

    ~AST() {}

    void generateAST(GenerateCPGOptions& options);
    void getLocalsNames(Func* f, std::vector<std::string>& names) const;

    void construct(const Expr& e,
                   std::vector<Node*>& expStack,
                   std::vector<Node*>& expList);

    void construct(const ExprList& es,
                   Index nresults,
                   Node* holder,
                   Func* function = nullptr);
};

}  // namespace wasmati
#endif  // WABT_AST_BUILDER_H_
