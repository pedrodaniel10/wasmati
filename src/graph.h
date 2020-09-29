#ifndef WASMATI_GRAPH_H
#define WASMATI_GRAPH_H
#include <set>
#include "src/cast.h"
#include "src/common.h"
#include "src/ir-util.h"
#include "src/ir.h"
#include "src/options.h"
#include "src/stream.h"

using namespace wabt;
namespace wasmati {
class GraphVisitor;
class Edge;

inline const std::string& emptyString() {
    static const std::string empty = "";
    return empty;
}

inline const Const& emptyConst() {
    static const Const empty = {};
    return empty;
}

enum class EdgeType { AST, CFG, PDG };

enum class NodeType {
    Module,
    Function,
    VarNode,
    FunctionSignature,
    Instructions,
    Instruction,
    Parameters,
    Locals,
    Results,
    Return,
    Else,
    Trap,
    Start,
};

class Node {
    static Index idCount;
    const Index _id;
    std::vector<Edge*> _inEdges;
    std::vector<Edge*> _outEdges;

public:
    const NodeType _type;

    // Properties
    NodeType type() { return _type; }
    virtual const std::string& name() const {
        assert(false);
        return emptyString();
    }
    virtual Index index() const {
        assert(false);
        return 0;
    }
    virtual Index nargs() const {
        assert(false);
        return 0;
    }
    virtual Index nlocals() const {
        assert(false);
        return 0;
    }
    virtual Index nresults() const {
        assert(false);
        return 0;
    }
    virtual bool isImport() const {
        assert(false);
        return false;
    }
    virtual Type varType() const {
        assert(false);
        return {};
    }
    virtual ExprType instType() const {
        assert(false);
        return ExprType::Nop;
    }
    virtual Opcode opcode() const {
        assert(false);
        return {};
    }
    virtual const Const& value() const {
        assert(false);
        return emptyConst();
    }
    virtual const std::string& label() const {
        assert(false);
        return emptyString();
    }
    virtual bool hasElse() const {
        assert(false);
        return false;
    }
    virtual Index offset() const {
        assert(false);
        return 0;
    }
    virtual Location location() const {
        assert(false);
        return {};
    }
    virtual Node* block() {
        assert(false);
        return nullptr;
    }

    explicit Node(NodeType type) : _id(idCount++), _type(type) {}
    virtual ~Node();

    inline Index getId() const { return _id; }
    inline const std::vector<Edge*>& inEdges() const { return _inEdges; }
    inline const std::vector<Edge*>& outEdges() const { return _outEdges; }
    std::vector<Edge*> inEdges(EdgeType type);
    std::vector<Edge*> outEdges(EdgeType type);

    inline Edge* getOutEdge(Index i, EdgeType type);
    inline Index getNumOutEdges() const { return _outEdges.size(); }
    inline Edge* getInEdge(Index i, EdgeType type) {
        auto edges = inEdges(type);
        assert(i < edges.size());
        return edges[i];
    }

    Node* getChild(Index n, EdgeType type);
    Node* getParent(Index n, EdgeType type);
    inline Index getNumInEdges() const { return _inEdges.size(); }
    inline void addInEdge(Edge* e) { _inEdges.push_back(e); }
    inline void addOutEdge(Edge* e) { _outEdges.push_back(e); }

    bool hasEdgesOf(EdgeType) const;
    bool hasInEdgesOf(EdgeType) const;
    bool hasOutEdgesOf(EdgeType) const;

    virtual void accept(GraphVisitor* visitor);
    virtual void acceptEdges(GraphVisitor* visitor);
};

template <NodeType t>
class BaseNode : public Node {
public:
    BaseNode() : Node(t) {}

    static bool classof(const Node* node) { return node->_type == t; }
};

class Module : public BaseNode<NodeType::Module> {
    const std::string _name;

public:
    Module() {}
    Module(std::string name) : _name(name) {}

    const std::string& name() const override { return _name; }

    void accept(GraphVisitor* visitor);
};

class Function : public BaseNode<NodeType::Function> {
    Func* const _f;
    const Index _index;
    const Index _nargs;
    const Index _nlocals;
    const Index _nresults;
    const bool _isImport;

public:
    Function(Func* f, Index index, bool isImport)
        : _f(f),
          _index(index),
          _nargs(f->GetNumParams()),
          _nlocals(f->GetNumLocals()),
          _nresults(f->GetNumResults()),
          _isImport(isImport) {}

    const std::string& name() const override { return _f->name; }
    Index index() const override { return _index; }
    Index nargs() const override { return _nargs; }
    Index nlocals() const override { return _nlocals; }
    Index nresults() const override { return _nresults; }
    bool isImport() const override { return _isImport; }
    Func* getFunc() { return _f; }

    void accept(GraphVisitor* visitor);
};

class VarNode : public BaseNode<NodeType::VarNode> {
    Type _varType;
    const std::string _name;

public:
    VarNode(Type type, std::string name = "") : _varType(type), _name(name) {}

    const std::string& name() const override { return _name; }
    Type varType() const override { return _varType; }

    void accept(GraphVisitor* visitor);
};

template <NodeType T, char const* nodeName>
class SimpleNode : public BaseNode<T> {
public:
    inline const std::string getNodeName() const { return nodeName; }

    virtual void accept(GraphVisitor* visitor);
};

extern const char functionSignatureName[];
extern const char instructionsName[];
extern const char parametersName[];
extern const char localsName[];
extern const char resultsName[];
extern const char elseName[];
extern const char trapName[];
extern const char startName[];

typedef SimpleNode<NodeType::FunctionSignature, functionSignatureName>
    FunctionSignature;
typedef SimpleNode<NodeType::Instructions, instructionsName> Instructions;
typedef SimpleNode<NodeType::Parameters, parametersName> Parameters;
typedef SimpleNode<NodeType::Locals, localsName> Locals;
typedef SimpleNode<NodeType::Results, resultsName> Results;
typedef SimpleNode<NodeType::Else, elseName> Else;
typedef SimpleNode<NodeType::Trap, trapName> Trap;
typedef SimpleNode<NodeType::Start, startName> Start;

class Instruction : public BaseNode<NodeType::Instruction> {
protected:
    const ExprType _instType;
    const Location _loc;

public:
    Instruction(const ExprType type, const Location loc)
        : _instType(type), _loc(loc) {}

    ExprType instType() const override { return _instType; }

    Location location() const override { return _loc; }
};

template <ExprType exprType>
class BaseInstruction : public Instruction {
public:
    BaseInstruction(const Location _loc = Location())
        : Instruction(exprType, _loc) {}

    static bool classof(const Node* node) {
        return Instruction::classof(node) && (node->instType() == exprType);
    }

    virtual void accept(GraphVisitor* visitor);
};

typedef BaseInstruction<ExprType::Nop> NopInst;
typedef BaseInstruction<ExprType::Unreachable> UnreachableInst;
typedef BaseInstruction<ExprType::Return> ReturnInst;
typedef BaseInstruction<ExprType::BrTable> BrTableInst;
typedef BaseInstruction<ExprType::Drop> DropInst;
typedef BaseInstruction<ExprType::Select> SelectInst;
typedef BaseInstruction<ExprType::MemorySize> MemorySizeInst;
typedef BaseInstruction<ExprType::MemoryGrow> MemoryGrowInst;

class ConstInst : public BaseInstruction<ExprType::Const> {
    const Const _value;

public:
    ConstInst(const ConstExpr* expr)
        : BaseInstruction(expr->loc), _value(expr->const_) {}

    const Const& value() const override { return _value; }

    void accept(GraphVisitor* visitor);

    static std::string writeConst(const Const& _const) {
        std::string s;
        switch (_const.type) {
        case Type::I32:
            s += Opcode::I32Const_Opcode.GetName();
            s += " " + std::to_string(static_cast<int32_t>(_const.u32));
            break;

        case Type::I64:
            s += Opcode::I64Const_Opcode.GetName();
            s += " " + std::to_string(static_cast<int64_t>(_const.u64));
            break;

        case Type::F32: {
            s += Opcode::F32Const_Opcode.GetName();
            float f32;
            memcpy(&f32, &_const.f32_bits, sizeof(f32));
            s += " " + std::to_string(f32);
            break;
        }

        case Type::F64: {
            s += Opcode::F64Const_Opcode.GetName();
            double f64;
            memcpy(&f64, &_const.f64_bits, sizeof(f64));
            s += " " + std::to_string(f64);
            break;
        }

        case Type::V128: {
            assert(false);
            break;
        }

        default:
            assert(false);
            break;
        }
        return s;
    }
};

template <ExprType T>
class OpcodeInst : public BaseInstruction<T> {
    const Opcode _opcode;

public:
    OpcodeInst(const OpcodeExpr<T>* expr)
        : BaseInstruction<T>(expr->loc), _opcode(expr->opcode) {}
    OpcodeInst(Opcode opcode, const Location loc)
        : BaseInstruction<T>(loc), _opcode(opcode) {}

    Opcode opcode() const override { return _opcode; }

    virtual void accept(GraphVisitor* visitor);
};

typedef OpcodeInst<ExprType::Binary> BinaryInst;
typedef OpcodeInst<ExprType::Compare> CompareInst;
typedef OpcodeInst<ExprType::Convert> ConvertInst;
typedef OpcodeInst<ExprType::Unary> UnaryInst;

template <ExprType T>
class LoadStoreBase : public OpcodeInst<T> {
    const Index _offset;

public:
    LoadStoreBase(const LoadStoreExpr<T>* expr)
        : OpcodeInst<T>(expr->opcode, expr->loc), _offset(expr->offset) {}

    Index offset() const override { return _offset; }

    void accept(GraphVisitor* visitor);
};

typedef LoadStoreBase<ExprType::Load> LoadInst;
typedef LoadStoreBase<ExprType::Store> StoreInst;

template <ExprType T>
class LabeledInst : public BaseInstruction<T> {
    const std::string _label;

public:
    LabeledInst(const VarExpr<T>* expr)
        : BaseInstruction<T>(expr->loc), _label(expr->var.name()) {}
    LabeledInst(std::string label, const Location loc = Location())
        : BaseInstruction<T>(loc), _label(label) {}

    const std::string& label() const { return _label; }

    virtual void accept(GraphVisitor* visitor);
};

typedef LabeledInst<ExprType::Br> BrInst;
typedef LabeledInst<ExprType::BrIf> BrIfInst;
typedef LabeledInst<ExprType::GlobalGet> GlobalGetInst;
typedef LabeledInst<ExprType::GlobalSet> GlobalSetInst;
typedef LabeledInst<ExprType::LocalGet> LocalGetInst;
typedef LabeledInst<ExprType::LocalSet> LocalSetInst;
typedef LabeledInst<ExprType::LocalTee> LocalTeeInst;

template <ExprType T>
class CallBase : public LabeledInst<T> {
    const Index _nargs;
    const Index _nresults;

public:
    CallBase(const CallExpr* expr, Location loc, Index nargs, Index nresults)
        : LabeledInst<T>(expr->var.name(), loc),
          _nargs(nargs),
          _nresults(nresults) {}
    CallBase(const CallIndirectExpr* expr,
             Location loc,
             Index nargs,
             Index nresults)
        : LabeledInst<T>(expr->table.name(), loc),
          _nargs(nargs),
          _nresults(nresults) {}
    Index nargs() const override { return _nargs; }
    Index nresults() const override { return _nresults; }

    virtual void accept(GraphVisitor* visitor);
};

typedef CallBase<ExprType::Call> CallInst;
typedef CallBase<ExprType::CallIndirect> CallIndirectInst;

template <ExprType T>
class BlockBase : public LabeledInst<T> {
    const Index _nresults;

public:
    BlockBase(const BlockExprBase<T>* expr)
        : LabeledInst<T>(expr->block.label, expr->loc),
          _nresults(expr->block.decl.GetNumResults()) {}

    BlockBase(const Block& block)
        : LabeledInst<T>(block.label, block.end_loc),
          _nresults(block.decl.GetNumResults()) {}

    Index nresults() const override { return _nresults; }

    virtual void accept(GraphVisitor* visitor);
};

typedef BlockBase<ExprType::Block> BlockInst;
typedef BlockBase<ExprType::Loop> LoopInst;

class BeginBlockInst : public LabeledInst<ExprType::Block> {
    BlockInst* _block;

public:
    BeginBlockInst(const VarExpr<ExprType::Block>* expr, BlockInst* block)
        : LabeledInst<ExprType::Block>(expr), _block(block) {}
    BeginBlockInst(std::string label,
                   BlockInst* block,
                   const Location loc = Location())
        : LabeledInst<ExprType::Block>(label, loc), _block(block) {}

    Node* block() override { return _block; }

    virtual void accept(GraphVisitor* visitor);
};

class IfInst : public BaseInstruction<ExprType::If> {
    const Index _nresults;
    const bool _hasElse;

public:
    IfInst(const IfExpr* expr)
        : BaseInstruction(expr->loc),
          _nresults(expr->true_.decl.GetNumResults()),
          _hasElse(!expr->false_.empty()) {}

    Index nresults() const override { return _nresults; }
    bool hasElse() const override { return _hasElse; }
    virtual void accept(GraphVisitor* visitor);
};

enum class PDGType { Local, Global, Function, Control, Const };
struct Edge {
private:
    Node* const _src;
    Node* const _dest;
    const EdgeType _type;

public:
    Edge(Node* src, Node* dest, EdgeType type)
        : _src(src), _dest(dest), _type(type) {
        assert(src != nullptr && dest != nullptr);
        src->addOutEdge(this);
        dest->addInEdge(this);
    }

    virtual ~Edge() {}

    inline Node* src() const { return _src; }
    inline Node* dest() const { return _dest; }
    inline EdgeType type() const { return _type; }
    virtual PDGType pdgType() const {
        assert(false);
        return PDGType::Local;
    }
    virtual const std::string& label() const {
        assert(false);
        return emptyString();
    }
    virtual const Const& value() const {
        assert(false);
        return emptyConst();
    }
    virtual void accept(GraphVisitor* visitor) = 0;
};

struct ASTEdge : Edge {
    ASTEdge(Node* src, Node* dest) : Edge(src, dest, EdgeType::AST) {}
    void accept(GraphVisitor* visitor);
    static bool classof(const Edge* e) { return e->type() == EdgeType::AST; }
};

struct CFGEdge : Edge {
    const std::string _label;

    CFGEdge(Node* src, Node* dest) : Edge(src, dest, EdgeType::CFG) {}
    CFGEdge(Node* src, Node* dest, const std::string& label)
        : Edge(src, dest, EdgeType::CFG), _label(label) {}
    void accept(GraphVisitor* visitor);
    static bool classof(const Edge* e) { return e->type() == EdgeType::CFG; }
    inline const std::string& label() const { return _label; }
};

struct PDGEdge : Edge {
    const std::string _label;
    const PDGType _pdgType;

    PDGEdge(Node* src, Node* dest, PDGType type)
        : Edge(src, dest, EdgeType::PDG), _pdgType(type) {}
    PDGEdge(CFGEdge* e)
        : PDGEdge(e->src(), e->dest(), e->_label, PDGType::Control) {}
    PDGEdge(Node* src, Node* dest, const std::string& label, PDGType type)
        : Edge(src, dest, EdgeType::PDG), _label(label), _pdgType(type) {}

    void accept(GraphVisitor* visitor);
    static bool classof(const Edge* e) { return e->type() == EdgeType::PDG; }

    inline const std::string& label() const { return _label; }
    inline PDGType pdgType() const override { return _pdgType; }
};

struct PDGEdgeConst : PDGEdge {
    const Const _const;

    PDGEdgeConst(Node* src, Node* dest, const Const& const_)
        : PDGEdge(src, dest, ConstInst::writeConst(const_), PDGType::Const),
          _const(const_) {}

    inline const Const& value() const override { return _const; }
};

class Graph {
    wabt::ModuleContext _mc;
    std::vector<Node*> _nodes;
    Trap* _trap;
    Start* _start;
    Module* _module;

public:
    Graph(wabt::Module& mc);
    ~Graph();

    inline void setModule(Module* module) {
        assert(module != nullptr);
        _module = module;
    }
    inline void insertNode(Node* node) { _nodes.push_back(node); }
    inline const std::vector<Node*>& getNodes() const { return _nodes; }
    inline wabt::ModuleContext& getModuleContext() { return _mc; }
    inline Trap* getTrap() {
        if (_trap == nullptr) {
            _trap = new Trap();
            this->insertNode(_trap);
        }
        return _trap;
    }
    inline Start* getStart() {
        if (_start == nullptr) {
            _start = new Start();
            this->insertNode(_start);
        }
        return _start;
    }
    inline Module* getModule() const {
        assert(_module != nullptr);
        return _module;
    }
};

class GraphVisitor {
public:
    // Edges
    virtual void visitASTEdge(ASTEdge* e) = 0;
    virtual void visitCFGEdge(CFGEdge* e) = 0;
    virtual void visitPDGEdge(PDGEdge* e) = 0;

    // Nodes
    virtual void visitModule(Module* node) = 0;
    virtual void visitFunction(Function* node) = 0;
    virtual void visitFunctionSignature(FunctionSignature* node) = 0;
    virtual void visitParameters(Parameters* node) = 0;
    virtual void visitInstructions(Instructions* node) = 0;
    virtual void visitLocals(Locals* node) = 0;
    virtual void visitResults(Results* node) = 0;
    virtual void visitElse(Else* node) = 0;
    virtual void visitStart(Start* node) = 0;
    virtual void visitTrap(Trap* node) = 0;
    virtual void visitVarNode(VarNode* node) = 0;
    virtual void visitNopInst(NopInst* node) = 0;
    virtual void visitUnreachableInst(UnreachableInst* node) = 0;
    virtual void visitReturnInst(ReturnInst* node) = 0;
    virtual void visitBrTableInst(BrTableInst* node) = 0;
    virtual void visitDropInst(DropInst* node) = 0;
    virtual void visitSelectInst(SelectInst* node) = 0;
    virtual void visitMemorySizeInst(MemorySizeInst* node) = 0;
    virtual void visitMemoryGrowInst(MemoryGrowInst* node) = 0;
    virtual void visitConstInst(ConstInst* node) = 0;
    virtual void visitBinaryInst(BinaryInst* node) = 0;
    virtual void visitCompareInst(CompareInst* node) = 0;
    virtual void visitConvertInst(ConvertInst* node) = 0;
    virtual void visitUnaryInst(UnaryInst* node) = 0;
    virtual void visitLoadInst(LoadInst* node) = 0;
    virtual void visitStoreInst(StoreInst* node) = 0;
    virtual void visitBrInst(BrInst* node) = 0;
    virtual void visitBrIfInst(BrIfInst* node) = 0;
    virtual void visitGlobalGetInst(GlobalGetInst* node) = 0;
    virtual void visitGlobalSetInst(GlobalSetInst* node) = 0;
    virtual void visitLocalGetInst(LocalGetInst* node) = 0;
    virtual void visitLocalSetInst(LocalSetInst* node) = 0;
    virtual void visitLocalTeeInst(LocalTeeInst* node) = 0;
    virtual void visitBeginBlockInst(BeginBlockInst* node) = 0;
    virtual void visitCallInst(CallInst* node) = 0;
    virtual void visitCallIndirectInst(CallIndirectInst* node) = 0;
    virtual void visitBlockInst(BlockInst* node) = 0;
    virtual void visitLoopInst(LoopInst* node) = 0;
    virtual void visitIfInst(IfInst* node) = 0;
};

class GraphWriter : public GraphVisitor {
protected:
    wabt::Stream* _stream;
    Graph* _graph;
    GenerateCPGOptions _options;

    void writePuts(const char* s) {
        size_t len = strlen(s);
        _stream->WriteData(s, len);
    }
    void writeString(const std::string& str) { writePuts(str.c_str()); }
    void writePutsln(const char* s) {
        size_t len = strlen(s);
        _stream->WriteData(s, len);
        _stream->WriteChar('\n');
    }

    void writeStringln(const std::string& str) { writePutsln(str.c_str()); }

public:
    GraphWriter(wabt::Stream* stream, Graph* graph, GenerateCPGOptions options)
        : _stream(stream), _graph(graph), _options(options) {}

    virtual void writeGraph() = 0;
};

template <>
inline void SimpleNode<NodeType::FunctionSignature,
                       functionSignatureName>::accept(GraphVisitor* visitor) {
    visitor->visitFunctionSignature(this);
}

template <>
inline void SimpleNode<NodeType::Instructions, instructionsName>::accept(
    GraphVisitor* visitor) {
    visitor->visitInstructions(this);
}

template <>
inline void SimpleNode<NodeType::Parameters, parametersName>::accept(
    GraphVisitor* visitor) {
    visitor->visitParameters(this);
}

template <>
inline void SimpleNode<NodeType::Locals, localsName>::accept(
    GraphVisitor* visitor) {
    visitor->visitLocals(this);
}

template <>
inline void SimpleNode<NodeType::Results, resultsName>::accept(
    GraphVisitor* visitor) {
    visitor->visitResults(this);
}

template <>
inline void SimpleNode<NodeType::Else, elseName>::accept(
    GraphVisitor* visitor) {
    visitor->visitElse(this);
}

template <>
inline void SimpleNode<NodeType::Trap, trapName>::accept(
    GraphVisitor* visitor) {
    visitor->visitTrap(this);
}

template <>
inline void SimpleNode<NodeType::Start, startName>::accept(
    GraphVisitor* visitor) {
    visitor->visitStart(this);
}

template <>
inline void BaseInstruction<ExprType::Nop>::accept(GraphVisitor* visitor) {
    visitor->visitNopInst(this);
}

template <>
inline void BaseInstruction<ExprType::Unreachable>::accept(
    GraphVisitor* visitor) {
    visitor->visitUnreachableInst(this);
}

template <>
inline void BaseInstruction<ExprType::Return>::accept(GraphVisitor* visitor) {
    visitor->visitReturnInst(this);
}

template <>
inline void BaseInstruction<ExprType::BrTable>::accept(GraphVisitor* visitor) {
    visitor->visitBrTableInst(this);
}

template <>
inline void BaseInstruction<ExprType::Drop>::accept(GraphVisitor* visitor) {
    visitor->visitDropInst(this);
}

template <>
inline void BaseInstruction<ExprType::Select>::accept(GraphVisitor* visitor) {
    visitor->visitSelectInst(this);
}

template <>
inline void BaseInstruction<ExprType::MemorySize>::accept(
    GraphVisitor* visitor) {
    visitor->visitMemorySizeInst(this);
}

template <>
inline void BaseInstruction<ExprType::MemoryGrow>::accept(
    GraphVisitor* visitor) {
    visitor->visitMemoryGrowInst(this);
}

template <>
inline void OpcodeInst<ExprType::Binary>::accept(GraphVisitor* visitor) {
    visitor->visitBinaryInst(this);
}

template <>
inline void OpcodeInst<ExprType::Compare>::accept(GraphVisitor* visitor) {
    visitor->visitCompareInst(this);
}

template <>
inline void OpcodeInst<ExprType::Convert>::accept(GraphVisitor* visitor) {
    visitor->visitConvertInst(this);
}

template <>
inline void OpcodeInst<ExprType::Unary>::accept(GraphVisitor* visitor) {
    visitor->visitUnaryInst(this);
}

template <>
inline void LoadStoreBase<ExprType::Load>::accept(GraphVisitor* visitor) {
    visitor->visitLoadInst(this);
}

template <>
inline void LoadStoreBase<ExprType::Store>::accept(GraphVisitor* visitor) {
    visitor->visitStoreInst(this);
}

template <>
inline void LabeledInst<ExprType::Br>::accept(GraphVisitor* visitor) {
    visitor->visitBrInst(this);
}

template <>
inline void LabeledInst<ExprType::BrIf>::accept(GraphVisitor* visitor) {
    visitor->visitBrIfInst(this);
}

template <>
inline void LabeledInst<ExprType::GlobalGet>::accept(GraphVisitor* visitor) {
    visitor->visitGlobalGetInst(this);
}

template <>
inline void LabeledInst<ExprType::GlobalSet>::accept(GraphVisitor* visitor) {
    visitor->visitGlobalSetInst(this);
}

template <>
inline void LabeledInst<ExprType::LocalGet>::accept(GraphVisitor* visitor) {
    visitor->visitLocalGetInst(this);
}

template <>
inline void LabeledInst<ExprType::LocalSet>::accept(GraphVisitor* visitor) {
    visitor->visitLocalSetInst(this);
}

template <>
inline void LabeledInst<ExprType::LocalTee>::accept(GraphVisitor* visitor) {
    visitor->visitLocalTeeInst(this);
}

template <>
inline void CallBase<ExprType::CallIndirect>::accept(GraphVisitor* visitor) {
    visitor->visitCallIndirectInst(this);
}

template <>
inline void CallBase<ExprType::Call>::accept(GraphVisitor* visitor) {
    visitor->visitCallInst(this);
}

template <>
inline void BlockBase<ExprType::Block>::accept(GraphVisitor* visitor) {
    visitor->visitBlockInst(this);
}

template <>
inline void BlockBase<ExprType::Loop>::accept(GraphVisitor* visitor) {
    visitor->visitLoopInst(this);
}

template <ExprType t>
inline void BaseInstruction<t>::accept(GraphVisitor* visitor) {
    assert(false);
}

template <ExprType t>
inline void OpcodeInst<t>::accept(GraphVisitor* visitor) {
    assert(false);
}

template <ExprType t>
inline void LabeledInst<t>::accept(GraphVisitor* visitor) {
    assert(false);
}

typedef std::set<Node*> NodeSet;
typedef std::set<Edge*> EdgeSet;

}  // namespace wasmati

#endif /* WASMATI_GRAPH_H*/
