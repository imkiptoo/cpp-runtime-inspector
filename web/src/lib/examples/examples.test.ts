import { describe, it, expect } from 'vitest';
import {
	examples,
	getExample,
	getExamplesByTag,
	getAllTags,
	defaultCode,
	type Example
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
				expect(Array.isArray(example.tags)).toBe(true);
				expect(example.tags.length).toBeGreaterThan(0);
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
			expect(example?.title).toBe('Hello World');
		});

		it('returns undefined for unknown id', () => {
			const example = getExample('nonexistent');
			expect(example).toBeUndefined();
		});

		it('finds linked-list example', () => {
			const example = getExample('linked-list');
			expect(example).toBeDefined();
			expect(example?.title).toBe('Linked List');
			expect(example?.code).toContain('struct Node');
		});

		it('finds recursion example', () => {
			const example = getExample('recursion');
			expect(example).toBeDefined();
			expect(example?.code).toContain('factorial');
		});
	});

	describe('getExamplesByTag', () => {
		it('returns examples with matching tag', () => {
			const pointerExamples = getExamplesByTag('pointers');
			expect(pointerExamples.length).toBeGreaterThan(0);
			for (const example of pointerExamples) {
				expect(example.tags).toContain('pointers');
			}
		});

		it('returns empty array for unknown tag', () => {
			const noExamples = getExamplesByTag('nonexistent-tag');
			expect(noExamples).toEqual([]);
		});

		it('finds memory examples', () => {
			const memoryExamples = getExamplesByTag('memory');
			expect(memoryExamples.length).toBeGreaterThan(0);
		});

		it('finds heap examples', () => {
			const heapExamples = getExamplesByTag('heap');
			expect(heapExamples.length).toBeGreaterThanOrEqual(2);
		});
	});

	describe('getAllTags', () => {
		it('returns array of unique tags', () => {
			const tags = getAllTags();
			expect(Array.isArray(tags)).toBe(true);
			const uniqueTags = new Set(tags);
			expect(uniqueTags.size).toBe(tags.length);
		});

		it('tags are sorted alphabetically', () => {
			const tags = getAllTags();
			const sorted = [...tags].sort();
			expect(tags).toEqual(sorted);
		});

		it('includes common tags', () => {
			const tags = getAllTags();
			expect(tags).toContain('pointers');
			expect(tags).toContain('memory');
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
