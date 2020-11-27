#include "query.h"
#include <iostream>
namespace wasmati {
const Graph* Query::_graph = nullptr;
NodeSet Query::emptyNodeSet = NodeSet();
const EdgeCondition& Query::ALL_EDGES = [](Edge*) { return true; };
const EdgeCondition& Query::AST_EDGES = [](Edge* e) {
    return e->type() == EdgeType::AST;
};
const EdgeCondition& Query::CFG_EDGES = [](Edge* e) {
    return e->type() == EdgeType::CFG;
};
const EdgeCondition& Query::PDG_EDGES = [](Edge* e) {
    return e->type() == EdgeType::PDG;
};
const EdgeCondition& Query::CG_EDGES = [](Edge* e) {
    return e->type() == EdgeType::CG;
};
const EdgeCondition& Query::PG_EDGES = [](Edge* e) {
    return e->type() == EdgeType::PG;
};
const NodeCondition& Query::ALL_INSTS = [](Node* node) {
    return node->type() == NodeType::Instruction;
};

const NodeCondition& Query::ALL_NODES = [](Node* node) { return true; };

const Predicate& Query::TRUE_PREDICATE = Predicate().truePredicate();

NodeSet Query::children(const NodeSet& nodes,
                        const EdgeCondition& edgeCondition) {
    NodeSet result;
    for (Node* node : nodes) {
        EdgeSet edges = filterEdges(node->outEdges(), edgeCondition);
        for (Edge* e : edges) {
            result.insert(e->dest());
        }
    }
    return result;
}

NodeSet Query::parents(const NodeSet& nodes,
                       const EdgeCondition& edgeCondition) {
    NodeSet result;
    for (Node* node : nodes) {
        EdgeSet edges = filterEdges(node->inEdges(), edgeCondition);
        for (Edge* e : edges) {
            result.insert(e->src());
        }
    }
    return result;
}

EdgeSet Query::filterEdges(const EdgeSet& edges,
                           const EdgeCondition& edgeCondition) {
    EdgeSet result;
    for (auto e : edges) {
        if (edgeCondition(e)) {
            result.insert(e);
        }
    }
    return result;
}

#define WASMATI_EVALUATION(type, var, eval, rALL)                  \
    NodeSet Query::filter(const NodeSet& nodes, const type& var) { \
        NodeSet result;                                            \
        for (Node * node : nodes) {                                \
            if (eval(node)) {                                      \
                result.insert(node);                               \
            }                                                      \
        }                                                          \
        return result;                                             \
    }
#include "src/config/predicates.def"
#undef WASMATI_EVALUATION

#define WASMATI_EVALUATION(type, var, eval, rALL)                 \
    bool Query::contains(const NodeSet& nodes, const type& var) { \
        for (Node * node : nodes) {                               \
            if (eval(node)) {                                     \
                return true;                                      \
            }                                                     \
        }                                                         \
        return false;                                             \
    }
#include "src/config/predicates.def"
#undef WASMATI_EVALUATION

bool Query::containsEdge(const EdgeSet& edges,
                         const EdgeCondition& edgeCondition) {
    for (Edge* e : edges) {
        if (edgeCondition(e)) {
            return true;
        }
    }
    return false;
}

NodeSet Query::map(const NodeSet& nodes,
                   const std::function<Node*(Node*)> func) {
    NodeSet result;
    for (Node* node : nodes) {
        result.insert(func(node));
    }
    return result;
}

NodeSet Query::map(const NodeSet& nodes,
                   const std::function<NodeSet(Node*)> func) {
    NodeSet result;
    for (Node* node : nodes) {
        auto mapping = func(node);
        result.insert(mapping.begin(), mapping.end());
    }
    return result;
}

#define WASMATI_EVALUATION(type, var, eval, rALL)                         \
    NodeSet Query::BFS(const NodeSet& nodes, const type& var,             \
                       const EdgeCondition& edgeCondition, Index limit,   \
                       bool reverse) {                                    \
        NodeSet result;                                                   \
        NodeSet visited;                                                  \
        if (nodes.size() == 0 || limit == 0) {                            \
            return result;                                                \
        }                                                                 \
        auto mapFunction = [&](Node* node) {                              \
            auto edges = reverse ? node->inEdges() : node->outEdges();    \
            return map<Node*>(                                            \
                filterEdges(edges, edgeCondition),                        \
                [&](Edge* e) { return reverse ? e->src() : e->dest(); }); \
        };                                                                \
                                                                          \
        std::list<Node*> queue = map<Node*>(nodes, mapFunction);          \
                                                                          \
        while (!queue.empty()) {                                          \
            Node* node = queue.front();                                   \
            queue.pop_front();                                            \
                                                                          \
            if (visited.count(node) == 1) {                               \
                continue;                                                 \
            }                                                             \
            visited.insert(node);                                         \
            if (eval(node)) {                                             \
                result.insert(node);                                      \
            }                                                             \
            if (result.size() == limit) {                                 \
                return result;                                            \
            }                                                             \
            queue.splice(queue.end(), map<Node*>({node}, mapFunction));   \
        }                                                                 \
        return result;                                                    \
    }
#include "src/config/predicates.def"
#undef WASMATI_EVALUATION

#define WASMATI_EVALUATION(type, var, eval, rALL)                              \
    NodeSet Query::BFSincludes(const NodeSet& nodes, const type& var,          \
                               const EdgeCondition& edgeCondition,             \
                               Index limit, bool reverse) {                    \
        auto filtering = filter(nodes, var);                                   \
        if (filtering.size() >= limit) {                                       \
            auto end = filtering.cbegin();                                     \
            std::advance(end, limit);                                          \
            return NodeSet(filtering.cbegin(), end);                           \
        }                                                                      \
        auto result =                                                          \
            BFS(nodes, var, edgeCondition, limit - filtering.size(), reverse); \
        result.insert(filtering.begin(), filtering.end());                     \
        return result;                                                         \
    }
#include "src/config/predicates.def"
#undef WASMATI_EVALUATION

NodeSet Query::module() {
    assert(_graph != nullptr);
    return {_graph->getModule()};
}

NodeSet Query::functions(const NodeCondition& nodeCondition) {
    return filter(children(module(), AST_EDGES), nodeCondition);
}
Node* Query::function(Node* node) {
    assert(node->type() != NodeType::Module);
    return NodeStream(node)
        .BFSincludes(
            [](Node* node) { return node->type() == NodeType::Function; },
            AST_EDGES, 1, true)
        .findFirst()
        .get();
}
NodeSet Query::instructions(const NodeSet& nodes,
                            const NodeCondition& nodeCondition) {
    NodeSet funcInstsNode;
    for (Node* node : nodes) {
        assert(node->type() == NodeType::Function);
        if (node->isImport()) {
            continue;
        }
        funcInstsNode.insert(node->getChild(1, EdgeType::AST));
    }

    return BFS(
        funcInstsNode,
        [&](Node* node) {
            return node->type() == NodeType::Instruction && nodeCondition(node);
        },
        AST_EDGES);
}
NodeSet Query::parameters(const NodeSet& nodes,
                          const NodeCondition& nodeCondition) {
    NodeSet params;
    for (Node* node : nodes) {
        assert(node->type() == NodeType::Function);
        auto paramsNode = filter(
            children({node->getChild(0, EdgeType::AST)}, AST_EDGES),
            [](Node* node) { return node->type() == NodeType::Parameters; });
        assert(paramsNode.size() <= 1);
        if (paramsNode.size() == 1) {
            auto childs = children(paramsNode, AST_EDGES);
            params.insert(childs.begin(), childs.end());
        }
    }
    return params;
}
NodeSet Queries::loopsInsts(std::string& loopName) {
    NodeSet results;
    auto loops = NodeStream(Query::functions())
                     .instructions([&](Node* node) {
                         return node->instType() == ExprType::Loop &&
                                node->label() == loopName;
                     })
                     .toNodeSet();
    results = Query::BFSincludes(loops, Query::ALL_INSTS, Query::AST_EDGES);
    auto beginBlocks = Query::BFS(
        loops,
        [&](Node* node) {
            return node->instType() == ExprType::Block &&
                   Query::contains(
                       Query::parents({node}, Query::CFG_EDGES),
                       [&](Node* n) { return results.count(n) == 1; });
        },
        Query::CFG_EDGES);

    results.insert(beginBlocks.begin(), beginBlocks.end());
    return results;
}
}  // namespace wasmati
