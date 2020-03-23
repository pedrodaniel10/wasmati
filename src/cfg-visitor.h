#ifndef WASMATI_CFG_H
#define WASMATI_CFG_H
#include <list>
#include <set>
#include <stack>

#include "src/cast.h"
#include "src/graph-visitor.h"
#include "src/graph.h"

namespace wasmati {

class CFGvisitor : public GraphVisitor {
    Graph* _graph;
    Instruction* _lastInstruction;
    std::stack<Instruction*> _currentInstruction;
    std::list<std::pair<std::string, Instruction*>> _blocks;
    std::set<Node*> _unreachableInsts;

    Node* getLeftMostLeaf(Node* node);

    // Edges
    void visitASTEdge(ASTEdge*) override;
    void visitCFGEdge(CFGEdge*) override{/*Do nothing*/};
    void visitPDGEdge(PDGEdge*) override { assert(false); };

    // Nodes
    void visitModule(Module*) override;
    void visitFunction(Function*) override;
    void visitTypeNode(TypeNode*) override { assert(false); };
    void visitSimpleNode(SimpleNode*) override { assert(false); };
    void visitInstructions(Instructions*) override;
    void visitInstruction(Instruction*) override;
    void visitReturn(Return*) override;
    void visitElse(Else*) override;
    void visitStart(Start*) override { assert(false); };
    void visitTrap(Trap*) override { assert(false); };
    void visitIndexNode(IndexNode*) override { assert(false); };

protected:
    // Expressions
    void OnBinaryExpr(BinaryExpr*) override;
    void OnBlockExpr(BlockExpr*) override;
    void OnBrExpr(BrExpr*) override;
    void OnBrIfExpr(BrIfExpr*) override;
    void OnBrOnExnExpr(BrOnExnExpr*) override;
    void OnBrTableExpr(BrTableExpr*) override;
    void OnCallExpr(CallExpr*) override;
    void OnCallIndirectExpr(CallIndirectExpr*) override;
    void OnCompareExpr(CompareExpr*) override;
    void OnConstExpr(ConstExpr*) override;
    void OnConvertExpr(ConvertExpr*) override;
    void OnDropExpr(DropExpr*) override;
    void OnGlobalGetExpr(GlobalGetExpr*) override;
    void OnGlobalSetExpr(GlobalSetExpr*) override;
    void OnIfExpr(IfExpr*) override;
    void OnLoadExpr(LoadExpr*) override;
    void OnLocalGetExpr(LocalGetExpr*) override;
    void OnLocalSetExpr(LocalSetExpr*) override;
    void OnLocalTeeExpr(LocalTeeExpr*) override;
    void OnLoopExpr(LoopExpr*) override;
    void OnMemoryCopyExpr(MemoryCopyExpr*) override;
    void OnDataDropExpr(DataDropExpr*) override;
    void OnMemoryFillExpr(MemoryFillExpr*) override;
    void OnMemoryGrowExpr(MemoryGrowExpr*) override;
    void OnMemoryInitExpr(MemoryInitExpr*) override;
    void OnMemorySizeExpr(MemorySizeExpr*) override;
    void OnTableCopyExpr(TableCopyExpr*) override;
    void OnElemDropExpr(ElemDropExpr*) override;
    void OnTableInitExpr(TableInitExpr*) override;
    void OnTableGetExpr(TableGetExpr*) override;
    void OnTableSetExpr(TableSetExpr*) override;
    void OnTableGrowExpr(TableGrowExpr*) override;
    void OnTableSizeExpr(TableSizeExpr*) override;
    void OnTableFillExpr(TableFillExpr*) override;
    void OnRefFuncExpr(RefFuncExpr*) override;
    void OnRefNullExpr(RefNullExpr*) override;
    void OnRefIsNullExpr(RefIsNullExpr*) override;
    void OnNopExpr(NopExpr*) override;
    void OnReturnExpr(ReturnExpr*) override;
    void OnReturnCallExpr(ReturnCallExpr*) override;
    void OnReturnCallIndirectExpr(ReturnCallIndirectExpr*) override;
    void OnSelectExpr(SelectExpr*) override;
    void OnStoreExpr(StoreExpr*) override;
    void OnUnaryExpr(UnaryExpr*) override;
    void OnUnreachableExpr(UnreachableExpr*) override;
    void OnTryExpr(TryExpr*) override;
    void OnThrowExpr(ThrowExpr*) override;
    void OnRethrowExpr(RethrowExpr*) override;
    void OnAtomicWaitExpr(AtomicWaitExpr*) override;
    void OnAtomicNotifyExpr(AtomicNotifyExpr*) override;
    void OnAtomicLoadExpr(AtomicLoadExpr*) override;
    void OnAtomicStoreExpr(AtomicStoreExpr*) override;
    void OnAtomicRmwExpr(AtomicRmwExpr*) override;
    void OnAtomicRmwCmpxchgExpr(AtomicRmwCmpxchgExpr*) override;
    void OnTernaryExpr(TernaryExpr*) override;
    void OnSimdLaneOpExpr(SimdLaneOpExpr*) override;
    void OnSimdShuffleOpExpr(SimdShuffleOpExpr*) override;
    void OnLoadSplatExpr(LoadSplatExpr*) override;

private:
    void visitSequential(Node* node);
    bool visitArity1();
    bool visitArity2();

public:
    CFGvisitor(Graph* graph) : _graph(graph) { _lastInstruction = nullptr; }
};

}  // namespace wasmati

#endif /*end WASMATI_CFG_H*/