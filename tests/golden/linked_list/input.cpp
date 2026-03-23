// Linked list construction test
// Verify REF chains are visualized correctly

struct Node {
    int value;
    Node* next;
};

int main() {
    // Create a simple linked list: 1 -> 2 -> 3 -> nullptr
    Node* head = new Node{1, nullptr};
    head->next = new Node{2, nullptr};
    head->next->next = new Node{3, nullptr};

    // Clean up
    Node* current = head;
    while (current != nullptr) {
        Node* next = current->next;
        delete current;
        current = next;
    }

    return 0;
}
