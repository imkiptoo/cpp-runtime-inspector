/**
 * Main application state using Svelte 5 runes.
 * Singleton store for all app state.
 */
import { submitTrace } from '$lib/api/client';
import type { TraceOutput, TraceStep } from '$lib/trace/schema';
import { defaultCode } from '$lib/examples/examples';
import {
	type ThemePreference,
	type ResolvedTheme,
	getStoredPreference,
	setStoredPreference,
	resolveTheme,
	applyTheme,
	subscribeToSystemTheme
} from '$lib/theme/theme';
import type { RoutingMode } from '$lib/viz/routeEdges';
import type { HeapDensity } from '$lib/viz/layoutHeap';

export type StatusKind = 'idle' | 'compiling' | 'running' | 'success' | 'error';

// localStorage keys
const STORAGE_KEYS = {
	code: 'inspector-code',
	pointerRouting: 'inspector-pointer-routing',
	heapDensity: 'inspector-heap-density',
	consoleHeight: 'inspector-console-height',
	editorFraction: 'inspector-editor-fraction',
	autoPlayOnRun: 'inspector-auto-play-on-run',
	playSpeed: 'inspector-play-speed'
} as const;

/**
 * Load from localStorage with fallback.
 */
function loadStorage<T>(key: string, fallback: T): T {
	if (typeof localStorage === 'undefined') return fallback;
	try {
		const stored = localStorage.getItem(key);
		if (stored === null) return fallback;
		return JSON.parse(stored) as T;
	} catch {
		return fallback;
	}
}

/**
 * Save to localStorage.
 */
function saveStorage<T>(key: string, value: T): void {
	if (typeof localStorage === 'undefined') return;
	try {
		localStorage.setItem(key, JSON.stringify(value));
	} catch {
		// Ignore storage errors
	}
}

/**
 * Create the app state singleton.
 */
function createAppState() {
	// === Workspace State ===
	let code = $state(loadStorage(STORAGE_KEYS.code, defaultCode));
	let lastRunCode = $state('');

	// === Execution State ===
	let running = $state(false); // True while waiting for backend
	let trace = $state<TraceOutput | null>(null);
	let error = $state<string | null>(null);
	let buildOutput = $state<string | null>(null);

	// === Navigation State ===
	let stepIndex = $state(0);
	let playing = $state(false);
	let playIntervalId: ReturnType<typeof setInterval> | null = null;

	// === UI State ===
	let consoleOpen = $state(false);
	let consoleHeightPx = $state(loadStorage(STORAGE_KEYS.consoleHeight, 200));
	let editorFraction = $state(loadStorage(STORAGE_KEYS.editorFraction, 0.4));

	// === Settings State ===
	let themePreference = $state<ThemePreference>(getStoredPreference());
	let pointerRouting = $state<RoutingMode>(loadStorage(STORAGE_KEYS.pointerRouting, 'curved'));
	let heapDensity = $state<HeapDensity>(loadStorage(STORAGE_KEYS.heapDensity, 'normal'));
	let autoPlayOnRun = $state<boolean>(loadStorage(STORAGE_KEYS.autoPlayOnRun, true));
	let playSpeed = $state<number>(loadStorage(STORAGE_KEYS.playSpeed, 700)); // ms per step

	// Subscribe to system theme changes
	let resolvedTheme = $state<ResolvedTheme>(resolveTheme(themePreference));
	let unsubscribeSystemTheme: (() => void) | null = null;

	// === Derived State ===
	const currentStep = $derived<TraceStep | null>(
		trace && trace.trace.length > 0 ? trace.trace[stepIndex] ?? null : null
	);

	const stale = $derived(code !== lastRunCode && lastRunCode !== '');

	const statusKind = $derived<StatusKind>(
		running
			? 'compiling'  // Waiting for backend
			: playing
				? 'running'   // Animation playing
				: error
					? 'error'
					: trace
						? 'success'
						: 'idle'
	);

	const totalSteps = $derived(trace?.trace.length ?? 0);

	const canStepBackward = $derived(stepIndex > 0);
	const canStepForward = $derived(trace !== null && stepIndex < totalSteps - 1);

	const hasMemoryLeaks = $derived(
		trace?.memory_leaks && trace.memory_leaks.length > 0
	);

	// === Actions ===

	/**
	 * Run the current code and fetch trace.
	 */
	async function run(): Promise<void> {
		if (running) return;

		running = true;
		error = null;
		buildOutput = null;
		stopPlaying();

		try {
			const result = await submitTrace(code);

			if (result.success) {
				trace = result.data;
				lastRunCode = code;
				stepIndex = 0;
				error = null;

				// Auto-open console if there's stdout or memory leaks
				if (result.data.trace.some((s) => s.stdout) || result.data.memory_leaks?.length) {
					consoleOpen = true;
				}

				// Auto-play if setting is enabled
				if (autoPlayOnRun && result.data.trace.length > 1) {
					startPlaying();
				}
			} else {
				error = result.error;
				buildOutput = result.compileOutput ?? null;
				trace = null;
				consoleOpen = true;
			}
		} catch (err) {
			error = err instanceof Error ? err.message : String(err);
			trace = null;
			consoleOpen = true;
		} finally {
			running = false;
		}
	}

	/**
	 * Step forward in trace.
	 */
	function stepForward(): void {
		if (trace && stepIndex < trace.trace.length - 1) {
			stepIndex++;
		}
	}

	/**
	 * Step backward in trace.
	 */
	function stepBackward(): void {
		if (stepIndex > 0) {
			stepIndex--;
		}
	}

	/**
	 * Jump to specific step.
	 */
	function stepTo(index: number): void {
		if (trace && index >= 0 && index < trace.trace.length) {
			stepIndex = index;
		}
	}

	/**
	 * Jump to start.
	 */
	function goToStart(): void {
		stepIndex = 0;
	}

	/**
	 * Jump to end.
	 */
	function goToEnd(): void {
		if (trace) {
			stepIndex = trace.trace.length - 1;
		}
	}

	/**
	 * Toggle play/pause.
	 */
	function togglePlay(): void {
		if (playing) {
			stopPlaying();
		} else {
			startPlaying();
		}
	}

	/**
	 * Start auto-advancing.
	 */
	function startPlaying(): void {
		if (!trace || playing) return;

		// If at the end, go back to start
		if (stepIndex >= trace.trace.length - 1) {
			stepIndex = 0;
		}

		playing = true;
		playIntervalId = setInterval(() => {
			if (trace && stepIndex < trace.trace.length - 1) {
				stepIndex++;
			} else {
				stopPlaying();
			}
		}, playSpeed);
	}

	/**
	 * Stop auto-advancing.
	 */
	function stopPlaying(): void {
		playing = false;
		if (playIntervalId !== null) {
			clearInterval(playIntervalId);
			playIntervalId = null;
		}
	}

	/**
	 * Jump to next occurrence of current line.
	 */
	function jumpToNextOccurrence(): void {
		if (!trace || !currentStep) return;

		const currentLine = currentStep.line;
		for (let i = stepIndex + 1; i < trace.trace.length; i++) {
			if (trace.trace[i].line === currentLine) {
				stepIndex = i;
				return;
			}
		}
		// Wrap around
		for (let i = 0; i < stepIndex; i++) {
			if (trace.trace[i].line === currentLine) {
				stepIndex = i;
				return;
			}
		}
	}

	/**
	 * Step into next function call.
	 */
	function stepInto(): void {
		if (!trace) return;

		for (let i = stepIndex + 1; i < trace.trace.length; i++) {
			if (trace.trace[i].event === 'call') {
				stepIndex = i;
				return;
			}
		}
	}

	/**
	 * Step out of current function.
	 */
	function stepOut(): void {
		if (!trace || !currentStep) return;

		const currentFunc = currentStep.func_name;
		for (let i = stepIndex + 1; i < trace.trace.length; i++) {
			if (trace.trace[i].event === 'return' && trace.trace[i].func_name === currentFunc) {
				stepIndex = i;
				return;
			}
		}
	}

	/**
	 * Set code content.
	 */
	function setCode(newCode: string): void {
		code = newCode;
		saveStorage(STORAGE_KEYS.code, newCode);
	}

	/**
	 * Load example code.
	 */
	function loadExample(exampleCode: string): void {
		setCode(exampleCode);
	}

	/**
	 * Set theme preference.
	 */
	function setThemePreference(pref: ThemePreference): void {
		themePreference = pref;
		setStoredPreference(pref);
		updateResolvedTheme();
	}

	/**
	 * Update resolved theme based on preference and system.
	 */
	function updateResolvedTheme(): void {
		resolvedTheme = resolveTheme(themePreference);
		applyTheme(resolvedTheme);
	}

	/**
	 * Initialize theme system.
	 */
	function initTheme(): void {
		updateResolvedTheme();

		// Subscribe to system changes
		unsubscribeSystemTheme = subscribeToSystemTheme(() => {
			if (themePreference === 'system') {
				updateResolvedTheme();
			}
		});
	}

	/**
	 * Cleanup subscriptions.
	 */
	function destroy(): void {
		stopPlaying();
		if (unsubscribeSystemTheme) {
			unsubscribeSystemTheme();
		}
	}

	/**
	 * Set pointer routing mode.
	 */
	function setPointerRouting(mode: RoutingMode): void {
		pointerRouting = mode;
		saveStorage(STORAGE_KEYS.pointerRouting, mode);
	}

	/**
	 * Set heap density.
	 */
	function setHeapDensity(density: HeapDensity): void {
		heapDensity = density;
		saveStorage(STORAGE_KEYS.heapDensity, density);
	}

	/**
	 * Set auto-play on run.
	 */
	function setAutoPlayOnRun(enabled: boolean): void {
		autoPlayOnRun = enabled;
		saveStorage(STORAGE_KEYS.autoPlayOnRun, enabled);
	}

	/**
	 * Set play speed (ms per step).
	 */
	function setPlaySpeed(speed: number): void {
		playSpeed = Math.max(100, Math.min(2000, speed));
		saveStorage(STORAGE_KEYS.playSpeed, playSpeed);

		// If currently playing, restart with new speed
		if (playing) {
			stopPlaying();
			startPlaying();
		}
	}

	/**
	 * Set console height.
	 */
	function setConsoleHeight(height: number): void {
		consoleHeightPx = height;
		saveStorage(STORAGE_KEYS.consoleHeight, height);
	}

	/**
	 * Set editor fraction.
	 */
	function setEditorFraction(fraction: number): void {
		editorFraction = Math.max(0.2, Math.min(0.8, fraction));
		saveStorage(STORAGE_KEYS.editorFraction, editorFraction);
	}

	/**
	 * Toggle console visibility.
	 */
	function toggleConsole(): void {
		consoleOpen = !consoleOpen;
	}

	/**
	 * Clear error state.
	 */
	function clearError(): void {
		error = null;
		buildOutput = null;
	}

	return {
		// Workspace
		get code() { return code; },
		get lastRunCode() { return lastRunCode; },

		// Execution
		get running() { return running; },
		get trace() { return trace; },
		get error() { return error; },
		get buildOutput() { return buildOutput; },

		// Navigation
		get stepIndex() { return stepIndex; },
		get playing() { return playing; },

		// UI
		get consoleOpen() { return consoleOpen; },
		get consoleHeightPx() { return consoleHeightPx; },
		get editorFraction() { return editorFraction; },

		// Settings
		get themePreference() { return themePreference; },
		get resolvedTheme() { return resolvedTheme; },
		get pointerRouting() { return pointerRouting; },
		get heapDensity() { return heapDensity; },
		get autoPlayOnRun() { return autoPlayOnRun; },
		get playSpeed() { return playSpeed; },

		// Derived
		get currentStep() { return currentStep; },
		get stale() { return stale; },
		get statusKind() { return statusKind; },
		get totalSteps() { return totalSteps; },
		get canStepBackward() { return canStepBackward; },
		get canStepForward() { return canStepForward; },
		get hasMemoryLeaks() { return hasMemoryLeaks; },

		// Actions
		run,
		stepForward,
		stepBackward,
		stepTo,
		goToStart,
		goToEnd,
		togglePlay,
		startPlaying,
		stopPlaying,
		jumpToNextOccurrence,
		stepInto,
		stepOut,
		setCode,
		loadExample,
		setThemePreference,
		initTheme,
		setPointerRouting,
		setHeapDensity,
		setAutoPlayOnRun,
		setPlaySpeed,
		setConsoleHeight,
		setEditorFraction,
		toggleConsole,
		clearError,
		destroy
	};
}

// Singleton instance
export const appState = createAppState();
