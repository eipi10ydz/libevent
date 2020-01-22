#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct ListNode {
    // 注意, 这里 TAILQ_ENTRY 里面填的类型需要和上面的 struct 名称一致
    TAILQ_ENTRY(ListNode) entry;
    int val;
}ListNode;

typedef TAILQ_HEAD(ListHeader, ListNode) ListHeader;

int main() {
    ListHeader listHeader;
    TAILQ_INIT(&listHeader);
    if (TAILQ_EMPTY(&listHeader)) {
        printf("list empty\n");
    }

    ListNode *node;
    for (int i = 0; i < 10; ++i) {
        // ListNode node;
        // node.val = i;
        // TAILQ_INSERT_HEAD(&listHeader, &node, entry);
        node = (ListNode*)malloc(sizeof(ListNode));
        node -> val = i;
        if (i < 5) {
            TAILQ_INSERT_HEAD(&listHeader, node, entry);
        } else {
            TAILQ_INSERT_TAIL(&listHeader, node, entry);
        }
    }

    ListNode *traverse;
    TAILQ_FOREACH(traverse, &listHeader, entry) {
        printf("%d ", traverse -> val);
    }
    printf("\n");

    ListNode *newElem = (ListNode*)malloc(sizeof(ListNode));
    newElem -> val = 20;
    TAILQ_INSERT_AFTER(&listHeader, node, newElem, entry);

    newElem = (ListNode*)malloc(sizeof(ListNode));
    newElem -> val = 30;
    TAILQ_INSERT_BEFORE(node, newElem, entry);

    TAILQ_FOREACH(traverse, &listHeader, entry) {
        printf("%d ", traverse -> val);
    }
    printf("\n");

    TAILQ_REMOVE(&listHeader, node, entry);

    TAILQ_FOREACH_REVERSE(traverse, &listHeader, ListHeader, entry) {
        printf("%d ", traverse -> val);
    }
    printf("\n");

    ListHeader listHeaderNew;
    TAILQ_INIT(&listHeaderNew);
    for (int i = 100; i < 110; ++i) {
        node = (ListNode*)malloc(sizeof(ListNode));
        node -> val = i;
        TAILQ_INSERT_TAIL(&listHeaderNew, node, entry);
    }

    printf("list new:\n");
    TAILQ_FOREACH(traverse, &listHeaderNew, entry) {
        printf("%d ", traverse -> val);
    }
    printf("\n");

    printf("list concat\n");
    TAILQ_CONCAT(&listHeader, &listHeaderNew, entry);
    printf("list orig:\n");
    TAILQ_FOREACH(traverse, &listHeader, entry) {
        printf("%d ", traverse -> val);
    }
    printf("\n");
    printf("list new:\n");
    TAILQ_FOREACH(traverse, &listHeaderNew, entry) {
        printf("%d ", traverse -> val);
    }
    printf("\n");
}
