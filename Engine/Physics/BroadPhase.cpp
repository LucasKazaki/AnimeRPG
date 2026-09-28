#include "Engine/Physics/BroadPhase.h"

#include <algorithm>
#include <cmath>

namespace Astral::Physics {

using Math::AABB;
using Math::Vec3;

int DynamicAabbTree::Allocate() {
    int node;
    if (freeList_ == kNull) {
        nodes_.push_back(Node{});
        node = static_cast<int>(nodes_.size() - 1);
    } else {
        // Free nodes chain through `parent`.
        node = freeList_;
        freeList_ = nodes_[static_cast<std::size_t>(node)].parent;
    }
    Node& n = nodes_[static_cast<std::size_t>(node)];
    n = Node{};
    n.height = 0;
    return node;
}

void DynamicAabbTree::Free(int node) {
    Node& n = nodes_[static_cast<std::size_t>(node)];
    n = Node{};
    n.parent = freeList_;
    n.height = -1;
    freeList_ = node;
}

int DynamicAabbTree::CreateProxy(const AABB& box, std::uint32_t userData, float margin) {
    const int proxy = Allocate();
    Node& n = nodes_[static_cast<std::size_t>(proxy)];
    n.box = box.Inflated(std::max(0.0f, margin));
    n.userData = userData;
    n.height = 0;
    InsertLeaf(proxy);
    ++proxyCount_;
    return proxy;
}

void DynamicAabbTree::DestroyProxy(int proxy) {
    if (proxy < 0 || static_cast<std::size_t>(proxy) >= nodes_.size() || !nodes_[static_cast<std::size_t>(proxy)].IsLeaf()
        || nodes_[static_cast<std::size_t>(proxy)].height != 0) {
        return;
    }
    RemoveLeaf(proxy);
    Free(proxy);
    --proxyCount_;
}

bool DynamicAabbTree::MoveProxy(int proxy, const AABB& box, Vec3 displacement, float margin) {
    Node& n = nodes_[static_cast<std::size_t>(proxy)];
    if (n.box.Contains(box)) return false;
    RemoveLeaf(proxy);
    AABB fat = box.Inflated(std::max(0.0f, margin));
    // Predictive extension in the direction of motion.
    const Vec3 d = displacement * 2.0f;
    if (d.x < 0.0f) fat.min.x += d.x; else fat.max.x += d.x;
    if (d.y < 0.0f) fat.min.y += d.y; else fat.max.y += d.y;
    if (d.z < 0.0f) fat.min.z += d.z; else fat.max.z += d.z;
    nodes_[static_cast<std::size_t>(proxy)].box = fat;
    InsertLeaf(proxy);
    return true;
}

void DynamicAabbTree::InsertLeaf(int leaf) {
    if (root_ == kNull) {
        root_ = leaf;
        nodes_[static_cast<std::size_t>(leaf)].parent = kNull;
        return;
    }
    const AABB leafBox = nodes_[static_cast<std::size_t>(leaf)].box;
    int index = root_;
    // Descend using the surface-area cost heuristic.
    while (!nodes_[static_cast<std::size_t>(index)].IsLeaf()) {
        const Node& node = nodes_[static_cast<std::size_t>(index)];
        const float area = node.box.SurfaceArea();
        const float combinedArea = Math::Merge(node.box, leafBox).SurfaceArea();
        const float cost = 2.0f * combinedArea;
        const float inheritance = 2.0f * (combinedArea - area);
        auto childCost = [&](int child) {
            const Node& c = nodes_[static_cast<std::size_t>(child)];
            const float merged = Math::Merge(c.box, leafBox).SurfaceArea();
            return c.IsLeaf() ? merged + inheritance : (merged - c.box.SurfaceArea()) + inheritance;
        };
        const float cost1 = childCost(node.child1);
        const float cost2 = childCost(node.child2);
        if (cost < cost1 && cost < cost2) break;
        index = cost1 < cost2 ? node.child1 : node.child2;
    }
    const int sibling = index;
    const int oldParent = nodes_[static_cast<std::size_t>(sibling)].parent;
    const int newParent = Allocate();
    {
        Node& p = nodes_[static_cast<std::size_t>(newParent)];
        p.parent = oldParent;
        p.box = Math::Merge(leafBox, nodes_[static_cast<std::size_t>(sibling)].box);
        p.height = nodes_[static_cast<std::size_t>(sibling)].height + 1;
        p.child1 = sibling;
        p.child2 = leaf;
    }
    if (oldParent != kNull) {
        Node& op = nodes_[static_cast<std::size_t>(oldParent)];
        if (op.child1 == sibling) op.child1 = newParent; else op.child2 = newParent;
    } else {
        root_ = newParent;
    }
    nodes_[static_cast<std::size_t>(sibling)].parent = newParent;
    nodes_[static_cast<std::size_t>(leaf)].parent = newParent;
    // Refit and rebalance ancestors.
    index = nodes_[static_cast<std::size_t>(leaf)].parent;
    while (index != kNull) {
        index = Balance(index);
        Node& n = nodes_[static_cast<std::size_t>(index)];
        const Node& c1 = nodes_[static_cast<std::size_t>(n.child1)];
        const Node& c2 = nodes_[static_cast<std::size_t>(n.child2)];
        n.height = 1 + std::max(c1.height, c2.height);
        n.box = Math::Merge(c1.box, c2.box);
        index = n.parent;
    }
}

void DynamicAabbTree::RemoveLeaf(int leaf) {
    if (leaf == root_) {
        root_ = kNull;
        return;
    }
    const int parent = nodes_[static_cast<std::size_t>(leaf)].parent;
    const int grandParent = nodes_[static_cast<std::size_t>(parent)].parent;
    const int sibling = nodes_[static_cast<std::size_t>(parent)].child1 == leaf
        ? nodes_[static_cast<std::size_t>(parent)].child2 : nodes_[static_cast<std::size_t>(parent)].child1;
    if (grandParent != kNull) {
        Node& g = nodes_[static_cast<std::size_t>(grandParent)];
        if (g.child1 == parent) g.child1 = sibling; else g.child2 = sibling;
        nodes_[static_cast<std::size_t>(sibling)].parent = grandParent;
        Free(parent);
        int index = grandParent;
        while (index != kNull) {
            index = Balance(index);
            Node& n = nodes_[static_cast<std::size_t>(index)];
            const Node& c1 = nodes_[static_cast<std::size_t>(n.child1)];
            const Node& c2 = nodes_[static_cast<std::size_t>(n.child2)];
            n.box = Math::Merge(c1.box, c2.box);
            n.height = 1 + std::max(c1.height, c2.height);
            index = n.parent;
        }
    } else {
        root_ = sibling;
        nodes_[static_cast<std::size_t>(sibling)].parent = kNull;
        Free(parent);
    }
    nodes_[static_cast<std::size_t>(leaf)].parent = kNull;
}

// Performs a left or right rotation if node A is imbalanced; returns the new subtree root.
int DynamicAabbTree::Balance(int iA) {
    Node& A = nodes_[static_cast<std::size_t>(iA)];
    if (A.IsLeaf() || A.height < 2) return iA;
    const int iB = A.child1, iC = A.child2;
    Node& B = nodes_[static_cast<std::size_t>(iB)];
    Node& C = nodes_[static_cast<std::size_t>(iC)];
    const int balance = C.height - B.height;
    auto rotate = [&](int iUp, int iDown, bool upIsChild2) {
        // iUp (child of A) becomes the subtree root; A takes one of iUp's children.
        Node& up = nodes_[static_cast<std::size_t>(iUp)];
        const int iF = up.child1, iG = up.child2;
        Node& F = nodes_[static_cast<std::size_t>(iF)];
        Node& G = nodes_[static_cast<std::size_t>(iG)];
        up.child1 = iA;
        up.parent = A.parent;
        A.parent = iUp;
        if (up.parent != kNull) {
            Node& p = nodes_[static_cast<std::size_t>(up.parent)];
            if (p.child1 == iA) p.child1 = iUp; else p.child2 = iUp;
        } else {
            root_ = iUp;
        }
        const Node& down = nodes_[static_cast<std::size_t>(iDown)];
        if (F.height > G.height) {
            up.child2 = iF;
            if (upIsChild2) A.child2 = iG; else A.child1 = iG;
            G.parent = iA;
            A.box = Math::Merge(down.box, G.box);
            A.height = 1 + std::max(down.height, G.height);
            up.box = Math::Merge(A.box, F.box);
            up.height = 1 + std::max(A.height, F.height);
        } else {
            up.child2 = iG;
            if (upIsChild2) A.child2 = iF; else A.child1 = iF;
            F.parent = iA;
            A.box = Math::Merge(down.box, F.box);
            A.height = 1 + std::max(down.height, F.height);
            up.box = Math::Merge(A.box, G.box);
            up.height = 1 + std::max(A.height, G.height);
        }
        return iUp;
    };
    if (balance > 1) return rotate(iC, iB, true);
    if (balance < -1) return rotate(iB, iC, false);
    return iA;
}

void DynamicAabbTree::Query(const AABB& box, const std::function<bool(int)>& callback) const {
    if (root_ == kNull) return;
    std::vector<int> stack{root_};
    while (!stack.empty()) {
        const int index = stack.back();
        stack.pop_back();
        const Node& n = nodes_[static_cast<std::size_t>(index)];
        if (!n.box.Overlaps(box)) continue;
        if (n.IsLeaf()) {
            if (!callback(index)) return;
        } else {
            stack.push_back(n.child1);
            stack.push_back(n.child2);
        }
    }
}

void DynamicAabbTree::RayCast(const Math::Ray& ray, float maxDistance, const std::function<float(int, float)>& callback) const {
    if (root_ == kNull) return;
    float clip = maxDistance;
    std::vector<int> stack{root_};
    while (!stack.empty()) {
        const int index = stack.back();
        stack.pop_back();
        const Node& n = nodes_[static_cast<std::size_t>(index)];
        float t;
        if (!Math::IntersectRayAABB(ray, n.box, clip, t)) continue;
        if (n.IsLeaf()) {
            const float value = callback(index, clip);
            if (value == 0.0f) return;
            if (value > 0.0f) clip = std::min(clip, value);
        } else {
            stack.push_back(n.child1);
            stack.push_back(n.child2);
        }
    }
}

int DynamicAabbTree::Height() const { return root_ == kNull ? 0 : nodes_[static_cast<std::size_t>(root_)].height; }

int DynamicAabbTree::ComputeHeight(int node) const {
    const Node& n = nodes_[static_cast<std::size_t>(node)];
    if (n.IsLeaf()) return 0;
    return 1 + std::max(ComputeHeight(n.child1), ComputeHeight(n.child2));
}

bool DynamicAabbTree::Validate() const {
    if (root_ == kNull) return proxyCount_ == 0;
    if (nodes_[static_cast<std::size_t>(root_)].parent != kNull) return false;
    int leaves = 0;
    std::vector<int> stack{root_};
    while (!stack.empty()) {
        const int index = stack.back();
        stack.pop_back();
        const Node& n = nodes_[static_cast<std::size_t>(index)];
        if (n.height < 0) return false;
        if (n.IsLeaf()) {
            if (n.height != 0 || n.child2 != kNull) return false;
            ++leaves;
            continue;
        }
        const Node& c1 = nodes_[static_cast<std::size_t>(n.child1)];
        const Node& c2 = nodes_[static_cast<std::size_t>(n.child2)];
        if (c1.parent != index || c2.parent != index) return false;
        if (n.height != 1 + std::max(c1.height, c2.height)) return false;
        if (!n.box.Contains(c1.box) || !n.box.Contains(c2.box)) return false;
        if (std::abs(c1.height - c2.height) > 1) return false;
        stack.push_back(n.child1);
        stack.push_back(n.child2);
    }
    return leaves == proxyCount_ && ComputeHeight(root_) == Height();
}

} // namespace Astral::Physics
