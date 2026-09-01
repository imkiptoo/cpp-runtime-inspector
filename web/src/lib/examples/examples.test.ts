import { describe, it, expect } from 'vitest';
import {
	examples,
	getExample,
	getExamplesByCategory,
	getAllCategories,
	defaultCode
} from './examples';

describe('examples/examples', () => {
	describe('examples array', () => {
		it('has at least 6 examples', () => {
			expect(examples.length).toBeGreaterThanOrEqual(6);
		});

		it('each example has required fields', () => {
			for (const example of examples) {
				expect(example.id).toBeDefined();
				expect(typeof example.id).toBe('string');
				expect(example.title).toBeDefined();
				expect(typeof example.title).toBe('string');
				expect(example.description).toBeDefined();
				expect(typeof example.description).toBe('string');
				expect(example.code).toBeDefined();
				expect(typeof example.code).toBe('string');
				expect(typeof example.category).toBe('string');
				expect(example.category.length).toBeGreaterThan(0);
				expect(typeof example.steps).toBe('number');
				expect([1, 2, 3]).toContain(example.complexity);
			}
		});

		it('each example has unique id', () => {
			const ids = examples.map((e) => e.id);
			const uniqueIds = new Set(ids);
			expect(uniqueIds.size).toBe(ids.length);
		});

		it('each example code contains main function', () => {
			for (const example of examples) {
				expect(example.code).toContain('int main()');
			}
		});
	});

	describe('getExample', () => {
		it('returns example by id', () => {
			const example = getExample('hello-world');
			expect(example).toBeDefined();
			expect(example?.id).toBe('hello-world');
			expect(example?.title).toBe('Hello, world');
		});

		it('returns undefined for unknown id', () => {
			const example = getExample('nonexistent');
			expect(example).toBeUndefined();
		});

		it('finds linked-list example', () => {
			const example = getExample('linked-list');
			expect(example).toBeDefined();
			expect(example?.title).toBe('Linked list - build & reverse');
			expect(example?.code).toContain('struct Node');
		});

		it('finds recursion example', () => {
			const example = getExample('recursion');
			expect(example).toBeDefined();
			expect(example?.code).toContain('factorial');
		});
	});

	describe('getExamplesByCategory', () => {
		it('returns examples with matching category', () => {
			// Matching is a case-insensitive substring, so "pointers" also
			// picks up "Pointers & Dynamic Memory".
			const pointerExamples = getExamplesByCategory('pointers');
			expect(pointerExamples.length).toBeGreaterThan(0);
			for (const example of pointerExamples) {
				expect(example.category.toLowerCase()).toContain('pointers');
			}
		});

		it('is case-insensitive', () => {
			expect(getExamplesByCategory('BASICS')).toEqual(getExamplesByCategory('basics'));
		});

		it('returns empty array for unknown category', () => {
			expect(getExamplesByCategory('nonexistent-category')).toEqual([]);
		});

		it('finds memory examples', () => {
			expect(getExamplesByCategory('memory').length).toBeGreaterThan(0);
		});

		it('finds heap examples', () => {
			expect(getExamplesByCategory('heap').length).toBeGreaterThan(0);
		});

		it('finds recursion examples', () => {
			expect(getExamplesByCategory('recursion').length).toBeGreaterThanOrEqual(2);
		});
	});

	describe('getAllCategories', () => {
		it('returns array of unique categories', () => {
			const categories = getAllCategories();
			expect(Array.isArray(categories)).toBe(true);
			expect(new Set(categories).size).toBe(categories.length);
		});

		it('categories are sorted alphabetically', () => {
			const categories = getAllCategories();
			expect(categories).toEqual([...categories].sort());
		});

		it('covers every example category', () => {
			const categories = getAllCategories();
			for (const example of examples) {
				expect(categories).toContain(example.category);
			}
		});

		it('includes common categories', () => {
			const categories = getAllCategories();
			expect(categories).toContain('Basics');
			expect(categories).toContain('Pointers');
		});
	});

	describe('defaultCode', () => {
		it('is a valid C++ program', () => {
			expect(defaultCode).toContain('#include');
			expect(defaultCode).toContain('int main()');
			expect(defaultCode).toContain('return 0');
		});

		it('has simple variable operations', () => {
			expect(defaultCode).toContain('int x');
			expect(defaultCode).toContain('int y');
			expect(defaultCode).toContain('sum');
		});
	});
});
