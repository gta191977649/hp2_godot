// PAL 250550..250620 / Beta249958..249A28, bware/src/bList.cpp.
// The existing native concrete Car binding retains the original stable
// intrusive-list sort and comparator; console pointer width is adapted.
#include "indep/inc/World.hpp"

void worldSortCarList(World* world)
{
    Car* root = &world->carList;
    int32_t swaps;
    do {
        swaps = 0;
        Car* first = root->next;
        if (first == root) return;
        Car* second = first->next;
        while (second != root) {
            if (worldCarIsBefore(first, second)) {
                first = second;
                second = second->next;
            } else {
                Car* after = second->next;
                Car* before = second->previous;
                before->next = after;
                after->previous = before;
                before = first->previous;
                before->next = second;
                first->previous = second;
                second->previous = before;
                second->next = first;
                ++swaps;
                second = first->next;
            }
        }
    } while (swaps);
}
