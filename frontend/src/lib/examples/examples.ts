/**
 * Bundled C++ example programs for the examples modal.
 */

export interface Example {
	id: string;
	title: string;
	description: string;
	code: string;
	category: string;
	steps: number;
	complexity: 1 | 2 | 3; // 1 = easy, 2 = medium, 3 = complex
}

export const examples: Example[] = [
	{
		id: 'hello-world',
		title: 'Hello, world',
		description: 'Simplest possible program. Good first trace to get oriented.',
		category: 'Basics',
		steps: 3,
		complexity: 1,
		code: `#include <iostream>

int main() {
    int x = 42;
    std::cout << "Hello, World!" << std::endl;
    std::cout << "x = " << x << std::endl;
    return 0;
}`
	},
	{
		id: 'pointers-swap',
		title: 'Pointer swap & aliasing',
		description: 'Two pointers to one int; aliasing makes both values appear to change.',
		category: 'Pointers',
		steps: 8,
		complexity: 2,
		code: `#include <iostream>

int main() {
    int value = 10;
    int* ptr1 = &value;
    int* ptr2 = &value;

    std::cout << "value: " << value << std::endl;
    std::cout << "ptr1 points to: " << *ptr1 << std::endl;

    *ptr1 = 20;
    std::cout << "After *ptr1 = 20:" << std::endl;
    std::cout << "  value: " << value << std::endl;
    std::cout << "  *ptr2: " << *ptr2 << std::endl;

    return 0;
}`
	},
	{
		id: 'dynamic-array',
		title: 'Dynamic array (new int[])',
		description: 'Heap allocations with indexed cells; allocated once, freed at end.',
		category: 'Heap Arrays',
		steps: 12,
		complexity: 2,
		code: `#include <iostream>

int main() {
    // Allocate single int on heap
    int* p = new int(42);
    std::cout << "Heap int: " << *p << std::endl;

    // Allocate array on heap
    int* arr = new int[3]{1, 2, 3};
    std::cout << "Array: " << arr[0] << ", " << arr[1] << ", " << arr[2] << std::endl;

    // Clean up
    delete p;
    delete[] arr;

    return 0;
}`
	},
	{
		id: 'linked-list',
		title: 'Linked list - build & reverse',
		description: 'Allocate nodes, chain them, reverse in place. See heap relayout shine.',
		category: 'Pointers & Dynamic Memory',
		steps: 22,
		complexity: 2,
		code: `#include <iostream>

struct Node {
    int value;
    Node* next;

    Node(int v) : value(v), next(nullptr) {}
};

int main() {
    Node* head = new Node(1);
    head->next = new Node(2);
    head->next->next = new Node(3);

    // Traverse
    Node* curr = head;
    while (curr != nullptr) {
        std::cout << curr->value << " ";
        curr = curr->next;
    }
    std::cout << std::endl;

    // Cleanup
    while (head != nullptr) {
        Node* temp = head;
        head = head->next;
        delete temp;
    }

    return 0;
}`
	},
	{
		id: 'recursion',
		title: 'Recursive factorial',
		description: 'Deep stack growth and collapse - the classic recursion visualization.',
		category: 'Stack & Recursion',
		steps: 14,
		complexity: 3,
		code: `#include <iostream>

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int result = factorial(5);
    std::cout << "5! = " << result << std::endl;
    return 0;
}`
	},
	{
		id: 'bst-insertion',
		title: 'Binary search tree insertion',
		description: 'Recursively insert 5 values into an empty BST; watch the tree grow by height.',
		category: 'Recursion & Trees',
		steps: 35,
		complexity: 3,
		code: `#include <iostream>

struct TreeNode {
    int value;
    TreeNode* left;
    TreeNode* right;

    TreeNode(int v) : value(v), left(nullptr), right(nullptr) {}
};

TreeNode* insert(TreeNode* root, int val) {
    if (root == nullptr) {
        return new TreeNode(val);
    }
    if (val < root->value) {
        root->left = insert(root->left, val);
    } else {
        root->right = insert(root->right, val);
    }
    return root;
}

void inorder(TreeNode* root) {
    if (root == nullptr) return;
    inorder(root->left);
    std::cout << root->value << " ";
    inorder(root->right);
}

int main() {
    TreeNode* root = nullptr;
    root = insert(root, 5);
    root = insert(root, 3);
    root = insert(root, 7);
    root = insert(root, 2);
    root = insert(root, 4);

    std::cout << "Inorder: ";
    inorder(root);
    std::cout << std::endl;

    return 0;
}`
	}
];

/**
 * Get example by ID.
 */
export function getExample(id: string): Example | undefined {
	return examples.find((e) => e.id === id);
}

/**
 * Get examples by category.
 */
export function getExamplesByCategory(category: string): Example[] {
	return examples.filter((e) => e.category.toLowerCase().includes(category.toLowerCase()));
}

/**
 * Get all unique categories.
 */
export function getAllCategories(): string[] {
	const categories = new Set<string>();
	for (const example of examples) {
		categories.add(example.category);
	}
	return [...categories].sort();
}

/**
 * Default code shown when app loads.
 */
export const defaultCode = `#include <iostream>

int main() {
    int x = 5;
    int y = 10;
    int sum = x + y;

    std::cout << "Sum: " << sum << std::endl;

    return 0;
}
`;
