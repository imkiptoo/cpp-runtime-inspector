import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { submitTrace, checkHealth } from './client';

// Mock fetch globally
const mockFetch = vi.fn();
global.fetch = mockFetch;

describe('api/client', () => {
	beforeEach(() => {
		mockFetch.mockClear();
	});

	describe('submitTrace', () => {
		it('sends code to trace endpoint', async () => {
			const traceData = {
				code: 'int main() { return 0; }',
				trace: [
					{
						line: 1,
						event: 'call',
						func_name: 'main',
						stack_to_render: [],
						globals: {},
						ordered_globals: [],
						heap: {},
						stdout: ''
					}
				]
			};

			mockFetch.mockResolvedValueOnce({
				ok: true,
				text: () => Promise.resolve(JSON.stringify(traceData))
			});

			const code = 'int main() { return 0; }';
			const result = await submitTrace(code);

			expect(mockFetch).toHaveBeenCalledWith(
				'/trace',
				expect.objectContaining({
					method: 'POST',
					headers: { 'Content-Type': 'text/plain' },
					body: code
				})
			);

			expect(result.success).toBe(true);
			if (result.success) {
				expect(result.data.code).toBe(code);
				expect(result.data.trace.length).toBe(1);
			}
		});

		it('handles server error response', async () => {
			mockFetch.mockResolvedValueOnce({
				ok: false,
				status: 500,
				text: () => Promise.resolve('Internal Server Error')
			});

			const result = await submitTrace('code');

			expect(result.success).toBe(false);
			if (!result.success) {
				expect(result.error).toContain('Server error');
				expect(result.error).toContain('500');
			}
		});

		it('handles compilation error response', async () => {
			// Backend sends: error (phase), message (description), details (compiler output)
			const errorData = {
				error: 'compilation',
				message: 'Failed to compile instrumented source',
				details: "error: expected ';' before '}'"
			};

			mockFetch.mockResolvedValueOnce({
				ok: false,
				status: 400,
				text: () => Promise.resolve(JSON.stringify(errorData))
			});

			const result = await submitTrace('invalid code');

			expect(result.success).toBe(false);
			if (!result.success) {
				expect(result.error).toContain('compilation');
				expect(result.error).toContain('Failed to compile');
				expect(result.compileOutput).toContain("expected ';'");
			}
		});

		it('handles invalid JSON response', async () => {
			mockFetch.mockResolvedValueOnce({
				ok: true,
				text: () => Promise.resolve('not valid json')
			});

			const result = await submitTrace('code');

			expect(result.success).toBe(false);
			if (!result.success) {
				expect(result.error).toContain('Invalid JSON');
			}
		});

		it('handles invalid trace format', async () => {
			// Missing required fields
			mockFetch.mockResolvedValueOnce({
				ok: true,
				text: () => Promise.resolve(JSON.stringify({ invalid: 'data' }))
			});

			const result = await submitTrace('code');

			expect(result.success).toBe(false);
			if (!result.success) {
				expect(result.error).toContain('Invalid trace format');
			}
		});

		it('handles network error', async () => {
			mockFetch.mockRejectedValueOnce(new TypeError('fetch failed'));

			const result = await submitTrace('code');

			expect(result.success).toBe(false);
			if (!result.success) {
				expect(result.error).toContain('Cannot connect');
			}
		});

		it('handles abort error', async () => {
			const abortError = new DOMException('Aborted', 'AbortError');
			mockFetch.mockRejectedValueOnce(abortError);

			const result = await submitTrace('code');

			expect(result.success).toBe(false);
			if (!result.success) {
				expect(result.error).toContain('timed out');
			}
		});

		it('handles trace with heap objects', async () => {
			const traceData = {
				code: 'int main() { int* p = new int(42); delete p; return 0; }',
				trace: [
					{
						line: 1,
						event: 'step_line',
						func_name: 'main',
						stack_to_render: [
							{
								frame_id: 0,
								func_name: 'main',
								encoded_locals: { p: ['REF', 1] },
								ordered_varnames: ['p'],
								is_highlighted: true,
								is_zombie: false
							}
						],
						globals: {},
						ordered_globals: [],
						heap: {
							'1': ['HEAP_PRIMITIVE', 'int', 42]
						},
						stdout: ''
					}
				]
			};

			mockFetch.mockResolvedValueOnce({
				ok: true,
				text: () => Promise.resolve(JSON.stringify(traceData))
			});

			const result = await submitTrace('code');

			expect(result.success).toBe(true);
			if (result.success) {
				expect(result.data.trace[0].heap['1']).toEqual(['HEAP_PRIMITIVE', 'int', 42]);
			}
		});

		it('handles trace with memory leaks', async () => {
			const traceData = {
				code: 'int main() { int* p = new int(42); return 0; }',
				trace: [],
				memory_leaks: [['leaked', 1, 'int']]
			};

			mockFetch.mockResolvedValueOnce({
				ok: true,
				text: () => Promise.resolve(JSON.stringify(traceData))
			});

			const result = await submitTrace('code');

			expect(result.success).toBe(true);
			if (result.success) {
				expect(result.data.memory_leaks).toHaveLength(1);
				expect(result.data.memory_leaks![0]).toEqual(['leaked', 1, 'int']);
			}
		});
	});

	describe('checkHealth', () => {
		it('returns healthy when server responds ok', async () => {
			mockFetch.mockResolvedValueOnce({
				ok: true,
				json: () => Promise.resolve({ status: 'ok' })
			});

			const result = await checkHealth();

			expect(result.healthy).toBe(true);
			expect(result.message).toBe('ok');
		});

		it('returns unhealthy when server responds with error', async () => {
			mockFetch.mockResolvedValueOnce({
				ok: false,
				status: 503
			});

			const result = await checkHealth();

			expect(result.healthy).toBe(false);
			expect(result.message).toContain('503');
		});

		it('returns unhealthy when fetch fails', async () => {
			mockFetch.mockRejectedValueOnce(new Error('Network error'));

			const result = await checkHealth();

			expect(result.healthy).toBe(false);
			expect(result.message).toContain('Cannot connect');
		});
	});
});
