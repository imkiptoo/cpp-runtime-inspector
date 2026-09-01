/**
 * API client for communicating with the C++ Runtime Inspector backend.
 */
import { TraceOutputSchema, TraceErrorSchema } from '$lib/trace/schema';
import type { TraceOutput, TraceError } from '$lib/trace/schema';

export interface TraceResult {
	success: true;
	data: TraceOutput;
}

export interface TraceFailure {
	success: false;
	error: string;
	compileOutput?: string;
	/** Rewritten source, when the plugin ran but a later phase failed. */
	instrumentedSource?: string;
}

export type TraceResponse = TraceResult | TraceFailure;

const TRACE_ENDPOINT = '/trace';
const HEALTH_ENDPOINT = '/health';
const DEFAULT_TIMEOUT = 60000; // 60 seconds for compilation + execution

/**
 * Submit C++ code for tracing and receive execution trace.
 */
export async function submitTrace(
	code: string,
	options: { timeout?: number; signal?: AbortSignal } = {}
): Promise<TraceResponse> {
	const { timeout = DEFAULT_TIMEOUT, signal } = options;

	const controller = new AbortController();
	const timeoutId = setTimeout(() => controller.abort(), timeout);

	// Combine user signal with timeout
	const combinedSignal = signal
		? AbortSignal.any([signal, controller.signal])
		: controller.signal;

	try {
		const response = await fetch(TRACE_ENDPOINT, {
			method: 'POST',
			headers: {
				'Content-Type': 'text/plain'
			},
			body: code,
			signal: combinedSignal
		});

		clearTimeout(timeoutId);

		const text = await response.text();

		if (!response.ok) {
			// Try to parse as error response
			try {
				const errorData = JSON.parse(text);
				const parsed = TraceErrorSchema.safeParse(errorData);
				if (parsed.success) {
					// Backend sends: error (phase), message (description), details (compiler output)
					const errorMsg = `${parsed.data.error}: ${parsed.data.message}`;
					return {
						success: false,
						error: errorMsg,
						compileOutput: parsed.data.details,
						instrumentedSource: parsed.data.instrumented_source
					};
				}
			} catch {
				// Not JSON, use raw text
			}

			return {
				success: false,
				error: `Server error (${response.status}): ${text.slice(0, 500)}`
			};
		}

		// Parse successful response
		let data: unknown;
		try {
			data = JSON.parse(text);
		} catch {
			return {
				success: false,
				error: 'Invalid JSON response from server'
			};
		}

		// Validate with Zod schema
		const parsed = TraceOutputSchema.safeParse(data);
		if (!parsed.success) {
			console.error('Trace validation errors:', parsed.error.issues);
			return {
				success: false,
				error: `Invalid trace format: ${parsed.error.issues.map((i) => i.message).join(', ')}`
			};
		}

		return {
			success: true,
			data: parsed.data
		};
	} catch (err) {
		clearTimeout(timeoutId);

		if (err instanceof DOMException && err.name === 'AbortError') {
			return {
				success: false,
				error: 'Request timed out or was cancelled'
			};
		}

		if (err instanceof TypeError && err.message.includes('fetch')) {
			return {
				success: false,
				error: 'Cannot connect to backend server. Is it running? (start it with `just run backend`)'
			};
		}

		return {
			success: false,
			error: `Network error: ${err instanceof Error ? err.message : String(err)}`
		};
	}
}

/**
 * Check backend health status.
 */
export async function checkHealth(): Promise<{ healthy: boolean; message?: string }> {
	try {
		const response = await fetch(HEALTH_ENDPOINT, {
			method: 'GET',
			signal: AbortSignal.timeout(5000)
		});

		if (response.ok) {
			const data = await response.json();
			return { healthy: true, message: data.status };
		}

		return { healthy: false, message: `Status ${response.status}` };
	} catch {
		return { healthy: false, message: 'Cannot connect to backend' };
	}
}
