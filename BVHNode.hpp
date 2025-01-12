#ifndef BVHNode_hpp
#define BVHNode_hpp

#include <vector>
#include "glm/glm.hpp"
#include "Frustum.hpp"
#include "MeshBatch.hpp"

namespace gps {

    class BVHNode {
    public:
        // Child pointers (in a typical BVH, you might have 2 for a binary tree;
        // or for a more general bounding volume hierarchy, you may have multiple).
        BVHNode* leftChild;
        BVHNode* rightChild;

        // The bounding box that encloses children or objects at this node
        glm::vec3 minBounds;
        glm::vec3 maxBounds;

        // Leaf data: If this node is a leaf, store the MeshBatches
        std::vector<MeshBatch*> meshBatches;

        // Constructor
        BVHNode() : leftChild(nullptr), rightChild(nullptr) {}

        bool isLeaf() const {
            return (leftChild == nullptr && rightChild == nullptr);
        }

        // Frustum culling at the node level
        bool isVisible(const Frustum& frustum) const {
            // If the entire bounding box is out of view, return false
            return frustum.isVisible(minBounds, maxBounds);
        }
    };

}

#endif
