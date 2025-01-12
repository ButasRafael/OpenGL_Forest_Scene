#ifndef BVH_hpp
#define BVH_hpp

#include <vector>
#include <algorithm>
#include "BVHNode.hpp"

#define LEAF_SIZE 16

namespace gps {

    class BVH {
    public:
        BVHNode* root;

        BVH() : root(nullptr) {}

        ~BVH() {
            cleanupNode(root);
        }

        // Build the BVH from an array of pointers to MeshBatches
        void build(std::vector<MeshBatch*>& batches) {
            if (batches.empty()) return;

            // For a simple approach, we create a node that encloses everything
            root = buildRecursive(batches, 0, batches.size());
        }

        // Frustum cull traverse
        void frustumCulledDraw(const Frustum& frustum, Shader& shader) {
            traverseAndDraw(root, frustum, shader);
        }

    private:
        BVHNode* buildRecursive(std::vector<MeshBatch*>& batches, size_t start, size_t end) {
            if (start >= end) return nullptr;

            BVHNode* node = new BVHNode();


            glm::vec3 globalMin(std::numeric_limits<float>::max());
            glm::vec3 globalMax(-std::numeric_limits<float>::max());

            for (size_t i = start; i < end; i++) {
                globalMin = glm::min(globalMin, batches[i]->minBounds);
                globalMax = glm::max(globalMax, batches[i]->maxBounds);
            }
            node->minBounds = globalMin;
            node->maxBounds = globalMax;

            if (end - start <= LEAF_SIZE) {
                for (size_t i = start; i < end; ++i) {
                    node->meshBatches.push_back(batches[i]);
                }
                return node;
            }

            // 2) Decide splitting axis (x, y, or z) based on largest extent
            glm::vec3 extent = globalMax - globalMin;
            int axis = 0;
            if (extent.y > extent.x && extent.y > extent.z) axis = 1;
            if (extent.z > extent.x && extent.z > extent.y) axis = 2;

            // 3) Sort the batches by the center of their bounding box on that axis
            std::sort(batches.begin() + start, batches.begin() + end,
                [axis](MeshBatch* a, MeshBatch* b) {
                    glm::vec3 centerA = (a->minBounds + a->maxBounds) * 0.5f;
                    glm::vec3 centerB = (b->minBounds + b->maxBounds) * 0.5f;
                    return centerA[axis] < centerB[axis];
                }
            );

            // 4) Split in half
            size_t mid = (start + end) / 2;
            node->leftChild = buildRecursive(batches, start, mid);
            node->rightChild = buildRecursive(batches, mid, end);

            return node;
        }

        void traverseAndDraw(BVHNode* node, const Frustum& frustum, Shader& shader) {
            if (!node) return;

            // If bounding box is not visible, skip entire subtree
            if (!node->isVisible(frustum)) {
                return;
            }

            // If it's a leaf, draw the stored meshBatches
            if (node->isLeaf()) {
                for (auto mb : node->meshBatches) {
                    mb->Draw(shader, frustum);
                }
            }
            else {
                traverseAndDraw(node->leftChild, frustum, shader);
                traverseAndDraw(node->rightChild, frustum, shader);
            }
        }

        void cleanupNode(BVHNode* node) {
            if (!node) return;
            cleanupNode(node->leftChild);
            cleanupNode(node->rightChild);
            delete node;
        }
    };

}

#endif
